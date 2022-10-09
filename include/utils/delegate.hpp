#pragma once

template<typename R, typename... Args>
class Delegate {
  public:
    virtual R call([[maybe_unused]] Args... arguments) { return {}; }

    template<typename R1, typename... Args1>
    friend bool operator==(
        Delegate<R1, Args1...>& delete1, Delegate<R1, Args1...>& delegate2
    );
};

template<typename... Args>
class Delegate<void, Args...> {
  public:
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
        : _caller(caller), _callback(callback) {}
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
    R (*_callback)(Args...);

  public:
    DelegateFunction(R (*callback)(Args...)) : _callback(callback) {}
    ~DelegateFunction() {}

    R call(Args... arguments) { return (*_callback)(arguments...); }

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

    auto function1 = dynamic_cast<DelegateFunction<R, Args...>*>(&delegate1);
    auto function2 = dynamic_cast<DelegateFunction<R, Args...>*>(&delegate2);

    if (!function1) {
        if (function2) return false;

        // TODO: COMPARE METHODS
        // auto method1 = dynamic_cast<DelegateMethod<void*, R,
        // Args...>*>(&delegate1); auto method2 =
        // dynamic_cast<DelegateMethod<void*, R, Args...>*>(&delegate2);

        // if (method1->_caller == method2->caller &&
        //     method1->_callback == method2->_callback) return true;
        return false;
    }
    if (!function2) return false;

    if (function1->_callback == function2->_callback) return true;
    return false;
}