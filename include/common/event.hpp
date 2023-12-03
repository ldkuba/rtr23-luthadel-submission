#pragma once

#include "delegate.hpp"
#include "vector.hpp"
#include "outcome.hpp"

namespace ENGINE_NAMESPACE {

template<typename Signature>
class Event;

/**
 * @brief Event object. When invoked (when triggered) also invokes all
 * subscribing functions with same function arguments.
 *
 * @tparam R Return type
 * @tparam Args Argument types
 */
template<typename R, typename... Args>
class Event<R(Args...)> {
  private:
    Vector<Delegate<R, Args...>*> _callbacks {};

  public:
    Event() {}
    ~Event() {}

    /**
     * @brief Subscribe to an event.
     * Attaches a class method as a callback.
     * If multiple instances of the same method are attached, on invoke, method
     * will be called multiple times.
     *
     * @tparam Caller class type.
     * @param caller Class from which to call the method.
     * @param callback Called method
     */
    template<typename T>
    void subscribe(T* caller, R (T::*callback)(Args...)) {
        auto delegate = DelegateMethod<T, R, Args...>(caller, callback);
        _callbacks.emplace_back(delegate);
    }
    /**
     * @brief Subscribe to an event.
     * Attaches a function as callback.
     * If multiple instances of the same function are attached, on invoke,
     * function will be called multiple times.
     *
     * @param callback Called function
     */
    void subscribe(std::function<R(Args...)> callback) {
        auto delegate =
            new (MemoryTag::Callback) DelegateFunction<R, Args...>(callback);
        _callbacks.emplace_back(delegate);
    }

    /**
     * @brief Unsubscribe from the event.
     * Detaches one instance of attached method from the list.
     *
     * @param callback Method to detach.
     * @return true - if a method was detached
     * @return false - if no such method was found
     */
    template<typename T>
    Outcome unsubscribe(T* caller, R (T::*callback)(Args...)) {
        auto delegate = new (MemoryTag::Callback)
            DelegateMethod<T, R, Args...>(caller, callback);
        return remove_delegate(_callbacks, delegate);
    }

    /**
     * @brief Unsubscribe from the event.
     * Detaches one instance of attached function from the list.
     *
     * @param callback Function to detach.
     * @return true - if a function was detached
     * @return false - if no such function was found
     */
    Outcome unsubscribe(std::function<R(Args...)> callback) {
        auto delegate =
            new (MemoryTag::Callback) DelegateFunction<R, Args...>(callback);
        return remove_delegate(_callbacks, delegate);
    }

    /**
     * @brief Invoke all subscribed callbacks with the passed arguments.
     *
     * @param arguments Arguments to be passed to callbacks.
     * @return Returns value returned by the last invoked callback.
     */
    R invoke(Args... arguments) {
        R result;
        // TODO: Make parallel
        for (auto callback : _callbacks) {
            result = callback->call(arguments...);
        }
        return result;
    }

    inline void operator+=(std::function<R(Args...)> callback) {
        subscribe(callback);
    }
    inline void operator-=(std::function<R(Args...)> callback) {
        unsubscribe(callback);
    }
    inline R operator()(Args... arguments) { return invoke(arguments...); }
};

/**
 * @brief Event object. When invoked (when triggered) also invokes all
 * subscribing functions with same function arguments.
 *
 * @tparam Args Argument types
 */
template<typename... Args>
class Event<void(Args...)> {
  private:
    Vector<Delegate<void, Args...>*> _callbacks {};

  public:
    Event() {}
    ~Event() {}

    /**
     * @brief Subscribe to an event.
     * Attaches a class method as a callback.
     * If multiple instances of the same method are attached, on invoke, method
     * will be called multiple times.
     *
     * @tparam Caller class type.
     * @param caller Class from which to call the method.
     * @param callback Called method
     */
    template<typename T>
    void subscribe(T* caller, void (T::*callback)(Args...)) {
        auto delegate = new (MemoryTag::Callback)
            DelegateMethod<T, void, Args...>(caller, callback);
        _callbacks.emplace_back(delegate);
    }
    /**
     * @brief Subscribe to an event.
     * Attaches a function as callback.
     * If multiple instances of the same function are attached, on invoke,
     * function will be called multiple times.
     *
     * @param callback Called function
     */
    void subscribe(std::function<void(Args...)> callback) {
        auto delegate =
            new (MemoryTag::Callback) DelegateFunction<void, Args...>(callback);
        _callbacks.emplace_back(delegate);
    }

    /**
     * @brief Unsubscribe from the event.
     * Detaches one instance of attached method from the list.
     *
     * @param callback Method to detach.
     * @return true - if a method was detached
     * @return false - if no such method was found
     */
    template<typename T>
    Outcome unsubscribe(T* caller, void (T::*callback)(Args...)) {
        auto delegate = new (MemoryTag::Callback)
            DelegateMethod<T, void, Args...>(caller, callback);
        return remove_delegate(_callbacks, delegate);
    }

    /**
     * @brief Unsubscribe from the event.
     * Detaches one instance of attached function from the list.
     *
     * @param callback Function to detach.
     * @return true - if a function was detached
     * @return false - if no such function was found
     */
    Outcome unsubscribe(std::function<void(Args...)> callback) {
        auto delegate =
            new (MemoryTag::Callback) DelegateFunction<void, Args...>(callback);
        return remove_delegate(_callbacks, delegate);
    }

    /**
     * @brief Invoke all subscribed callbacks with the passed arguments.
     *
     * @param arguments Arguments to be passed to callbacks.
     */
    void invoke(Args... arguments) {
        for (auto callback : _callbacks) {
            callback->call(arguments...);
        }
    }

    inline void operator+=(std::function<void(Args...)> callback) {
        subscribe(callback);
    }
    inline void operator-=(std::function<void(Args...)> callback) {
        unsubscribe(callback);
    }
    inline void operator()(Args... arguments) { return invoke(arguments...); }
};

template<typename R, typename... Args>
Outcome remove_delegate(
    Vector<Delegate<R, Args...>*> callbacks, Delegate<R, Args...>* delegate
) {
    auto iter = callbacks.begin();
    while (iter != callbacks.end()) {
        if (**iter == *delegate) break;
        iter++;
    }
    if (iter == callbacks.end()) return Outcome::Failed;

    callbacks.erase(iter);
    del(*iter);

    return Outcome::Successful;
}

} // namespace ENGINE_NAMESPACE