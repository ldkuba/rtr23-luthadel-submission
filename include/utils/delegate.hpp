#pragma once

#include <functional>

template<typename R, typename... Args>
class Delegate {
  public:
    std::size_t owner = 0;

    virtual R call([[maybe_unused]] Args... arguments) { return {}; }

    template<typename R1, typename... Args1>
    friend bool operator==(
        Delegate<R1, Args1...>& delete1, Delegate<R1, Args1...>& delegate2
    );
};

template<typename... Args>
class Delegate<void, Args...> {
  public:
    std::size_t owner = 0;

    virtual void call([[maybe_unused]] Args... arguments) {}

    template<typename R1, typename... Args1>
    friend bool operator==(
        Delegate<void, Args1...>& delete1, Delegate<void, Args1...>& delegate2
    );
};

template<typename T, typename R, typename... Args>
class DelegateMethod : public Delegate<R, Args...> {
  private:
    T* _caller;
    R (T::*_callback)(Args...);

  public:
    DelegateMethod(T* caller, R (T::*callback)(Args...))
        : _caller(caller), _callback(callback) {
        this->owner = (size_t) caller;
    }
    ~DelegateMethod() {}

    R call(Args... arguments) { return (_caller->*(_callback))(arguments...); }

    template<typename R1, typename... Args1>
    friend bool operator==(
        Delegate<R1, Args1...>& delete1, Delegate<R1, Args1...>& delegate2
    );
};

template<typename R, typename... Args>
class DelegateFunction : public Delegate<R, Args...> {
  private:
    std::function<R(Args...)> _callback;

  public:
    DelegateFunction(std::function<R(Args...)> callback)
        : _callback(callback) {}
    ~DelegateFunction() {}

    R call(Args... arguments) { return _callback(arguments...); }

    template<typename R1, typename... Args1>
    friend bool operator==(
        Delegate<R1, Args1...>& delete1, Delegate<R1, Args1...>& delegate2
    );
};

template<typename R, typename... Args>
bool operator==(
    Delegate<R, Args...>& delegate1, Delegate<R, Args...>& delegate2
) {
    if (&delegate1 == &delegate2) return true;

    if (delegate1.owner != delegate2.owner) return false;

    // Method comparison
    if (delegate1.owner) { return false; }

    // Function comparison
    auto function1 = dynamic_cast<DelegateFunction<R, Args...>*>(&delegate1);
    auto function2 = dynamic_cast<DelegateFunction<R, Args...>*>(&delegate2);

    if (function1->_callback == function2->_callback) return true;
    return false;
}