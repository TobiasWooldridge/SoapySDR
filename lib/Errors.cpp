// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <SoapySDR/Errors.hpp>

const char *SoapySDR::errToStr(const int errorCode)
{
    return SoapySDR_errToStr(errorCode);
}

std::string SoapySDR::formatError(
    const std::string &message,
    const std::string &driver,
    const std::string &details)
{
    std::string result;
    if (!driver.empty())
    {
        result += "[" + driver + "] ";
    }
    result += message;
    if (!details.empty())
    {
        result += ": " + details;
    }
    return result;
}

SoapySDR::Exception::Exception(
    const std::string &message,
    const std::string &driver,
    const std::string &details)
    : std::runtime_error(formatError(message, driver, details)),
      _driver(driver),
      _details(details),
      _fullMessage(formatError(message, driver, details))
{
}
