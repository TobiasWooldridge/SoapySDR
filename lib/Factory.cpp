// Copyright (c) 2014-2018 Josh Blum
//                    2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Constants.h>
#include <algorithm>
#include <stdexcept>
#include <exception>
#include <future>
#include <iterator>
#include <chrono>
#include <mutex>
#include <atomic>

static std::recursive_mutex &getFactoryMutex(void)
{
    static std::recursive_mutex mutex;
    return mutex;
}

typedef std::map<SoapySDR::Kwargs, SoapySDR::Device *> DeviceTable;

static DeviceTable &getDeviceTable(void)
{
    static DeviceTable table;
    return table;
}

typedef std::map<SoapySDR::Device *, size_t> DeviceCounts;

static DeviceCounts &getDeviceCounts(void)
{
    static DeviceCounts table;
    return table;
}

void automaticLoadModules(void);

/*******************************************************************
 * Cancellation support
 ******************************************************************/
static std::atomic<bool> &getCancelFlag(void)
{
    static std::atomic<bool> flag(false);
    return flag;
}

void SoapySDR::Device::cancelEnumerate(void)
{
    getCancelFlag().store(true);
}

void SoapySDR::Device::cancelMake(void)
{
    getCancelFlag().store(true);
}

bool SoapySDR::Device::isCancelled(void)
{
    return getCancelFlag().load();
}

void SoapySDR::Device::clearCancel(void)
{
    getCancelFlag().store(false);
}

/*******************************************************************
 * Pre-open capability query
 ******************************************************************/
SoapySDR::Device::DeviceCapabilities SoapySDR::Device::queryCapabilities(const Kwargs &args, const long timeoutUs)
{
    DeviceCapabilities caps;

    //briefly open the device to query capabilities
    Device *device = nullptr;
    try
    {
        device = Device::make(args, timeoutUs);
        if (device == nullptr)
        {
            throw std::runtime_error("SoapySDR::Device::queryCapabilities() device not found");
        }

        //populate capabilities
        caps.driverKey = device->getDriverKey();
        caps.hardwareKey = device->getHardwareKey();
        caps.numRxChannels = device->getNumChannels(SOAPY_SDR_RX);
        caps.numTxChannels = device->getNumChannels(SOAPY_SDR_TX);

        //get frequency and sample rate ranges for first RX channel
        if (caps.numRxChannels > 0)
        {
            caps.frequencyRange = device->getFrequencyRange(SOAPY_SDR_RX, 0);
            caps.sampleRateRange = device->getSampleRateRange(SOAPY_SDR_RX, 0);
            caps.antennas = device->listAntennas(SOAPY_SDR_RX, 0);
            caps.gains = device->listGains(SOAPY_SDR_RX, 0);
            caps.streamFormats = device->getStreamFormats(SOAPY_SDR_RX, 0);
            caps.supportsAgc = device->hasGainMode(SOAPY_SDR_RX, 0);
            caps.supportsFullDuplex = device->getFullDuplex(SOAPY_SDR_RX, 0);
        }

        //get hardware info
        caps.extraInfo = device->getHardwareInfo();

        Device::unmake(device);
    }
    catch (...)
    {
        if (device != nullptr) Device::unmake(device);
        throw;
    }

    return caps;
}

SoapySDR::Device::DeviceCapabilities SoapySDR::Device::queryCapabilities(const std::string &args, const long timeoutUs)
{
    return queryCapabilities(KwargsFromString(args), timeoutUs);
}

/*******************************************************************
 * Enumeration implementation
 ******************************************************************/
SoapySDR::KwargsList SoapySDR::Device::enumerate(const Kwargs &args, const long timeoutUs)
{
    //use default timeout if none specified to avoid blocking forever
    const long effectiveTimeout = (timeoutUs <= 0) ? 10000000 : timeoutUs; //10 seconds default

    automaticLoadModules(); //perform one-shot load

    //enumerate cache data structure
    //(driver key, find args) -> (timestamp, handles list)
    //Since available devices should not change rapidly,
    //the cache allows the enumerate results to persist for some time
    //across multiple concurrent callers or subsequent sequential calls.
    static std::recursive_mutex cacheMutex;
    static std::map<std::pair<std::string, Kwargs>,
        std::pair<std::chrono::high_resolution_clock::time_point, std::shared_future<KwargsList>>
    > cache;

    //clean expired entries from the cache
    {
        static const auto CACHE_TIMEOUT = std::chrono::seconds(1);
        std::lock_guard<std::recursive_mutex> lock(cacheMutex);
        const auto now = std::chrono::high_resolution_clock::now();
        for (auto it = cache.begin(); it != cache.end();)
        {
            if (it->second.first+CACHE_TIMEOUT < now) cache.erase(it++);
            else it++;
        }
    }

    //launch futures to enumerate devices for each module
    std::map<std::string, std::shared_future<KwargsList>> futures;
    for (const auto &it : Registry::listFindFunctions())
    {
        const bool specifiedDriver = args.count("driver") != 0;
        if (specifiedDriver && args.at("driver") != it.first) continue;

        //protect the cache to search it for results and update it
        std::lock_guard<std::recursive_mutex> lock(cacheMutex);
        auto &cacheEntry = cache[std::make_pair(it.first, args)];

        //use the cache entry if its been initialized (valid) and not expired
        if (cacheEntry.second.valid() && (cacheEntry.first + std::chrono::seconds(1)) > std::chrono::high_resolution_clock::now())
        {
            futures[it.first] = cacheEntry.second;
        }

        //otherwise create a new future and place it into the cache
        else
        {
            //always use async launch so wait_for() can timeout properly
            //deferred launch causes wait_for() to return deferred status forever
            futures[it.first] = std::async(std::launch::async, it.second, args);
            cacheEntry = std::make_pair(std::chrono::high_resolution_clock::now(), futures[it.first]);
        }
    }

    //collect the asynchronous results with timeout
    SoapySDR::KwargsList results;
    const auto deadline = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(effectiveTimeout);

    for (auto &it : futures)
    {
        //check for cancellation
        if (getCancelFlag().load())
        {
            SoapySDR::logf(SOAPY_SDR_INFO, "SoapySDR::Device::enumerate() cancelled");
            break;
        }

        try
        {
            //poll with short intervals to allow cancellation and timeout
            bool timedOut = false;
            bool cancelled = false;
            while (true)
            {
                //check for cancellation
                if (getCancelFlag().load())
                {
                    SoapySDR::logf(SOAPY_SDR_INFO, "SoapySDR::Device::enumerate() cancelled");
                    cancelled = true;
                    break;
                }
                //check for overall timeout
                const auto now = std::chrono::high_resolution_clock::now();
                if (now >= deadline)
                {
                    SoapySDR::logf(SOAPY_SDR_WARNING, "SoapySDR::Device::enumerate(%s) timed out", it.first.c_str());
                    timedOut = true;
                    break;
                }
                //wait for this future with short poll interval
                const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now);
                const auto pollInterval = std::min(remaining, std::chrono::microseconds(100000)); //100ms max
                if (it.second.wait_for(pollInterval) == std::future_status::ready)
                {
                    break; //future is ready, exit polling loop
                }
                //continue polling this same future
            }
            if (cancelled) break; //break out of outer futures loop
            if (timedOut) continue; //skip this future, try next

            for (auto handle : it.second.get())
            {
                handle["driver"] = it.first;
                results.push_back(handle);
            }
        }
        catch (const std::exception &ex)
        {
            SoapySDR::logf(SOAPY_SDR_ERROR, "SoapySDR::Device::enumerate(%s) %s", it.first.c_str(), ex.what());
        }
        catch (...)
        {
            SoapySDR::logf(SOAPY_SDR_ERROR, "SoapySDR::Device::enumerate(%s) unknown error", it.first.c_str());
        }
    }
    return results;
}

SoapySDR::KwargsList SoapySDR::Device::enumerate(const std::string &args, const long timeoutUs)
{
    return enumerate(KwargsFromString(args), timeoutUs);
}

std::future<SoapySDR::KwargsList> SoapySDR::Device::enumerateAsync(const Kwargs &args)
{
    return std::async(std::launch::async, [args]() {
        return SoapySDR::Device::enumerate(args, 0);
    });
}

std::future<SoapySDR::KwargsList> SoapySDR::Device::enumerateAsync(const std::string &args)
{
    return enumerateAsync(KwargsFromString(args));
}

static SoapySDR::Device* getDeviceFromTable(const SoapySDR::Kwargs &args)
{
    if (args.empty()) return nullptr;
    const auto it = getDeviceTable().find(args);
    if (it == getDeviceTable().end()) return nullptr;
    const auto device = it->second;
    if (device == nullptr) throw std::runtime_error("SoapySDR::Device::make() device deletion in-progress");
    getDeviceCounts()[device]++;
    return device;
}

SoapySDR::Device* SoapySDR::Device::make(const Kwargs &inputArgs, const long timeoutUs)
{
    //use default timeout if none specified to avoid blocking forever
    const long effectiveTimeout = (timeoutUs <= 0) ? 30000000 : timeoutUs; //30 seconds default

    //check for cancellation before starting
    if (getCancelFlag().load())
    {
        throw std::runtime_error("SoapySDR::Device::make() cancelled");
    }

    std::unique_lock<std::recursive_mutex> lock(getFactoryMutex());

    //the arguments may have already come from enumerate and been used to open a device
    auto device = getDeviceFromTable(inputArgs);
    if (device != nullptr) return device;

    //otherwise the args must always come from an enumeration result
    //unlock the mutex to block on the enumeration call
    Kwargs discoveredArgs;
    lock.unlock();
    const auto results = Device::enumerate(inputArgs, effectiveTimeout);
    if (!results.empty()) discoveredArgs = results.front();
    lock.lock();

    //check the device table for an already allocated device
    device = getDeviceFromTable(discoveredArgs);
    if (device != nullptr) return device;

    //load the enumeration args with missing keys from the make argument
    Kwargs hybridArgs = discoveredArgs;
    for (const auto &it : inputArgs)
    {
        if (hybridArgs.count(it.first) == 0) hybridArgs[it.first] = it.second;
    }

    //dont continue when driver is unspecified,
    //unless there is only one available driver option
    const bool specifiedDriver = hybridArgs.count("driver") != 0;
    const auto makeFunctions = Registry::listMakeFunctions();
    if (!specifiedDriver && makeFunctions.size() > 2) //more than factory: null + one loaded driver
    {
        throw std::runtime_error("SoapySDR::Device::make() no driver specified and no enumeration results");
    }

    //search for a cache entry or launch a future if not found
    //cache must be static to deduplicate concurrent make() calls
    static std::map<Kwargs, std::shared_future<Device *>> cache;
    std::shared_future<Device *> deviceFuture;
    for (const auto &it : makeFunctions)
    {
        if (!specifiedDriver && it.first == "null") continue; //skip null unless explicitly specified
        if (specifiedDriver && hybridArgs.at("driver") != it.first) continue; //filter for driver match
        auto &cacheEntry = cache[discoveredArgs];
        //use async launch so the driver make runs in background thread
        //deferred launch causes wait_for() to return deferred status forever
        if (!cacheEntry.valid()) cacheEntry = std::async(std::launch::async, it.second, hybridArgs);
        deviceFuture = cacheEntry;
        break;
    }

    //no match found for the arguments in the loop above
    if (!deviceFuture.valid()) throw std::runtime_error("SoapySDR::Device::make() no match");

    //unlock the mutex to block on the factory call with timeout
    lock.unlock();
    {
        const auto deadline = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(effectiveTimeout);
        while (true)
        {
            //check for cancellation
            if (getCancelFlag().load())
            {
                throw std::runtime_error("SoapySDR::Device::make() cancelled");
            }
            //poll with short intervals to allow cancellation
            const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
                deadline - std::chrono::high_resolution_clock::now());
            if (remaining.count() <= 0)
            {
                throw std::runtime_error("SoapySDR::Device::make() timed out");
            }
            const auto pollInterval = std::min(remaining, std::chrono::microseconds(100000)); //100ms max
            const auto status = deviceFuture.wait_for(pollInterval);
            if (status == std::future_status::ready) break;
        }
    }
    lock.lock();

    //the future is complete, erase the cache entry
    //other callers have a copy of the shared future copy or a device table entry
    cache.erase(discoveredArgs);

    //store into the table
    device = deviceFuture.get(); //may throw
    getDeviceTable()[discoveredArgs] = device;
    getDeviceCounts()[device]++;

    return device;
}

SoapySDR::Device *SoapySDR::Device::make(const std::string &args, const long timeoutUs)
{
    return make(KwargsFromString(args), timeoutUs);
}

void SoapySDR::Device::unmake(Device *device)
{
    if (device == nullptr) return; //safe to unmake a null device

    std::unique_lock<std::recursive_mutex> lock(getFactoryMutex());

    auto countIt = getDeviceCounts().find(device);
    if (countIt == getDeviceCounts().end())
    {
        throw std::runtime_error("SoapySDR::Device::unmake() unknown device");
    }

    if ((--countIt->second) != 0) return;

    //cleanup case for last instance of open device
    getDeviceCounts().erase(countIt);

    //nullify matching entries in the device table
    //make throws if it matches handles which are being deleted
    KwargsList argsList;
    for (auto &it : getDeviceTable())
    {
        if (it.second != device) continue;
        argsList.push_back(it.first);
        it.second = nullptr;
    }

    //do not block other callers while we wait on destructor
    lock.unlock();
    delete device;
    lock.lock();

    //now clean the device table to signal that deletion is complete
    for (const auto &args : argsList) getDeviceTable().erase(args);
}

/*******************************************************************
 * Parallel support
 ******************************************************************/
std::vector<SoapySDR::Device *> SoapySDR::Device::make(const KwargsList &argsList)
{
    std::vector<std::future<Device *>> futures;
    for (const auto &args : argsList)
    {
        futures.push_back(std::async(std::launch::async, [args]{return SoapySDR::Device::make(args);}));
    }

    std::vector<Device *> devices;
    try
    {
        for (auto &future : futures) devices.push_back(future.get());
    }
    catch(...)
    {
        //cleanup all devices made so far, and squelch their errors
        try{SoapySDR::Device::unmake(devices);}
        catch(...){}

        //and then rethrow the exception after cleanup
        throw;
    }
    return devices;
}

std::vector<SoapySDR::Device *> SoapySDR::Device::make(const std::vector<std::string> &argsList)
{
    SoapySDR::KwargsList kwargsList;
    std::transform(
        argsList.begin(),
        argsList.end(),
        std::back_inserter(kwargsList),
        SoapySDR::KwargsFromString);

    return make(kwargsList);
}

void SoapySDR::Device::unmake(const std::vector<Device *> &devices)
{
    std::vector<std::future<void>> futures;
    for (const auto &device : devices)
    {
        futures.push_back(std::async(std::launch::async, [device]{SoapySDR::Device::unmake(device);}));
    }

    //unmake will only throw the last exception
    //Since unmake only throws for unknown handles, this is probably API misuse.
    //The actual particular exception and its associated device is not important.
    std::exception_ptr eptr;
    for (auto &future : futures)
    {
        try {future.get();}
        catch(...){eptr = std::current_exception();}
    }
    if (eptr) std::rethrow_exception(eptr);
}
