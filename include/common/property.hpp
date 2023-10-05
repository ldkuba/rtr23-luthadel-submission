#pragma once

#include "string.hpp"

#include <functional>
#include <utility>

namespace ENGINE_NAMESPACE {

#define GET [this]() -> auto const&
#define SET , [this](auto&& value)

template<class T>
class Property {
  private:
    class PropertyException {
      public:
        PropertyException(const String message) {}
        ~PropertyException() {}
    };

  public:
    Property(
        const std::function<T const&()> get_fn,
        const std::function<void(T)>    set_fn =
            [](auto&& value) {
                throw PropertyException("This property cannot be changed.");
            }
    ) noexcept
        : _getter { std::move(get_fn) }, _setter { std::move(set_fn) } {}

    Property()                                 = delete;
    Property<T>& operator=(Property<T> const&) = delete;
    // TODO: Probable error causes, should deal with them.
    // Property<T>(Property<T> const&) = delete;
    // Property(Property<T>&&) = delete;
    // Property<T>& operator = (Property<T>&&) = delete;

    operator T const&() const { return _getter(); }

    void     operator=(T const& value) { _setter(value); }
    void     operator=(T&& value) { _setter(std::move(value)); }
    T const& operator()() const { return _getter(); }

  private:
    const std::function<T const&()> _getter;
    const std::function<void(T)>    _setter;
};

} // namespace ENGINE_NAMESPACE