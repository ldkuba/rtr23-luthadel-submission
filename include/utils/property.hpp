#pragma once

#include <functional>
#include <utility>

#define Get [this]() -> auto const&
#define Set [this](auto&& value)

template<class T>
class Property {
public:
    Property(
        std::function<T const& ()> get_fn,
        std::function<void(T)> set_fn
        = [](auto&& value) { throw std::runtime_error("This property cannot be changed."); }
    ) : _getter{ std::move(get_fn) }, _setter{ std::move(set_fn) } {}

    Property() = delete;
    Property(Property<T> const&) = delete;
    Property<T>& operator = (Property<T> const&) = delete;
    Property(Property<T>&&) = delete;
    Property<T>& operator = (Property<T>&&) = delete;

    operator T const& () const {
        return _getter();
    }

    void operator=(T const& value) {
        _setter(value);
    }

    void operator=(T&& value) {
        _setter(std::move(value));
    }

    T const& operator () () const {
        return _getter();
    }

private:
    std::function<T const& ()> _getter;
    std::function<void(T)> _setter;
};