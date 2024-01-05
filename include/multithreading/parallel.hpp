#pragma once

#include "logger.hpp"
#include <tbb/tbb.h>

namespace ENGINE_NAMESPACE {

/**
 * @brief Static class holding a list of functions for parallel multithreaded
 * execution of code.
 */
class Parallel {
  private:
    // Define a type trait to check for the existence of 'iterator' type
    template<typename T, typename = std::void_t<>>
    struct has_iterator //
        : std::false_type {};
    template<typename T>
    struct has_iterator<T, std::void_t<typename T::iterator>> //
        : std::true_type {};

    // Not initialize-able
    Parallel() {}
    ~Parallel() {}

    // Prevent accidental copying
    Parallel(Parallel const&)            = delete;
    Parallel& operator=(Parallel const&) = delete;

  public:
    /**
     * @brief Mutex lock. Allows only one thread to enter code between @p
     * `lock()` and @p `unlock()` calls.
     */
    class Mutex : public tbb::spin_mutex {};

  public:
    // Parallel algorithms

    /**
     * @brief Sort data from range [begin, end) in increasing order. Uses
     * default comparator (less than).
     * @tparam RandomAccessIterator Iterator type
     * @param begin Iterator to the first element of sort range
     * @param end Iterator one past the last element of sort range
     */
    template<typename RandomAccessIterator>
    static void sort(RandomAccessIterator begin, RandomAccessIterator end) {
        tbb::parallel_sort(begin, end);
    }
    /**
     * @brief Sort data from range [begin, end) in order as defined by given
     * comparator.
     * @tparam RandomAccessIterator Iterator type
     * @tparam Compare Comparator type
     * @param begin Iterator to the first element of sort range
     * @param end Iterator one past the last element of sort range
     * @param comp Custom comparator
     */
    template<typename RandomAccessIterator, typename Compare>
    static void sort(
        RandomAccessIterator begin,
        RandomAccessIterator end,
        const Compare&       comp
    ) {
        tbb::parallel_sort(begin, end, comp);
    }

    /**
     * @brief Sort data from range [begin, end] in order as defined by given
     * comparator.
     * @tparam T Data type
     * @tparam Compare Comparator type
     * @param begin Pointer to the first element of sort range
     * @param end Pointer to the last element of sort range
     * @param comp Custom comparator
     */
    template<typename T, typename Compare>
    static void sort(T* begin, T* end, const Compare& comp = std::less<T>()) {
        tbb::parallel_sort(begin, end, comp);
    }

  private:
    // Parallel for loop
    typedef void* var;
    typedef void* iterator_begin;
    typedef void* iterator_end;
    typedef int*  from;
    typedef int*  to;
    typedef void* collection;

  public:
    template<typename T>
    struct __Loop__ {
        __Loop__() {};

        T from;
        T to;

        __Loop__& set_from(T from) {
            this->from = from;
            return *this;
        }
        __Loop__& set_to(T to) {
            this->to = to;
            return *this;
        }

        void operator=(std::function<void(T)> callback) {
            tbb::parallel_for(
                tbb::blocked_range<T>(from, to),
                [&callback](tbb::blocked_range<T> range) {
                    for (T i = range.begin(); i != range.end(); i++)
                        callback(i);
                }
            );
        }
    };

    template<typename T>
    static __Loop__<T> __parallel_for_get__(T& type) {
        return __Loop__<T>();
    }
    template<typename T>
    static __Loop__<T> __parallel_for_get__(T&& type) {
        return __Loop__<T>();
    }

    template<typename T>
    static __Loop__<typename T::iterator> __parallel_for_get_it__(T& type) {
        auto loop = __Loop__<typename T::iterator>();
        loop.from = type.begin();
        loop.to   = type.end();
        return loop;
    }
    template<typename T>
    static __Loop__<typename T::iterator> __parallel_for_get_it__(T&& type) {
        auto loop = __Loop__<typename T::iterator>();
        loop.from = type.begin();
        loop.to   = type.end();
        return loop;
    }

    static void for_loop(var, from, to);
    static void for_loop(var, iterator_begin, iterator_end);
    static void for_loop(var, collection);

  public:
  private:
};

#define for_loop_1(variable, begin, end)                                       \
    __parallel_for_get__(begin).set_from(begin).set_to(end) =                  \
        (std::function<void(variable)>) [&](variable)

#define for_loop_2(variable, collection)                                       \
    __parallel_for_get_it__(collection) =                                      \
        (std::function<void(variable)>) [&](variable)

#define GET_FOR_LOOP_MACRO(_1, _2, _3, NAME, ...) NAME

#undef for_loop
#define for_loop(...)                                                          \
    GET_FOR_LOOP_MACRO(__VA_ARGS__, for_loop_1, for_loop_2)(__VA_ARGS__)

} // namespace ENGINE_NAMESPACE