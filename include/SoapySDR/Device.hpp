///
/// \file SoapySDR/Device.hpp
///
/// Interface definition for Soapy SDR devices.
///
/// \copyright
/// Copyright (c) 2014-2019 Josh Blum
/// Copyright (c) 2016-2016 Bastille Networks
///                    2021 Nicholas Corgan
/// SPDX-License-Identifier: BSL-1.0
///

#pragma once
#include <SoapySDR/Config.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Constants.h>
#include <SoapySDR/Errors.h>
#include <vector>
#include <string>
#include <complex>
#include <cstddef> //size_t
#include <cstdint> //uint64_t, uint32_t
#include <future>

namespace SoapySDR
{

//! Forward declaration of stream handle for type safety
class Stream;

/*!
 * Abstraction for an SDR transceiver device - configuration and streaming.
 */
class SOAPY_SDR_API Device
{
public:

    //! virtual destructor for inheritance
    virtual ~Device(void);

    /*!
     * Enumerate a list of available devices on the system.
     * \param args device construction key/value argument filters
     * \param timeoutUs timeout in microseconds (0 = no timeout, default)
     * \return a list of argument maps, each unique to a device
     */
    static KwargsList enumerate(const Kwargs &args = Kwargs(), const long timeoutUs = 0);

    /*!
     * Enumerate a list of available devices on the system.
     * Markup format for args: "keyA=valA, keyB=valB".
     * \param args a markup string of key/value argument filters
     * \param timeoutUs timeout in microseconds (0 = no timeout, default)
     * \return a list of argument maps, each unique to a device
     */
    static KwargsList enumerate(const std::string &args, const long timeoutUs = 0);

    /*!
     * Asynchronously enumerate a list of available devices on the system.
     * Returns immediately with a future that will hold the results.
     * \param args device construction key/value argument filters
     * \return a future containing a list of argument maps, each unique to a device
     */
    static std::future<KwargsList> enumerateAsync(const Kwargs &args = Kwargs());

    /*!
     * Asynchronously enumerate a list of available devices on the system.
     * Returns immediately with a future that will hold the results.
     * Markup format for args: "keyA=valA, keyB=valB".
     * \param args a markup string of key/value argument filters
     * \return a future containing a list of argument maps, each unique to a device
     */
    static std::future<KwargsList> enumerateAsync(const std::string &args);

    /*!
     * Cancel any in-progress enumerate operations.
     * This allows applications to abort stuck enumeration from another thread.
     * Pending enumerate calls will return with partial or empty results.
     * Thread-safe: can be called from any thread.
     */
    static void cancelEnumerate(void);

    /*!
     * Cancel any in-progress make operations.
     * This allows applications to abort stuck device creation from another thread.
     * Pending make calls will throw an exception indicating cancellation.
     * Thread-safe: can be called from any thread.
     */
    static void cancelMake(void);

    /*!
     * Check if operations are currently cancelled.
     * \return true if cancel has been requested
     */
    static bool isCancelled(void);

    /*!
     * Clear the cancellation flag.
     * Call this before starting new operations after a cancel.
     */
    static void clearCancel(void);

    /*******************************************************************
     * Pre-open Capability Query API
     ******************************************************************/

    /*!
     * Device capabilities that can be queried without opening the device.
     * This allows applications to filter devices before attempting to open them.
     */
    struct DeviceCapabilities
    {
        std::string driverKey;              //!< Driver name (e.g., "rtlsdr", "sdrplay")
        std::string hardwareKey;            //!< Hardware identifier
        size_t numRxChannels;               //!< Number of receive channels
        size_t numTxChannels;               //!< Number of transmit channels
        RangeList frequencyRange;           //!< Overall tunable frequency range
        RangeList sampleRateRange;          //!< Available sample rates
        std::vector<std::string> antennas;  //!< Available antenna names
        std::vector<std::string> gains;     //!< Available gain element names
        std::vector<std::string> streamFormats; //!< Supported stream formats
        bool supportsAgc;                   //!< Whether AGC is supported
        bool supportsFullDuplex;            //!< Whether full duplex is supported
        Kwargs extraInfo;                   //!< Additional device-specific info
    };

    /*!
     * Query device capabilities without fully opening the device.
     * This is a lightweight alternative to make() for getting device info.
     * The query may still need to briefly access the device but will
     * release it immediately after gathering information.
     *
     * \param args device identification arguments (from enumerate)
     * \param timeoutUs timeout in microseconds (0 = no timeout)
     * \return DeviceCapabilities structure with device information
     * \throws std::runtime_error if device cannot be queried
     */
    static DeviceCapabilities queryCapabilities(const Kwargs &args, const long timeoutUs = 0);

    /*!
     * Query device capabilities without fully opening the device.
     * \param args a markup string of key/value arguments
     * \param timeoutUs timeout in microseconds (0 = no timeout)
     * \return DeviceCapabilities structure with device information
     */
    static DeviceCapabilities queryCapabilities(const std::string &args, const long timeoutUs = 0);

    /*!
     * Make a new Device object given device construction args.
     * The device pointer will be stored in a table so subsequent calls
     * with the same arguments will produce the same device.
     * For every call to make, there should be a matched call to unmake.
     *
     * \param args device construction key/value argument map
     * \param timeoutUs timeout in microseconds (0 = no timeout, default)
     * \return a pointer to a new Device object
     */
    static Device *make(const Kwargs &args = Kwargs(), const long timeoutUs = 0);

    /*!
     * Make a new Device object given device construction args.
     * The device pointer will be stored in a table so subsequent calls
     * with the same arguments will produce the same device.
     * For every call to make, there should be a matched call to unmake.
     *
     * \param args a markup string of key/value arguments
     * \param timeoutUs timeout in microseconds (0 = no timeout, default)
     * \return a pointer to a new Device object
     */
    static Device *make(const std::string &args, const long timeoutUs = 0);

    /*!
     * Unmake or release a device object handle.
     *
     * \param device a pointer to a device object
     */
    static void unmake(Device *device);

    /*******************************************************************
     * Parallel support
     ******************************************************************/

    /*!
     * Create a list of devices from a list of construction arguments.
     * This is a convenience call to parallelize device construction,
     * and is fundamentally a parallel for loop of make(Kwargs).
     *
     * \param argsList a list of device arguments per each device
     * \return a list of device pointers per each specified argument
     */
    static std::vector<Device *> make(const KwargsList &argsList);

    /*!
     * Create a list of devices from a list of construction arguments.
     * This is a convenience call to parallelize device construction,
     * and is fundamentally a parallel for loop of make(args).
     *
     * \param argsList a list of device arguments per each device
     * \return a list of device pointers per each specified argument
     */
    static std::vector<Device *> make(const std::vector<std::string> &argsList);

    /*!
     * Unmake or release a list of device handles.
     * This is a convenience call to parallelize device destruction,
     * and is fundamentally a parallel for loop of unmake(Device *).
     *
     * \param devices a list of pointers to device objects
     */
    static void unmake(const std::vector<Device *> &devices);

    /*******************************************************************
     * Identification API
     ******************************************************************/

    /*!
     * A key that uniquely identifies the device driver.
     * This key identifies the underlying implementation.
     * Several variants of a product may share a driver.
     */
    virtual std::string getDriverKey(void) const;

    /*!
     * A key that uniquely identifies the hardware.
     * This key should be meaningful to the user
     * to optimize for the underlying hardware.
     */
    virtual std::string getHardwareKey(void) const;

    /*!
     * Query a dictionary of available device information.
     * This dictionary can any number of values like
     * vendor name, product name, revisions, serials...
     * This information can be displayed to the user
     * to help identify the instantiated device.
     */
    virtual Kwargs getHardwareInfo(void) const;

    /*******************************************************************
     * Health Check API
     ******************************************************************/

    /*!
     * Structure containing detailed device health information.
     */
    struct HealthStatus
    {
        bool responsive;        //!< True if device appears responsive
        std::string state;      //!< State: "ok", "disconnected", "error", "unknown"
        std::string message;    //!< Human-readable status message
        double lastSuccessfulOpTime; //!< Timestamp of last successful operation (0 if unknown)
    };

    /*!
     * Check if the device is responsive.
     * This is a quick health check that should complete within 100ms
     * even if the device is hung. Use this for watchdog monitoring
     * or to detect USB disconnection before attempting operations.
     *
     * The default implementation returns true (assumes device is ok).
     * Drivers should override this to provide actual health checking
     * such as USB presence detection, ping/heartbeat, or API service checks.
     *
     * \return true if device appears responsive, false otherwise
     */
    virtual bool isResponsive(void) const;

    /*!
     * Get detailed device health status.
     * Provides more information than isResponsive() for debugging
     * and monitoring purposes.
     *
     * The default implementation returns {true, "unknown", "", 0}.
     *
     * Possible state values:
     * - "ok": Device is functioning normally
     * - "disconnected": Device has been disconnected (USB unplug, network loss)
     * - "error": Device is in an error state (firmware crash, API failure)
     * - "unknown": Health status cannot be determined (driver doesn't support)
     *
     * \return HealthStatus structure with device health information
     */
    virtual HealthStatus getHealth(void) const;

    /*******************************************************************
     * Channels API
     ******************************************************************/

    /*!
     * Set the frontend mapping of available DSP units to RF frontends.
     * This mapping controls channel mapping and channel availability.
     * \param direction the channel direction RX or TX
     * \param mapping a vendor-specific mapping string
     */
    virtual void setFrontendMapping(const int direction, const std::string &mapping);

    /*!
     * Get the mapping configuration string.
     * \param direction the channel direction RX or TX
     * \return the vendor-specific mapping string
     */
    virtual std::string getFrontendMapping(const int direction) const;

    /*!
     * Get a number of channels given the streaming direction
     */
    virtual size_t getNumChannels(const int direction) const;

    /*!
     * Query a dictionary of available channel information.
     * This dictionary can any number of values like
     * decoder type, version, available functions...
     * This information can be displayed to the user
     * to help identify the instantiated channel.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return channel information
     */
    virtual Kwargs getChannelInfo(const int direction, const size_t channel) const;

    /*!
     * Find out if the specified channel is full or half duplex.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true for full duplex, false for half duplex
     */
    virtual bool getFullDuplex(const int direction, const size_t channel) const;

    /*******************************************************************
     * Stream API
     ******************************************************************/

    /*!
     * Query a list of the available stream formats.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of allowed format strings. See setupStream() for the format syntax.
     */
    virtual std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;

    /*!
     * Get the hardware's native stream format for this channel.
     * This is the format used by the underlying transport layer,
     * and the direct buffer access API calls (when available).
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param [out] fullScale the maximum possible value
     * \return the native stream buffer format string
     */
    virtual std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;

    /*!
     * Query the argument info description for stream args.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of argument info structures
     */
    virtual ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;

    /*!
     * Initialize a stream given a list of channels and stream arguments.
     * The implementation may change switches or power-up components.
     * All stream API calls should be usable with the new stream object
     * after setupStream() is complete, regardless of the activity state.
     *
     * The API allows any number of simultaneous TX and RX streams, but many dual-channel
     * devices are limited to one stream in each direction, using either one or both channels.
     * This call will throw an exception if an unsupported combination is requested,
     * or if a requested channel in this direction is already in use by another stream.
     *
     * When multiple channels are added to a stream, they are typically expected to have
     * the same sample rate. See setSampleRate().
     *
     * \param direction the channel direction (`SOAPY_SDR_RX` or `SOAPY_SDR_TX`)
     * \param format A string representing the desired buffer format in `readStream()` / `writeStream()`.
     * \parblock
     *
     * The first character selects the number type:
     *   - "C" means complex
     *   - "F" means floating point
     *   - "S" means signed integer
     *   - "U" means unsigned integer
     *
     * The type character is followed by the number of bits per number (complex is 2x this size per sample)
     *
     *  Example format strings:
     *   - "CF32" -  complex float32 (8 bytes per element)
     *   - "CS16" -  complex int16 (4 bytes per element)
     *   - "CS12" -  complex int12 (3 bytes per element)
     *   - "CS4" -  complex int4 (1 byte per element)
     *   - "S32" -  int32 (4 bytes per element)
     *   - "U8" -  uint8 (1 byte per element)
     *
     * \endparblock
     * \param channels a list of channels or empty for automatic.
     * \param args stream args or empty for defaults.
     * \parblock
     *
     * Recommended keys to use in the args dictionary:
     *  - "WIRE" - format of the samples between device and host
     * \endparblock
     * \return an opaque pointer to a stream handle.
     * \parblock
     *
     * The returned stream is not required to have internal locking, and may not be used
     * concurrently from multiple threads.
     * \endparblock
     */
    virtual Stream *setupStream(
        const int direction,
        const std::string &format,
        const std::vector<size_t> &channels = std::vector<size_t>(),
        const Kwargs &args = Kwargs());

    /*!
     * Close an open stream created by setupStream
     * The implementation may change switches or power-down components.
     * \param stream the opaque pointer to a stream handle
     */
    virtual void closeStream(Stream *stream);

    /*!
     * Get the stream's maximum transmission unit (MTU) in number of elements.
     * The MTU specifies the maximum payload transfer in a stream operation.
     * This value can be used as a stream buffer allocation size that can
     * best optimize throughput given the underlying stream implementation.
     *
     * \param stream the opaque pointer to a stream handle
     * \return the MTU in number of stream elements (never zero)
     */
    virtual size_t getStreamMTU(Stream *stream) const;

    /*!
     * Activate a stream.
     * Call activate to prepare a stream before using read/write().
     * The implementation control switches or stimulate data flow.
     *
     * The timeNs is only valid when the flags have SOAPY_SDR_HAS_TIME.
     * The numElems count can be used to request a finite burst size.
     * The SOAPY_SDR_END_BURST flag can signal end on the finite burst.
     * Not all implementations will support the full range of options.
     * In this case, the implementation returns SOAPY_SDR_NOT_SUPPORTED.
     *
     * \param stream the opaque pointer to a stream handle
     * \param flags optional flag indicators about the stream
     * \param timeNs optional activation time in nanoseconds
     * \param numElems optional element count for burst control
     * \return 0 for success or error code on failure
     */
    virtual int activateStream(
        Stream *stream,
        const int flags = 0,
        const long long timeNs = 0,
        const size_t numElems = 0);

    /*!
     * Deactivate a stream.
     * Call deactivate when not using using read/write().
     * The implementation control switches or halt data flow.
     *
     * The timeNs is only valid when the flags have SOAPY_SDR_HAS_TIME.
     * Not all implementations will support the full range of options.
     * In this case, the implementation returns SOAPY_SDR_NOT_SUPPORTED.
     *
     * \param stream the opaque pointer to a stream handle
     * \param flags optional flag indicators about the stream
     * \param timeNs optional deactivation time in nanoseconds
     * \return 0 for success or error code on failure
     */
    virtual int deactivateStream(
        Stream *stream,
        const int flags = 0,
        const long long timeNs = 0);

    /*!
     * Read elements from a stream for reception.
     * This is a multi-channel call, and buffs should be an array of void *,
     * where each pointer will be filled with data from a different channel.
     *
     * **Client code compatibility:**
     * The readStream() call should be well defined at all times,
     * including prior to activation and after deactivation.
     * When inactive, readStream() should implement the timeout
     * specified by the caller and return SOAPY_SDR_TIMEOUT.
     *
     * **Timeout behavior specification:**
     * - timeoutUs > 0: Block for at most timeoutUs microseconds waiting for data.
     *   Returns SOAPY_SDR_TIMEOUT if no data becomes available within the timeout.
     *   May return partial data if some but not all requested samples are available.
     * - timeoutUs == 0: Non-blocking. Return immediately with available data,
     *   or SOAPY_SDR_TIMEOUT if no data is available.
     * - timeoutUs < 0: Block indefinitely until data is available.
     *   Not recommended for production code as it may block forever if device hangs.
     *
     * **Driver implementation requirements:**
     * - Drivers MUST honor the timeout parameter to prevent indefinite blocking.
     * - If a driver cannot implement timeout (e.g., due to hardware limitations),
     *   it should document this limitation clearly.
     * - On timeout with no data: return SOAPY_SDR_TIMEOUT.
     * - On timeout with partial data: return the number of samples read (> 0).
     *
     * \param stream the opaque pointer to a stream handle
     * \param buffs an array of void* buffers num chans in size
     * \param numElems the number of elements in each buffer
     * \param flags optional flag indicators about the result
     * \param timeNs the buffer's timestamp in nanoseconds
     * \param timeoutUs the timeout in microseconds (see timeout behavior above)
     * \return the number of elements read per buffer or error code
     *         (SOAPY_SDR_TIMEOUT, SOAPY_SDR_OVERFLOW, SOAPY_SDR_STREAM_ERROR, etc.)
     */
    virtual int readStream(
        Stream *stream,
        void * const *buffs,
        const size_t numElems,
        int &flags,
        long long &timeNs,
        const long timeoutUs = 100000);

    /*!
     * Write elements to a stream for transmission.
     * This is a multi-channel call, and buffs should be an array of void *,
     * where each pointer will be filled with data for a different channel.
     *
     * **Client code compatibility:**
     * Client code relies on writeStream() for proper back-pressure.
     * The writeStream() implementation must enforce the timeout
     * such that the call blocks until space becomes available
     * or timeout expiration.
     *
     * **Timeout behavior specification:**
     * - timeoutUs > 0: Block for at most timeoutUs microseconds waiting for buffer space.
     *   Returns SOAPY_SDR_TIMEOUT if buffer space doesn't become available.
     * - timeoutUs == 0: Non-blocking. Return immediately with what could be written,
     *   or SOAPY_SDR_TIMEOUT if no buffer space is available.
     * - timeoutUs < 0: Block indefinitely until buffer space is available.
     *
     * \param stream the opaque pointer to a stream handle
     * \param buffs an array of void* buffers num chans in size
     * \param numElems the number of elements in each buffer
     * \param flags optional input flags and output flags
     * \param timeNs the buffer's timestamp in nanoseconds
     * \param timeoutUs the timeout in microseconds (see timeout behavior above)
     * \return the number of elements written per buffer or error code
     *         (SOAPY_SDR_TIMEOUT, SOAPY_SDR_UNDERFLOW, SOAPY_SDR_STREAM_ERROR, etc.)
     */
    virtual int writeStream(
        Stream *stream,
        const void * const *buffs,
        const size_t numElems,
        int &flags,
        const long long timeNs = 0,
        const long timeoutUs = 100000);

    /*!
     * Readback status information about a stream.
     * This call is typically used on a transmit stream
     * to report time errors, underflows, and burst completion.
     *
     * **Client code compatibility:**
     * Client code may continually poll readStreamStatus() in a loop.
     * Implementations of readStreamStatus() should wait in the call
     * for a status change event or until the timeout expiration.
     * When stream status is not implemented on a particular stream,
     * readStreamStatus() should return SOAPY_SDR_NOT_SUPPORTED.
     * Client code may use this indication to disable a polling loop.
     *
     * \param stream the opaque pointer to a stream handle
     * \param chanMask to which channels this status applies
     * \param flags optional input flags and output flags
     * \param timeNs the buffer's timestamp in nanoseconds
     * \param timeoutUs the timeout in microseconds
     * \return 0 for success or error code like timeout
     */
    virtual int readStreamStatus(
        Stream *stream,
        size_t &chanMask,
        int &flags,
        long long &timeNs,
        const long timeoutUs = 100000);

    /*******************************************************************
     * Overflow Recovery API
     ******************************************************************/

    /*!
     * Overflow recovery behavior enumeration.
     * Indicates what action is required when an overflow occurs.
     */
    enum OverflowRecovery
    {
        OVERFLOW_AUTO_RECOVER,      //!< Driver handles it, just continue reading
        OVERFLOW_RESET_REQUIRED,    //!< Call resetStream() to recover
        OVERFLOW_RESTART_REQUIRED,  //!< Full deactivateStream()/activateStream() cycle needed
        OVERFLOW_UNKNOWN            //!< Legacy/not specified (treat as AUTO_RECOVER)
    };

    /*!
     * Get the overflow recovery behavior for this stream.
     * Applications can use this to determine what action to take
     * when an overflow (SOAPY_SDR_OVERFLOW) is detected.
     *
     * \param stream the opaque pointer to a stream handle
     * \return the overflow recovery behavior for this stream
     */
    virtual OverflowRecovery getOverflowRecovery(Stream *stream) const;

    /*!
     * Reset stream state after an overflow or error condition.
     * This is a lightweight recovery mechanism that clears internal
     * buffers and resets stream state without the overhead of a full
     * deactivate/activate cycle.
     *
     * Call this method when:
     * - readStream() returns SOAPY_SDR_OVERFLOW and getOverflowRecovery()
     *   returns OVERFLOW_RESET_REQUIRED
     * - You want to clear stale data from buffers
     * - You need to resynchronize after a processing delay
     *
     * The default implementation returns SOAPY_SDR_NOT_SUPPORTED,
     * indicating that a full deactivate/activate cycle is needed.
     *
     * \param stream the opaque pointer to a stream handle
     * \return 0 for success or SOAPY_SDR_NOT_SUPPORTED if not implemented
     */
    virtual int resetStream(Stream *stream);

    /*******************************************************************
     * Stream Statistics API
     ******************************************************************/

    /*!
     * Structure containing streaming statistics.
     */
    struct StreamStats
    {
        uint64_t samplesRead;       //!< Total samples read since activation
        uint64_t samplesWritten;    //!< Total samples written since activation
        uint64_t samplesDropped;    //!< Samples lost due to overflow or other issues
        uint32_t overflowCount;     //!< Number of overflow events
        uint32_t underflowCount;    //!< Number of underflow events
        uint32_t errorCount;        //!< Number of other errors
        double effectiveSampleRate; //!< Measured actual sample rate (0 if unknown)
        double streamActiveTime;    //!< Seconds since activateStream() (0 if unknown)
    };

    /*!
     * Get streaming statistics for the given stream.
     * This provides insight into stream health including sample counts,
     * overflow/underflow events, and effective sample rate.
     *
     * The default implementation returns all zeros (no stats tracked).
     *
     * Statistics should be cheap to query (no blocking).
     * Counters use 64-bit types to avoid overflow during long-running streams.
     *
     * \param stream the opaque pointer to a stream handle
     * \return StreamStats structure with current statistics
     */
    virtual StreamStats getStreamStats(Stream *stream) const;

    /*!
     * Reset streaming statistics for the given stream.
     * Clears all counters back to zero. Call this to start fresh
     * measurements without restarting the stream.
     *
     * The default implementation does nothing.
     *
     * \param stream the opaque pointer to a stream handle
     */
    virtual void resetStreamStats(Stream *stream);

    /*******************************************************************
     * Direct buffer access API
     ******************************************************************/

    /*!
     * How many direct access buffers can the stream provide?
     * This is the number of times the user can call acquire()
     * on a stream without making subsequent calls to release().
     * A return value of 0 means that direct access is not supported.
     *
     * \param stream the opaque pointer to a stream handle
     * \return the number of direct access buffers or 0
     */
    virtual size_t getNumDirectAccessBuffers(Stream *stream);

    /*!
     * Get the buffer addresses for a scatter/gather table entry.
     * When the underlying DMA implementation uses scatter/gather
     * then this call provides the user addresses for that table.
     *
     * Example: The caller may query the DMA memory addresses once
     * after stream creation to pre-allocate a re-usable ring-buffer.
     *
     * \param stream the opaque pointer to a stream handle
     * \param handle an index value between 0 and num direct buffers - 1
     * \param buffs an array of void* buffers num chans in size
     * \return 0 for success or error code when not supported
     */
    virtual int getDirectAccessBufferAddrs(Stream *stream, const size_t handle, void **buffs);

    /*!
     * Acquire direct buffers from a receive stream.
     * This call is part of the direct buffer access API.
     *
     * The buffs array will be filled with a stream pointer for each channel.
     * Each pointer can be read up to the number of return value elements.
     *
     * The handle will be set by the implementation so that the caller
     * may later release access to the buffers with releaseReadBuffer().
     * Handle represents an index into the internal scatter/gather table
     * such that handle is between 0 and num direct buffers - 1.
     *
     * \param stream the opaque pointer to a stream handle
     * \param handle an index value used in the release() call
     * \param buffs an array of void* buffers num chans in size
     * \param flags optional flag indicators about the result
     * \param timeNs the buffer's timestamp in nanoseconds
     * \param timeoutUs the timeout in microseconds
     * \return the number of elements read per buffer or error code
     */
    virtual int acquireReadBuffer(
        Stream *stream,
        size_t &handle,
        const void **buffs,
        int &flags,
        long long &timeNs,
        const long timeoutUs = 100000);

    /*!
     * Release an acquired buffer back to the receive stream.
     * This call is part of the direct buffer access API.
     *
     * \param stream the opaque pointer to a stream handle
     * \param handle the opaque handle from the acquire() call
     */
    virtual void releaseReadBuffer(
        Stream *stream,
        const size_t handle);

    /*!
     * Acquire direct buffers from a transmit stream.
     * This call is part of the direct buffer access API.
     *
     * The buffs array will be filled with a stream pointer for each channel.
     * Each pointer can be written up to the number of return value elements.
     *
     * The handle will be set by the implementation so that the caller
     * may later release access to the buffers with releaseWriteBuffer().
     * Handle represents an index into the internal scatter/gather table
     * such that handle is between 0 and num direct buffers - 1.
     *
     * \param stream the opaque pointer to a stream handle
     * \param handle an index value used in the release() call
     * \param buffs an array of void* buffers num chans in size
     * \param timeoutUs the timeout in microseconds
     * \return the number of available elements per buffer or error
     */
    virtual int acquireWriteBuffer(
        Stream *stream,
        size_t &handle,
        void **buffs,
        const long timeoutUs = 100000);

    /*!
     * Release an acquired buffer back to the transmit stream.
     * This call is part of the direct buffer access API.
     *
     * Stream meta-data is provided as part of the release call,
     * and not the acquire call so that the caller may acquire
     * buffers without committing to the contents of the meta-data,
     * which can be determined by the user as the buffers are filled.
     *
     * \param stream the opaque pointer to a stream handle
     * \param handle the opaque handle from the acquire() call
     * \param numElems the number of elements written to each buffer
     * \param flags optional input flags and output flags
     * \param timeNs the buffer's timestamp in nanoseconds
     */
    virtual void releaseWriteBuffer(
        Stream *stream,
        const size_t handle,
        const size_t numElems,
        int &flags,
        const long long timeNs = 0);

    /*******************************************************************
     * Antenna API
     ******************************************************************/

    /*!
     * Get a list of available antennas to select on a given chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of available antenna names
     */
    virtual std::vector<std::string> listAntennas(const int direction, const size_t channel) const;

    /*!
     * Set the selected antenna on a chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of an available antenna
     */
    virtual void setAntenna(const int direction, const size_t channel, const std::string &name);

    /*!
     * Set the selected antenna on a chain with persistence option.
     * When persistent is true, the driver will re-apply this antenna setting
     * after operations that might reset it (like activateStream).
     *
     * This addresses a common issue where antenna settings are reset by
     * some drivers during stream activation.
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of an available antenna
     * \param persistent if true, automatically re-apply after stream activation
     */
    virtual void setAntennaPersistent(const int direction, const size_t channel, const std::string &name, const bool persistent = true);

    /*!
     * Get the selected antenna on a chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the name of an available antenna
     */
    virtual std::string getAntenna(const int direction, const size_t channel) const;

    /*!
     * Check if antenna persistence is enabled for a channel.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true if antenna persistence is enabled
     */
    virtual bool getAntennaPersistent(const int direction, const size_t channel) const;

    /*******************************************************************
     * Frontend corrections API
     ******************************************************************/

    /*!
     * Does the device support automatic DC offset corrections?
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true if automatic corrections are supported
     */
    virtual bool hasDCOffsetMode(const int direction, const size_t channel) const;

    /*!
     * Set the automatic DC offset corrections mode.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param automatic true for automatic offset correction
     */
    virtual void setDCOffsetMode(const int direction, const size_t channel, const bool automatic);

    /*!
     * Get the automatic DC offset corrections mode.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true for automatic offset correction
     */
    virtual bool getDCOffsetMode(const int direction, const size_t channel) const;

    /*!
     * Does the device support frontend DC offset correction?
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true if DC offset corrections are supported
     */
    virtual bool hasDCOffset(const int direction, const size_t channel) const;

    /*!
     * Set the frontend DC offset correction.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param offset the relative correction (1.0 max)
     */
    virtual void setDCOffset(const int direction, const size_t channel, const std::complex<double> &offset);

    /*!
     * Get the frontend DC offset correction.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the relative correction (1.0 max)
     */
    virtual std::complex<double> getDCOffset(const int direction, const size_t channel) const;

    /*!
     * Does the device support frontend IQ balance correction?
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true if IQ balance corrections are supported
     */
    virtual bool hasIQBalance(const int direction, const size_t channel) const;

    /*!
     * Set the frontend IQ balance correction.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param balance the relative correction (1.0 max)
     */
    virtual void setIQBalance(const int direction, const size_t channel, const std::complex<double> &balance);

    /*!
     * Get the frontend IQ balance correction.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the relative correction (1.0 max)
     */
    virtual std::complex<double> getIQBalance(const int direction, const size_t channel) const;

    /*!
     * Does the device support automatic frontend IQ balance correction?
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true if IQ balance corrections are supported
     */
    virtual bool hasIQBalanceMode(const int direction, const size_t channel) const;

    /*!
     * Set the automatic frontend IQ balance correction.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param automatic true for automatic correction
     */
    virtual void setIQBalanceMode(const int direction, const size_t channel, const bool automatic);

    /*!
     * Set the automatic IQ balance corrections mode.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true for automatic correction
     */
    virtual bool getIQBalanceMode(const int direction, const size_t channel) const;

    /*!
     * Does the device support frontend frequency correction?
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true if frequency corrections are supported
     */
    virtual bool hasFrequencyCorrection(const int direction, const size_t channel) const;

    /*!
     * Fine tune the frontend frequency correction.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param value the correction in PPM
     */
    virtual void setFrequencyCorrection(const int direction, const size_t channel, const double value);

    /*!
     * Get the frontend frequency correction value.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the correction value in PPM
     */
    virtual double getFrequencyCorrection(const int direction, const size_t channel) const;

    /*******************************************************************
     * Gain API
     ******************************************************************/

    /*!
     * List available amplification elements.
     * Elements should be in order RF to baseband.
     * \param direction the channel direction RX or TX
     * \param channel an available channel
     * \return a list of gain string names
     */
    virtual std::vector<std::string> listGains(const int direction, const size_t channel) const;

    /*!
     * Does the device support automatic gain control?
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true for automatic gain control
     */
    virtual bool hasGainMode(const int direction, const size_t channel) const;

    /*!
     * Set the automatic gain mode on the chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param automatic true for automatic gain setting
     */
    virtual void setGainMode(const int direction, const size_t channel, const bool automatic);

    /*!
     * Get the automatic gain mode on the chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return true for automatic gain setting
     */
    virtual bool getGainMode(const int direction, const size_t channel) const;

    /*!
     * Set the overall amplification in a chain.
     * The gain will be distributed automatically across available element.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param value the new amplification value in dB
     */
    virtual void setGain(const int direction, const size_t channel, const double value);

    /*!
     * Set the value of a amplification element in a chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of an amplification element
     * \param value the new amplification value in dB
     */
    virtual void setGain(const int direction, const size_t channel, const std::string &name, const double value);

    /*!
     * Get the overall value of the gain elements in a chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the value of the gain in dB
     */
    virtual double getGain(const int direction, const size_t channel) const;

    /*!
     * Get the value of an individual amplification element in a chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of an amplification element
     * \return the value of the gain in dB
     */
    virtual double getGain(const int direction, const size_t channel, const std::string &name) const;

    /*!
     * Get the overall range of possible gain values.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of gain ranges in dB
     */
    virtual Range getGainRange(const int direction, const size_t channel) const;

    /*!
     * Get the range of possible gain values for a specific element.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of an amplification element
     * \return a list of gain ranges in dB
     */
    virtual Range getGainRange(const int direction, const size_t channel, const std::string &name) const;

    /*******************************************************************
     * Frequency API
     ******************************************************************/

    /*!
     * Set the center frequency of the chain.
     *  - For RX, this specifies the down-conversion frequency.
     *  - For TX, this specifies the up-conversion frequency.
     *
     * The default implementation of setFrequency() will tune the "RF"
     * component as close as possible to the requested center frequency.
     * Tuning inaccuracies will be compensated for with the "BB" component.
     *
     * The args can be used to augment the tuning algorithm.
     *  - Use "OFFSET" to specify an "RF" tuning offset,
     *    usually with the intention of moving the LO out of the passband.
     *    The offset will be compensated for using the "BB" component.
     *  - Use the name of a component for the key and a frequency in Hz
     *    as the value (any format) to enforce a specific frequency.
     *    The other components will be tuned with compensation
     *    to achieve the specified overall frequency.
     *  - Use the name of a component for the key and the value "IGNORE"
     *    so that the tuning algorithm will avoid altering the component.
     *  - Vendor specific implementations can also use the same args to augment
     *    tuning in other ways such as specifying fractional vs integer N tuning.
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param frequency the center frequency in Hz
     * \param args optional tuner arguments
     */
    virtual void setFrequency(const int direction, const size_t channel, const double frequency, const Kwargs &args = Kwargs());

    /*!
     * Tune the center frequency of the specified element.
     *  - For RX, this specifies the down-conversion frequency.
     *  - For TX, this specifies the up-conversion frequency.
     *
     * Recommended names used to represent tunable components:
     *  - "CORR" - freq error correction in PPM
     *  - "RF" - frequency of the RF frontend
     *  - "BB" - frequency of the baseband DSP
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of a tunable element
     * \param frequency the center frequency in Hz
     * \param args optional tuner arguments
     */
    virtual void setFrequency(const int direction, const size_t channel, const std::string &name, const double frequency, const Kwargs &args = Kwargs());

    /*!
     * Get the overall center frequency of the chain.
     *  - For RX, this specifies the down-conversion frequency.
     *  - For TX, this specifies the up-conversion frequency.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the center frequency in Hz
     */
    virtual double getFrequency(const int direction, const size_t channel) const;

    /*!
     * Get the frequency of a tunable element in the chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of a tunable element
     * \return the tunable element's frequency in Hz
     */
    virtual double getFrequency(const int direction, const size_t channel, const std::string &name) const;

    /*!
     * List available tunable elements in the chain.
     * Elements should be in order RF to baseband.
     * \param direction the channel direction RX or TX
     * \param channel an available channel
     * \return a list of tunable elements by name
     */
    virtual std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;

    /*!
     * Get the range of overall frequency values.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of frequency ranges in Hz
     */
    virtual RangeList getFrequencyRange(const int direction, const size_t channel) const;

    /*!
     * Get the range of tunable values for the specified element.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param name the name of a tunable element
     * \return a list of frequency ranges in Hz
     */
    virtual RangeList getFrequencyRange(const int direction, const size_t channel, const std::string &name) const;

    /*!
     * Query the argument info description for tune args.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of argument info structures
     */
    virtual ArgInfoList getFrequencyArgsInfo(const int direction, const size_t channel) const;

    /*******************************************************************
     * Sample Rate API
     ******************************************************************/

    /*!
     * Set the baseband sample rate of the chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param rate the sample rate in samples per second
     */
    virtual void setSampleRate(const int direction, const size_t channel, const double rate);

    /*!
     * Get the baseband sample rate of the chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the sample rate in samples per second
     */
    virtual double getSampleRate(const int direction, const size_t channel) const;

    /*!
     * Get the range of possible baseband sample rates.
     * \deprecated replaced by getSampleRateRange()
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of possible rates in samples per second
     */
    virtual std::vector<double> listSampleRates(const int direction, const size_t channel) const;

    /*!
     * Get the range of possible baseband sample rates.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of sample rate ranges in samples per second
     */
    virtual RangeList getSampleRateRange(const int direction, const size_t channel) const;

    /*******************************************************************
     * Bandwidth API
     ******************************************************************/

    /*!
     * Set the baseband filter width of the chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param bw the baseband filter width in Hz
     */
    virtual void setBandwidth(const int direction, const size_t channel, const double bw);

    /*!
     * Get the baseband filter width of the chain.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return the baseband filter width in Hz
     */
    virtual double getBandwidth(const int direction, const size_t channel) const;

    /*!
     * Get the range of possible baseband filter widths.
     * \deprecated replaced by getBandwidthRange()
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of possible bandwidths in Hz
     */
    virtual std::vector<double> listBandwidths(const int direction, const size_t channel) const;

    /*!
     * Get the range of possible baseband filter widths.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of bandwidth ranges in Hz
     */
    virtual RangeList getBandwidthRange(const int direction, const size_t channel) const;

    /*******************************************************************
     * Clocking API
     ******************************************************************/

    /*!
     * Set the master clock rate of the device.
     * \param rate the clock rate in Hz
     */
    virtual void setMasterClockRate(const double rate);

    /*!
     * Get the master clock rate of the device.
     * \return the clock rate in Hz
     */
    virtual double getMasterClockRate(void) const;

    /*!
     * Get the range of available master clock rates.
     * \return a list of clock rate ranges in Hz
     */
    virtual RangeList getMasterClockRates(void) const;

    /*!
     * Set the reference clock rate of the device.
     * \param rate the clock rate in Hz
     */
    virtual void setReferenceClockRate(const double rate);

    /*!
     * Get the reference clock rate of the device.
     * \return the clock rate in Hz
     */
    virtual double getReferenceClockRate(void) const;

    /*!
     * Get the range of available reference clock rates.
     * \return a list of clock rate ranges in Hz
     */
    virtual RangeList getReferenceClockRates(void) const;

    /*!
     * Get the list of available clock sources.
     * \return a list of clock source names
     */
    virtual std::vector<std::string> listClockSources(void) const;

    /*!
     * Set the clock source on the device
     * \param source the name of a clock source
     */
    virtual void setClockSource(const std::string &source);

    /*!
     * Get the clock source of the device
     * \return the name of a clock source
     */
    virtual std::string getClockSource(void) const;

    /*******************************************************************
     * Time API
     ******************************************************************/

    /*!
     * Get the list of available time sources.
     * \return a list of time source names
     */
    virtual std::vector<std::string> listTimeSources(void) const;

    /*!
     * Set the time source on the device
     * \param source the name of a time source
     */
    virtual void setTimeSource(const std::string &source);

    /*!
     * Get the time source of the device
     * \return the name of a time source
     */
    virtual std::string getTimeSource(void) const;

    /*!
     * Does this device have a hardware clock?
     * \param what optional argument
     * \return true if the hardware clock exists
     */
    virtual bool hasHardwareTime(const std::string &what = "") const;

    /*!
     * Read the time from the hardware clock on the device.
     * The what argument can refer to a specific time counter.
     * \param what optional argument
     * \return the time in nanoseconds
     */
    virtual long long getHardwareTime(const std::string &what = "") const;

    /*!
     * Write the time to the hardware clock on the device.
     * The what argument can refer to a specific time counter.
     * \param timeNs time in nanoseconds
     * \param what optional argument
     */
    virtual void setHardwareTime(const long long timeNs, const std::string &what = "");

    /*!
     * Set the time of subsequent configuration calls.
     * The what argument can refer to a specific command queue.
     * Implementations may use a time of 0 to clear.
     * \deprecated replaced by setHardwareTime()
     * \param timeNs time in nanoseconds
     * \param what optional argument
     */
    virtual void setCommandTime(const long long timeNs, const std::string &what = "");

    /*******************************************************************
     * Sensor API
     ******************************************************************/

    /*!
     * List the available global readback sensors.
     * A sensor can represent a reference lock, RSSI, temperature.
     * \return a list of available sensor string names
     */
    virtual std::vector<std::string> listSensors(void) const;

    /*!
     * Get meta-information about a sensor.
     * Example: displayable name, type, range.
     * \param key the ID name of an available sensor
     * \return meta-information about a sensor
     */
    virtual ArgInfo getSensorInfo(const std::string &key) const;

    /*!
     * Readback a global sensor given the name.
     * The value returned is a string which can represent
     * a boolean ("true"/"false"), an integer, or float.
     * \param key the ID name of an available sensor
     * \return the current value of the sensor
     */
    virtual std::string readSensor(const std::string &key) const;

    /*!
     * Readback a global sensor given the name.
     * \tparam Type the return type for the sensor value
     * \param key the ID name of an available sensor
     * \return the current value of the sensor as the specified type
     */
    template <typename Type>
    Type readSensor(const std::string &key) const;

    /*!
     * List the available channel readback sensors.
     * A sensor can represent a reference lock, RSSI, temperature.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of available sensor string names
     */
    virtual std::vector<std::string> listSensors(const int direction, const size_t channel) const;

    /*!
     * Get meta-information about a channel sensor.
     * Example: displayable name, type, range.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the ID name of an available sensor
     * \return meta-information about a sensor
     */
    virtual ArgInfo getSensorInfo(const int direction, const size_t channel, const std::string &key) const;

    /*!
     * Readback a channel sensor given the name.
     * The value returned is a string which can represent
     * a boolean ("true"/"false"), an integer, or float.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the ID name of an available sensor
     * \return the current value of the sensor
     */
    virtual std::string readSensor(const int direction, const size_t channel, const std::string &key) const;

    /*!
     * Readback a channel sensor given the name.
     * \tparam Type the return type for the sensor value
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the ID name of an available sensor
     * \return the current value of the sensor as the specified type
     */
    template <typename Type>
    Type readSensor(const int direction, const size_t channel, const std::string &key) const;

    /*******************************************************************
     * Register API
     ******************************************************************/

    /*!
     * Get a list of available register interfaces by name.
     * \return a list of available register interfaces
     */
    virtual std::vector<std::string> listRegisterInterfaces(void) const;

    /*!
     * Write a register on the device given the interface name.
     * This can represent a register on a soft CPU, FPGA, IC;
     * the interpretation is up the implementation to decide.
     * \param name the name of a available register interface
     * \param addr the register address
     * \param value the register value
     */
    virtual void writeRegister(const std::string &name, const unsigned addr, const unsigned value);

    /*!
     * Read a register on the device given the interface name.
     * \param name the name of a available register interface
     * \param addr the register address
     * \return the register value
     */
    virtual unsigned readRegister(const std::string &name, const unsigned addr) const;

    /*!
     * Write a register on the device.
     * This can represent a register on a soft CPU, FPGA, IC;
     * the interpretation is up the implementation to decide.
     * \deprecated replaced by writeRegister(name)
     * \param addr the register address
     * \param value the register value
     */
    virtual void writeRegister(const unsigned addr, const unsigned value);

    /*!
     * Read a register on the device.
     * \deprecated replaced by readRegister(name)
     * \param addr the register address
     * \return the register value
     */
    virtual unsigned readRegister(const unsigned addr) const;

    /*!
     * Write a memory block on the device given the interface name.
     * This can represent a memory block on a soft CPU, FPGA, IC;
     * the interpretation is up the implementation to decide.
     * \param name the name of a available memory block interface
     * \param addr the memory block start address
     * \param value the memory block content
     */
    virtual void writeRegisters(const std::string &name, const unsigned addr, const std::vector<unsigned> &value);

    /*!
     * Read a memory block on the device given the interface name.
     * \param name the name of a available memory block interface
     * \param addr the memory block start address
     * \param length number of words to be read from memory block
     * \return the memory block content
     */
    virtual std::vector<unsigned> readRegisters(const std::string &name, const unsigned addr, const size_t length) const;

    /*******************************************************************
     * Settings API
     ******************************************************************/

    /*!
     * Describe the allowed keys and values used for settings.
     * \return a list of argument info structures
     */
    virtual ArgInfoList getSettingInfo(void) const;

    /*!
     * Get information on a specific setting.
     * \param key the setting identifier
     * \return all information for a specific setting
     */
    virtual ArgInfo getSettingInfo(const std::string &key) const;

    /*!
     * Write an arbitrary setting on the device.
     * The interpretation is up the implementation.
     * \param key the setting identifier
     * \param value the setting value
     */
    virtual void writeSetting(const std::string &key, const std::string &value);

    /*!
     * Write a setting with an arbitrary value type.
     * \tparam Type the data type of the value
     * \param key the setting identifier
     * \param value the setting value
     */
    template <typename Type>
    void writeSetting(const std::string &key, const Type &value);

    /*!
     * Read an arbitrary setting on the device.
     * \param key the setting identifier
     * \return the setting value
     */
    virtual std::string readSetting(const std::string &key) const;

    /*!
     * Read an arbitrary setting on the device.
     * \tparam Type the return type for the sensor value
     * \param key the setting identifier
     * \return the setting value
     */
    template <typename Type>
    Type readSetting(const std::string &key) const;

    /*!
     * Describe the allowed keys and values used for channel settings.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \return a list of argument info structures
     */
    virtual ArgInfoList getSettingInfo(const int direction, const size_t channel) const;

    /*!
     * Get information on a specific channel setting.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the setting identifier
     * \return all information for a specific channel setting
     */
    virtual ArgInfo getSettingInfo(const int direction, const size_t channnel, const std::string &key) const;

    /*!
     * Write an arbitrary channel setting on the device.
     * The interpretation is up the implementation.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the setting identifier
     * \param value the setting value
     */
    virtual void writeSetting(const int direction, const size_t channel, const std::string &key, const std::string &value);

    /*!
     * Write an arbitrary channel setting on the device.
     * \tparam Type the data type of the value
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the setting identifier
     * \param value the setting value
     */
    template <typename Type>
    void writeSetting(const int direction, const size_t channel, const std::string &key, const Type &value);

    /*!
     * Read an arbitrary channel setting on the device.
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the setting identifier
     * \return the setting value
     */
    virtual std::string readSetting(const int direction, const size_t channel, const std::string &key) const;

    /*!
     * Read an arbitrary channel setting on the device.
     * \tparam Type the return type for the sensor value
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the setting identifier
     * \return the setting value
     */
    template <typename Type>
    Type readSetting(const int direction, const size_t channel, const std::string &key) const;

    /*******************************************************************
     * Setting Verification API
     ******************************************************************/

    /*!
     * Structure containing setting verification result.
     */
    struct SettingVerification
    {
        bool success;           //!< True if setting was applied correctly
        std::string requested;  //!< The value that was requested
        std::string actual;     //!< The actual value read back
        std::string message;    //!< Human-readable message about the verification
    };

    /*!
     * Structure containing setting validation result (pre-apply check).
     */
    struct SettingValidation
    {
        bool valid;                 //!< True if the value would be accepted
        std::string normalizedValue;//!< The value after normalization (e.g., clamping)
        std::string message;        //!< Human-readable validation message
        std::vector<std::string> allowedValues; //!< List of allowed values (for enum types)
        Range allowedRange;         //!< Allowed range (for numeric types)
    };

    /*!
     * Validate a setting value before applying it.
     * This allows applications to check if a value is valid without
     * actually changing the device state.
     *
     * \param key the setting identifier
     * \param value the proposed setting value
     * \return SettingValidation with validity status and constraints
     */
    virtual SettingValidation validateSetting(const std::string &key, const std::string &value) const;

    /*!
     * Validate a channel setting value before applying it.
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the setting identifier
     * \param value the proposed setting value
     * \return SettingValidation with validity status and constraints
     */
    virtual SettingValidation validateSetting(const int direction, const size_t channel, const std::string &key, const std::string &value) const;

    /*!
     * Write and verify a global setting.
     * Writes the setting then reads it back to verify it was applied.
     *
     * \param key the setting identifier
     * \param value the setting value
     * \return SettingVerification with success status and actual value
     */
    virtual SettingVerification writeSettingVerified(const std::string &key, const std::string &value);

    /*!
     * Write and verify a channel setting.
     * Writes the setting then reads it back to verify it was applied.
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param key the setting identifier
     * \param value the setting value
     * \return SettingVerification with success status and actual value
     */
    virtual SettingVerification writeSettingVerified(const int direction, const size_t channel, const std::string &key, const std::string &value);

    /*!
     * Set frequency with verification.
     * Sets the frequency then reads it back to verify within tolerance.
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param frequency the center frequency in Hz
     * \param args optional tuning arguments
     * \param toleranceHz acceptable difference between requested and actual (default 1 Hz)
     * \return SettingVerification with success status, requested and actual frequency
     */
    virtual SettingVerification setFrequencyVerified(
        const int direction,
        const size_t channel,
        const double frequency,
        const Kwargs &args = Kwargs(),
        const double toleranceHz = 1.0);

    /*!
     * Set sample rate with verification.
     * Sets the sample rate then reads it back to verify within tolerance.
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param rate the sample rate in samples per second
     * \param tolerancePercent acceptable percentage difference (default 0.01 = 1%)
     * \return SettingVerification with success status, requested and actual rate
     */
    virtual SettingVerification setSampleRateVerified(
        const int direction,
        const size_t channel,
        const double rate,
        const double tolerancePercent = 0.01);

    /*!
     * Set gain with verification.
     * Sets the gain then reads it back to verify within tolerance.
     *
     * \param direction the channel direction RX or TX
     * \param channel an available channel on the device
     * \param gain the gain value in dB
     * \param tolerancedB acceptable difference in dB (default 0.1)
     * \return SettingVerification with success status, requested and actual gain
     */
    virtual SettingVerification setGainVerified(
        const int direction,
        const size_t channel,
        const double gain,
        const double tolerancedB = 0.1);

    /*******************************************************************
     * GPIO API
     ******************************************************************/

    /*!
     * Get a list of available GPIO banks by name.
     */
    virtual std::vector<std::string> listGPIOBanks(void) const;

    /*!
     * Write the value of a GPIO bank.
     * \param bank the name of an available bank
     * \param value an integer representing GPIO bits
     */
    virtual void writeGPIO(const std::string &bank, const unsigned value);

    /*!
     * Write the value of a GPIO bank with modification mask.
     * \param bank the name of an available bank
     * \param value an integer representing GPIO bits
     * \param mask a modification mask where 1 = modify
     */
    virtual void writeGPIO(const std::string &bank, const unsigned value, const unsigned mask);

    /*!
     * Readback the value of a GPIO bank.
     * \param bank the name of an available bank
     * \return an integer representing GPIO bits
     */
    virtual unsigned readGPIO(const std::string &bank) const;

    /*!
     * Write the data direction of a GPIO bank.
     * 1 bits represent outputs, 0 bits represent inputs.
     * \param bank the name of an available bank
     * \param dir an integer representing data direction bits
     */
    virtual void writeGPIODir(const std::string &bank, const unsigned dir);

    /*!
     * Write the data direction of a GPIO bank with modification mask.
     * 1 bits represent outputs, 0 bits represent inputs.
     * \param bank the name of an available bank
     * \param dir an integer representing data direction bits
     * \param mask a modification mask where 1 = modify
     */
    virtual void writeGPIODir(const std::string &bank, const unsigned dir, const unsigned mask);

    /*!
     * Read the data direction of a GPIO bank.
     * 1 bits represent outputs, 0 bits represent inputs.
     * \param bank the name of an available bank
     * \return an integer representing data direction bits
     */
    virtual unsigned readGPIODir(const std::string &bank) const;

    /*******************************************************************
     * I2C API
     ******************************************************************/

    /*!
     * Write to an available I2C slave.
     * If the device contains multiple I2C masters,
     * the address bits can encode which master.
     * \param addr the address of the slave
     * \param data an array of bytes write out
     */
    virtual void writeI2C(const int addr, const std::string &data);

    /*!
     * Read from an available I2C slave.
     * If the device contains multiple I2C masters,
     * the address bits can encode which master.
     * \param addr the address of the slave
     * \param numBytes the number of bytes to read
     * \return an array of bytes read from the slave
     */
    virtual std::string readI2C(const int addr, const size_t numBytes);

    /*******************************************************************
     * SPI API
     ******************************************************************/

    /*!
     * Perform a SPI transaction and return the result.
     * Its up to the implementation to set the clock rate,
     * and read edge, and the write edge of the SPI core.
     * SPI slaves without a readback pin will return 0.
     *
     * If the device contains multiple SPI masters,
     * the address bits can encode which master.
     *
     * \param addr an address of an available SPI slave
     * \param data the SPI data, numBits-1 is first out
     * \param numBits the number of bits to clock out
     * \return the readback data, numBits-1 is first in
     */
    virtual unsigned transactSPI(const int addr, const unsigned data, const size_t numBits);

    /*******************************************************************
     * UART API
     ******************************************************************/

    /*!
     * Enumerate the available UART devices.
     * \return a list of names of available UARTs
     */
    virtual std::vector<std::string> listUARTs(void) const;

    /*!
     * Write data to a UART device.
     * Its up to the implementation to set the baud rate,
     * carriage return settings, flushing on newline.
     * \param which the name of an available UART
     * \param data an array of bytes to write out
     */
    virtual void writeUART(const std::string &which, const std::string &data);

    /*!
     * Read bytes from a UART until timeout or newline.
     * Its up to the implementation to set the baud rate,
     * carriage return settings, flushing on newline.
     * \param which the name of an available UART
     * \param timeoutUs a timeout in microseconds
     * \return an array of bytes read from the UART
     */
    virtual std::string readUART(const std::string &which, const long timeoutUs = 100000) const;

    /*******************************************************************
     * Native Access API
     ******************************************************************/

    /*!
     * A handle to the native device used by the driver.
     * The implementation may return a null value if it does not support
     * or does not wish to provide access to the native handle.
     * \return a handle to the native device or null
     */
    virtual void* getNativeDeviceHandle(void) const;
};

}

template <typename Type>
Type SoapySDR::Device::readSensor(const std::string &key) const
{
    return SoapySDR::StringToSetting<Type>(this->readSensor(key));
}

template <typename Type>
Type SoapySDR::Device::readSensor(const int direction, const size_t channel, const std::string &key) const
{
    return SoapySDR::StringToSetting<Type>(this->readSensor(direction, channel, key));
}

template <typename Type>
void SoapySDR::Device::writeSetting(const std::string &key, const Type &value)
{
    this->writeSetting(key, SoapySDR::SettingToString(value));
}

template <typename Type>
Type SoapySDR::Device::readSetting(const std::string &key) const
{
    return SoapySDR::StringToSetting<Type>(this->readSetting(key));
}

template <typename Type>
void SoapySDR::Device::writeSetting(const int direction, const size_t channel, const std::string &key, const Type &value)
{
    this->writeSetting(direction, channel, key, SoapySDR::SettingToString(value));
}

template <typename Type>
Type SoapySDR::Device::readSetting(const int direction, const size_t channel, const std::string &key) const
{
    return SoapySDR::StringToSetting<Type>(this->readSetting(direction, channel, key));
}
