///
/// \file SoapySDR/Errors.hpp
///
/// Error codes used in the device API.
///
/// \copyright
/// Copyright (c) 2015-2015 Josh Blum
/// SPDX-License-Identifier: BSL-1.0
///

#pragma once
#include <SoapySDR/Config.h>
#include <SoapySDR/Errors.h>
#include <stdexcept>
#include <string>

namespace SoapySDR
{

/*!
 * Convert a error code to a string for printing purposes.
 * If the error code is unrecognized, errToStr returns "UNKNOWN".
 * \param errorCode a negative integer return code
 * \return a pointer to a string representing the error
 */
SOAPY_SDR_API const char *errToStr(const int errorCode);

/*!
 * Exception class with driver context information.
 * Use this to throw exceptions that include driver information
 * for better error diagnostics.
 *
 * Example: throw SoapySDR::Exception("Failed to open device", "rtlsdr", "USB transfer failed");
 */
class SOAPY_SDR_API Exception : public std::runtime_error
{
public:
    /*!
     * Construct an exception with driver context.
     * \param message the error message
     * \param driver the driver name (e.g., "rtlsdr", "sdrplay")
     * \param details optional additional details
     */
    Exception(const std::string &message,
              const std::string &driver = "",
              const std::string &details = "");

    //! Get the driver name that caused the error
    const std::string &getDriver(void) const { return _driver; }

    //! Get additional details about the error
    const std::string &getDetails(void) const { return _details; }

    //! Get the formatted message with full context
    const std::string &getFullMessage(void) const { return _fullMessage; }

private:
    std::string _driver;
    std::string _details;
    std::string _fullMessage;
};

/*!
 * Format an error message with driver context.
 * \param message the error message
 * \param driver the driver name
 * \param details optional additional details
 * \return formatted string like "[rtlsdr] Failed to open: USB error"
 */
SOAPY_SDR_API std::string formatError(
    const std::string &message,
    const std::string &driver = "",
    const std::string &details = "");

}
