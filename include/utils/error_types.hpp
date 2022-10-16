#pragma once

#include <system_error>

class RuntimeError : public std::runtime_error {
  public:
    RuntimeError() noexcept : std::runtime_error("Unknown error") {}
    template<typename String>
    RuntimeError(const String& __arg) noexcept : std::runtime_error(__arg) {}
    ~RuntimeError() noexcept {}

    using std::runtime_error::what;
    using std::runtime_error::operator=;
    using std::runtime_error::exception;
};

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