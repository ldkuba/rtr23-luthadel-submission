#pragma once

#include <system_error>
#include "defines.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief One of two subclasses of exception.
 * Specialization of std::runtime_error, specifically made for Result exception
 * handling system.
 * Runtime errors represent problems outside the scope of a program; they cannot
 * be easily predicted and can generally only be caught as the program executes.
 */
class RuntimeError : public std::runtime_error {
  public:
    /**
     * @brief Construct a new Runtime Error object
     */
    RuntimeError() noexcept : std::runtime_error("Unknown error") {}
    /**
     * @brief Construct a new Runtime Error object
     *
     * @tparam String String like type
     * @param __arg Error message
     */
    template<typename String>
    RuntimeError(const String& __arg) noexcept : std::runtime_error(__arg) {}
    ~RuntimeError() noexcept {}

    using std::runtime_error::what;
    using std::runtime_error::operator=;
    using std::runtime_error::exception;
};

/**
 * @brief Specialization of RuntimeError which also produces an error code.
 */
class RuntimeErrorCode : public RuntimeError {
  public:
    /**
     * @brief Construct a new Runtime Error Code object
     *
     * @param error_code Error code
     */
    RuntimeErrorCode(uint8 error_code = 0) noexcept
        : RuntimeError("Unknown error"), _error_code(error_code) {}
    /**
     * @brief Construct a new Runtime Error Code object
     *
     * @tparam String String like type
     * @param error_code Error code
     * @param __arg Message
     */
    template<typename String>
    RuntimeErrorCode(uint8 error_code, const String& __arg) noexcept
        : RuntimeError(__arg), _error_code(error_code) {}
    ~RuntimeErrorCode() noexcept {}

    using RuntimeError::what;
    using RuntimeError::operator=;
    using RuntimeError::exception;

    const uint8& code() const noexcept { return _error_code; }

  private:
    uint8 _error_code;
};

/**
 * @brief Thrown to report invalid arguments to functions.
 * Specialization of std::invalid_argument, specifically made for Result
 * exception handling system.
 */
class InvalidArgument : public std::invalid_argument {
  public:
    InvalidArgument() noexcept : std::invalid_argument("Unknown error") {}
    template<typename String>
    InvalidArgument(const String& __arg) noexcept
        : std::invalid_argument(__arg) {}
    ~InvalidArgument() noexcept {}

    using std::invalid_argument::what;
    using std::invalid_argument::logic_error;
    using std::invalid_argument::operator=;
};

} // namespace ENGINE_NAMESPACE