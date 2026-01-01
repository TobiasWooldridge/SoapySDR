// Copyright (c) 2014-2019 Josh Blum
// Copyright (c) 2016-2016 Bastille Networks
// Copyright (c) 2021-2023 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

%define DOCSTRING
"SoapySDR API.

SoapySDR is an open-source generalized API and runtime library for interfacing
with Software Defined Radio devices. With SoapySDR, you can instantiate,
configure, and stream with an SDR device in a variety of environments.
Refer to https://github.com/pothosware/SoapySDR/wiki

This Python interface closely maps to the C/C++ one.
See https://pothosware.github.io/SoapySDR/doxygen/latest/index.html for details.
"
%enddef

%module(directors="1", docstring=DOCSTRING) SoapySDR

// SWIG 4.0 added the ability to automatically generate Python docstrings
// from Doxygen input.
#if SWIG_VERSION >= 0x040000
%include "doctypes.i"
#endif

%include "soapy_common.i"

////////////////////////////////////////////////////////////////////////
// python3.8 and up need to have the dll search path set
// https://docs.python.org/3/whatsnew/3.8.html#bpo-36085-whatsnew
////////////////////////////////////////////////////////////////////////
%pythonbegin %{
import os

if os.name == 'nt' and hasattr(os, 'add_dll_directory'):
    root_dir = __file__
    for i in range(5): #limit search depth
        root_dir = os.path.dirname(root_dir)
        bin_dir = os.path.join(root_dir, 'bin')
        if os.path.exists(bin_dir):
            try: os.add_dll_directory(bin_dir)
            except Exception as ex:
                print('add_dll_directory(%s): %s'%(bin_dir, ex))
            break
%}

////////////////////////////////////////////////////////////////////////
// Check ABI before attempting to use Python module
////////////////////////////////////////////////////////////////////////
%insert("python")
%{

COMPILE_ABI_VERSION = "@SOAPY_SDR_ABI_VERSION@"
PYTHONLIB_ABI_VERSION = _SoapySDR.SOAPY_SDR_ABI_VERSION
CORELIB_ABI_VERSION = _SoapySDR.getABIVersion()

if not (COMPILE_ABI_VERSION == PYTHONLIB_ABI_VERSION == CORELIB_ABI_VERSION):
    raise Exception("""Failed ABI check.
Import script:    {0}
Python module:    {1}
SoapySDR library: {2}""".format(COMPILE_ABI_VERSION, PYTHONLIB_ABI_VERSION, CORELIB_ABI_VERSION))
%}

////////////////////////////////////////////////////////////////////////
// Include all major headers to compile against
////////////////////////////////////////////////////////////////////////
%{
#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Errors.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Time.hpp>
#include <SoapySDR/Logger.hpp>
%}

////////////////////////////////////////////////////////////////////////
// http://www.swig.org/Doc2.0/Library.html#Library_stl_exceptions
////////////////////////////////////////////////////////////////////////
%include <exception.i>

// We only expect to throw DirectorExceptions from within
// SoapySDR_pythonLogHandlerBase calls.  Catching them permits us to
// propagate exceptions thrown in the Python log handler callback back to
// Python.
%exception
{
    try{$action}
    catch (const Swig::DirectorException &e) { SWIG_fail; }
    SWIG_CATCH_STDEXCEPT
    catch (...)
    {SWIG_exception(SWIG_RuntimeError, "unknown");}
}

////////////////////////////////////////////////////////////////////////
// Config header defines API export
////////////////////////////////////////////////////////////////////////
%include <SoapySDR/Config.h>

////////////////////////////////////////////////////////////////////////
// Commonly used data types
////////////////////////////////////////////////////////////////////////
%include <std_complex.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_map.i>
%include <SoapySDR/Types.hpp>

//handle arm 32-bit case where size_t and unsigned are the same
#ifdef SIZE_T_IS_UNSIGNED_INT
%typedef unsigned int size_t;
#else
%template(SoapySDRUnsignedList) std::vector<unsigned>;
#endif

%template(SoapySDRKwargs) std::map<std::string, std::string>;
%template(SoapySDRKwargsList) std::vector<SoapySDR::Kwargs>;
%template(SoapySDRArgInfoList) std::vector<SoapySDR::ArgInfo>;
%template(SoapySDRStringList) std::vector<std::string>;
%template(SoapySDRRangeList) std::vector<SoapySDR::Range>;
%template(SoapySDRSizeList) std::vector<size_t>;
%template(SoapySDRDoubleList) std::vector<double>;
%template(SoapySDRDeviceList) std::vector<SoapySDR::Device *>;

%extend std::map<std::string, std::string>
{
    %insert("python")
    %{
        def __str__(self):
            out = list()
            for k, v in self.iteritems():
                out.append("%s=%s"%(k, v))
            return '{'+(', '.join(out))+'}'

        def __repr__(self):
            return self.__str__()
    %}
};

%extend SoapySDR::Range
{
    %insert("python")
    %{
        def __str__(self):
            fields = [self.minimum(), self.maximum()]
            if self.step() != 0.0: fields.append(self.step())
            return ', '.join(['%g'%f for f in fields])

        def __repr__(self):
            return self.__str__()
    %}
};

////////////////////////////////////////////////////////////////////////
// Stream result class
// Helps us deal with stream calls that return by reference
////////////////////////////////////////////////////////////////////////
%inline %{
    struct StreamResult
    {
        StreamResult(void):
            ret(0), flags(0), timeNs(0), chanMask(0){}
        int ret;
        int flags;
        long long timeNs;
        size_t chanMask;
    };
%}

%extend StreamResult
{
    %insert("python")
    %{
        def __str__(self):
            return "ret=%s, flags=%s, timeNs=%s"%(self.ret, self.flags, self.timeNs)

        def __repr__(self):
            return self.__str__()
    %}
};

////////////////////////////////////////////////////////////////////////
// Native stream format class
// Allows proper wrapper for SoapySDR::Device::getNativeStreamFormat()
////////////////////////////////////////////////////////////////////////
%inline %{
    struct NativeStreamFormat
    {
        std::string format;
        double fullScale;
    };
%}

%extend NativeStreamFormat
{
    %insert("python")
    %{
        def __str__(self):
            return "format=%s, fullScale=%f"%(self.format, self.fullScale)

        def __repr__(self):
            return self.__str__()
    %}
};

////////////////////////////////////////////////////////////////////////
// Constants SOAPY_SDR_*
////////////////////////////////////////////////////////////////////////
%include <SoapySDR/Constants.h>
//import types.h for the defines
%include <SoapySDR/Types.h>
%include <SoapySDR/Errors.h>
%include <SoapySDR/Version.h>
%include <SoapySDR/Formats.h>

////////////////////////////////////////////////////////////////////////
// Logging tie-ins for python
////////////////////////////////////////////////////////////////////////
%include <SoapySDR/Logger.h>
%include <SoapySDR/Logger.hpp>

%feature("director:except") {
    if ($error != NULL) {
        throw Swig::DirectorMethodException();
    }
}


%feature("director") _SoapySDR_pythonLogHandlerBase;

%inline %{
    class _SoapySDR_pythonLogHandlerBase
    {
    public:
        _SoapySDR_pythonLogHandlerBase(void)
        {
            globalHandle = this;
            SoapySDR::registerLogHandler(&globalHandler);
        }
        virtual ~_SoapySDR_pythonLogHandlerBase(void)
        {
            globalHandle = nullptr;
            // Restore the default, C coded, log handler.
            SoapySDR::registerLogHandler(nullptr);
        }
        virtual void handle(const SoapySDR::LogLevel, const char *) = 0;

    private:
        static void globalHandler(const SoapySDR::LogLevel logLevel, const char *message)
        {
            if (globalHandle != nullptr) globalHandle->handle(logLevel, message);
        }

        static _SoapySDR_pythonLogHandlerBase *globalHandle;
    };
%}

%{
    _SoapySDR_pythonLogHandlerBase *_SoapySDR_pythonLogHandlerBase::globalHandle = nullptr;
%}

%insert("python")
%{
_SoapySDR_globalLogHandlers = [None]

class _SoapySDR_pythonLogHandler(_SoapySDR_pythonLogHandlerBase):
    def __init__(self, handler):
        self.handler = handler
        getattr(_SoapySDR_pythonLogHandlerBase, '__init__')(self)

    def handle(self, *args): self.handler(*args)

def registerLogHandler(handler):
    """Register a new system log handler.

    Platforms should call this to replace the default stdio handler.

    :param handler: is a callback function that's called each time an event is
    to be logged by the SoapySDR module.  It is passed the log level and the
    the log message.  The callback shouldn't return anything, but may throw
    exceptions which can be handled in turn in the Python client code.
    Alternately, setting handler to None restores the default.

    :type handler: Callable[[int, str], None] or None

    :returns: None
    """
    if handler is None:
        _SoapySDR_globalLogHandlers[0] = None
    else:
        _SoapySDR_globalLogHandlers[0] = _SoapySDR_pythonLogHandler(handler)
%}

////////////////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////////////////
%include <SoapySDR/Errors.hpp>
%include <SoapySDR/Version.hpp>
%include <SoapySDR/Modules.hpp>
%include <SoapySDR/Formats.hpp>
%include <SoapySDR/Time.hpp>

%include <SoapySDR/Logger.hpp>

////////////////////////////////////////////////////////////////////////
// Device object
////////////////////////////////////////////////////////////////////////

// These are being replaced later.
%ignore SoapySDR::Device::getNativeStreamFormat;
%ignore SoapySDR::Device::readStream;
%ignore SoapySDR::Device::writeStream;
%ignore SoapySDR::Device::readStreamStatus;

// These have no meaning on this layer.
%ignore SoapySDR::Device::getNumDirectAccessBuffers;
%ignore SoapySDR::Device::getDirectAccessBufferAddrs;
%ignore SoapySDR::Device::acquireReadBuffer;
%ignore SoapySDR::Device::releaseReadBuffer;
%ignore SoapySDR::Device::acquireWriteBuffer;
%ignore SoapySDR::Device::releaseWriteBuffer;
%ignore SoapySDR::Device::getNativeDeviceHandle;

%nodefaultctor SoapySDR::Device;
%include <SoapySDR/Device.hpp>

//narrow import * to SOAPY_SDR_ constants
%pythoncode %{

__all__ = list()
for key in sorted(globals().keys()):
    if key.startswith('SOAPY_SDR_'):
        __all__.append(key)
%}

//make device a constructable class
%insert("python")
%{
class Device(Device):
    def __new__(cls, *args, **kwargs):
        return cls.make(*args, **kwargs)

def extractBuffPointer(buff):
    if hasattr(buff, '__array_interface__'): return buff.__array_interface__['data'][0]
    if hasattr(buff, 'buffer_info'): return buff.buffer_info()[0]
    if hasattr(buff, '__long__'): return long(buff)
    if hasattr(buff, '__int__'): return int(buff)
    raise Exception("Unrecognized data format: " + str(type(buff)))
%}

%extend SoapySDR::Device
{
    //additional overloads for writeSetting for basic types
    %template(writeSetting) SoapySDR::Device::writeSetting<bool>;
    %template(writeSetting) SoapySDR::Device::writeSetting<double>;
    %template(writeSetting) SoapySDR::Device::writeSetting<long long>;
    %template(readSensorBool) SoapySDR::Device::readSensor<bool>;
    %template(readSensorInt) SoapySDR::Device::readSensor<long long>;
    %template(readSensorFloat) SoapySDR::Device::readSensor<double>;
    %template(readSettingBool) SoapySDR::Device::readSetting<bool>;
    %template(readSettingInt) SoapySDR::Device::readSetting<long long>;
    %template(readSettingFloat) SoapySDR::Device::readSetting<double>;

    NativeStreamFormat __getNativeStreamFormat(const int direction, const size_t channel)
    {
        NativeStreamFormat format;
        format.format = self->getNativeStreamFormat(direction, channel, format.fullScale);

        return format;
    }

    StreamResult __readStream(SoapySDR::Stream *stream, const std::vector<size_t> &buffs, const size_t numElems, const int flags, const long timeoutUs)
    {
        StreamResult sr;
        sr.flags = flags;
        std::vector<void *> ptrs(buffs.size());
        for (size_t i = 0; i < buffs.size(); i++) ptrs[i] = (void *)buffs[i];
        sr.ret = self->readStream(stream, (&ptrs[0]), numElems, sr.flags, sr.timeNs, timeoutUs);
        return sr;
    }

    StreamResult __writeStream(SoapySDR::Stream *stream, const std::vector<size_t> &buffs, const size_t numElems, const int flags, const long long timeNs, const long timeoutUs)
    {
        StreamResult sr;
        sr.flags = flags;
        std::vector<const void *> ptrs(buffs.size());
        for (size_t i = 0; i < buffs.size(); i++) ptrs[i] = (const void *)buffs[i];
        sr.ret = self->writeStream(stream, (&ptrs[0]), numElems, sr.flags, timeNs, timeoutUs);
        return sr;
    }

    StreamResult __readStreamStatus(SoapySDR::Stream *stream, const long timeoutUs)
    {
        StreamResult sr;
        sr.ret = self->readStreamStatus(stream, sr.chanMask, sr.flags, sr.timeNs, timeoutUs);
        return sr;
    }

    %insert("python")
    %{
        #manually unmake and flag for future calls and the deleter
        def close(self):
            try: getattr(self, '__closed__')
            except AttributeError: Device.unmake(self)
            setattr(self, '__closed__', True)

        def __del__(self): self.close()

        def __str__(self):
            return "%s:%s"%(self.getDriverKey(), self.getHardwareKey())

        def __repr__(self):
            return "%s:%s"%(self.getDriverKey(), self.getHardwareKey())

        def getNativeStreamFormat(self, direction, chan):
            r"""
            Get the hardware's native stream format for this channel.
            This is the format used by the underlying transport layer.
            :type direction: int
            :param direction: the channel direction RX or TX
            :type chan: an available channel on the device
            :rtype SoapySDR.NativeStreamFormat
            :returns the native stream format and max possible value
            """
            return self.__getNativeStreamFormat(direction, chan)

        def readStream(self, stream, buffs, numElems, flags = 0, timeoutUs = 100000):
            r"""
            Read elements from a stream for reception.
            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :type buffs: numpy.ndarray
            :param buffs: a 2D NumPy array of the underlying stream type
            :type numElems: int
            :param numElems: the number of elements in each buffer
            :type timeoutUs: int
            :param timeoutUs: the timeout in microseconds
            :rtype: SoapySDR.StreamResult
            :returns the number of elements read per buffer, plus metadata
            """
            ptrs = [extractBuffPointer(b) for b in buffs]
            return self.__readStream(stream, ptrs, numElems, flags, timeoutUs)

        def writeStream(self, stream, buffs, numElems, flags = 0, timeNs = 0, timeoutUs = 100000):
            r"""
            Write elements to a stream for transmission.
            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :type numElems: int
            :param numElems: the number of elements in each buffer
            :type flags: int
            :param flags: optional input flags
            :type timeNs: int
            :param timeNs: the buffer's timestamp in nanoseconds
            :type timeoutUs: int
            :param timeoutUs: the timeout in microseconds
            :rtype: SoapySDR.StreamResult
            :returns the number of elements written per buffer, plus metadata
            """
            ptrs = [extractBuffPointer(b) for b in buffs]
            return self.__writeStream(stream, ptrs, numElems, flags, timeNs, timeoutUs)

        def readStreamStatus(self, stream, timeoutUs = 100000):
            r"""
            Readback status information about a stream. This call
            is typically used on a transmit stream to report time
            errors, underflows, and burst completion.
            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :type timeoutUs: int
            :param timeoutUs: the timeout in microseconds
            :rtype: SoapySDR.StreamResult
            :returns any stream errors, plus other metadata
            """
            return self.__readStreamStatus(stream, timeoutUs)

        def readStreamIntoBuffers(self, stream, buffs, flags = 0, timeoutUs = 100000):
            r"""
            Convenience method: Read elements from a stream directly into provided buffers.
            This method auto-detects the number of elements from the buffer size.

            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :type buffs: list of numpy.ndarray
            :param buffs: list of NumPy arrays to receive samples (one per channel)
            :type flags: int
            :param flags: optional input flags
            :type timeoutUs: int
            :param timeoutUs: the timeout in microseconds (default 100000 = 100ms)
            :rtype: SoapySDR.StreamResult
            :returns StreamResult with ret (samples read or error code), flags, timeNs

            Example:
                import numpy as np
                buff = np.zeros(1024, dtype=np.complex64)
                result = device.readStreamIntoBuffers(stream, [buff])
                if result.ret > 0:
                    samples = buff[:result.ret]  # Use actual samples read
            """
            if not buffs:
                raise ValueError("buffs must contain at least one buffer")
            numElems = len(buffs[0])
            ptrs = [extractBuffPointer(b) for b in buffs]
            return self.__readStream(stream, ptrs, numElems, flags, timeoutUs)

        def readStreamNumpy(self, stream, numSamples, format='CF32', numChannels=1, flags=0, timeoutUs=100000):
            r"""
            Convenience method: Read samples and return as NumPy array(s).
            Allocates buffers automatically - useful for simple cases.

            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :type numSamples: int
            :param numSamples: number of samples to read per channel
            :type format: str
            :param format: sample format ('CF32', 'CS16', 'CS8', 'CU8', 'F32', 'S16', etc.)
            :type numChannels: int
            :param numChannels: number of channels to read (default 1)
            :type flags: int
            :param flags: optional input flags
            :type timeoutUs: int
            :param timeoutUs: the timeout in microseconds (default 100000 = 100ms)
            :rtype: tuple (list of numpy.ndarray, SoapySDR.StreamResult)
            :returns tuple of (buffers, result) where buffers contains the sample data

            Example:
                buffers, result = device.readStreamNumpy(stream, 1024, 'CF32')
                if result.ret > 0:
                    samples = buffers[0][:result.ret]
                    # Process samples...
            """
            try:
                import numpy as np
            except ImportError:
                raise ImportError("readStreamNumpy requires NumPy. Install with: pip install numpy")

            # Map format strings to NumPy dtypes
            format_map = {
                'CF64': np.complex128,
                'CF32': np.complex64,
                'CS32': np.dtype([('re', np.int32), ('im', np.int32)]),
                'CS16': np.dtype([('re', np.int16), ('im', np.int16)]),
                'CS8': np.dtype([('re', np.int8), ('im', np.int8)]),
                'CU32': np.dtype([('re', np.uint32), ('im', np.uint32)]),
                'CU16': np.dtype([('re', np.uint16), ('im', np.uint16)]),
                'CU8': np.dtype([('re', np.uint8), ('im', np.uint8)]),
                'F64': np.float64,
                'F32': np.float32,
                'S32': np.int32,
                'S16': np.int16,
                'S8': np.int8,
                'U32': np.uint32,
                'U16': np.uint16,
                'U8': np.uint8,
            }

            if format not in format_map:
                raise ValueError(f"Unknown format '{format}'. Supported: {list(format_map.keys())}")

            dtype = format_map[format]
            buffs = [np.zeros(numSamples, dtype=dtype) for _ in range(numChannels)]
            result = self.readStreamIntoBuffers(stream, buffs, flags, timeoutUs)
            return (buffs, result)

        def getDirectBuffersInfo(self, stream):
            r"""
            Get information about available direct access buffers.
            This is useful for zero-copy streaming when supported by the driver.

            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :rtype: tuple (int, int)
            :returns (numBuffers, bufferSize) or (0, 0) if not supported

            Example:
                numBuffers, bufferSize = device.getDirectBuffersInfo(stream)
                if numBuffers > 0:
                    # Can use direct buffer access
                    pass
            """
            numBuffers = self.getNumDirectAccessBuffers(stream)
            if numBuffers == 0:
                return (0, 0)
            # Estimate buffer size from MTU
            bufferSize = self.getStreamMTU(stream)
            return (numBuffers, bufferSize)

        def hasDirectBufferAccess(self, stream):
            r"""
            Check if the stream supports direct buffer access (zero-copy).

            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :rtype: bool
            :returns True if direct buffer access is available
            """
            return self.getNumDirectAccessBuffers(stream) > 0

        def readStreamDirect(self, stream, timeoutUs=100000):
            r"""
            Acquire a read buffer directly from the stream (zero-copy).
            The returned buffer is valid until releaseReadBuffer is called.

            :type stream: SoapySDR.Stream
            :param stream: SoapySDR stream handle
            :type timeoutUs: int
            :param timeoutUs: timeout in microseconds
            :rtype: tuple (handle, buffers, numSamples, flags, timeNs) or None
            :returns buffer info or None if not supported/timeout

            Example:
                result = device.readStreamDirect(stream)
                if result:
                    handle, buffers, numSamples, flags, timeNs = result
                    # Process buffers directly (zero-copy)
                    device.releaseReadBuffer(stream, handle)
            """
            numBuffs = self.getNumDirectAccessBuffers(stream)
            if numBuffs == 0:
                return None
            # Call the C++ acquireReadBuffer through the wrapped interface
            try:
                result = self._Device__acquireReadBuffer(stream, timeoutUs)
                if result.ret > 0:
                    return (result.handle, result.buffs, result.ret, result.flags, result.timeNs)
                return None
            except:
                return None
    %}
};
