/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_EXCEPTION
#define LIBCPP_BITS_EXCEPTION

#include <__bits/trycatch.hpp>

namespace std
{
    /**
     * 18.8.1, class exception:
     */

    class exception
    {
        public:
            exception() noexcept = default;
            exception(const exception&) noexcept = default;
            exception& operator=(const exception&) noexcept = default;
            virtual ~exception() = default;

            virtual const char* what() const noexcept;
    };

    /**
     * 18.8.2, class bad_exception:
     */

    class bad_exception: public exception
    {
        public:
            bad_exception() noexcept = default;
            bad_exception(const bad_exception&) noexcept = default;
            bad_exception& operator=(const bad_exception&) noexcept = default;

            virtual const char* what() const noexcept;
    };

    /**
     * 18.8.3, abnormal termination:
     */

    using terminate_handler = void (*)();

    terminate_handler get_terminate() noexcept;
    terminate_handler set_terminate(terminate_handler) noexcept;
    [[noreturn]] void terminate() noexcept;

    /**
     * 18.8.4, uncaght_exceptions:
     */

    bool uncaught_exception() noexcept;
    int uncaught_exceptions() noexcept;

    using unexpected_handler = void (*)();

    unexpected_handler get_unexpected() noexcept;
    unexpected_handler set_unexpected(unexpected_handler) noexcept;
    [[noreturn]] void unexpected();

    /**
     * 18.8.5, exception propagation:
     */

    namespace aux
    {
        class exception_ptr_t
        { /* DUMMY BODY */ };
    }

    using exception_ptr = aux::exception_ptr_t;

    exception_ptr current_exception() noexcept;
    [[noreturn]] void rethrow_exception(exception_ptr);

    template<class E>
    exception_ptr make_exception_ptr(E e) noexcept
    {
        return exception_ptr{};
    }

    class nested_exception
    {
        public:
            nested_exception() noexcept = default;
            nested_exception(const nested_exception&) noexcept = default;
            nested_exception& operator=(const nested_exception&) noexcept = default;
            virtual ~nested_exception() = default;

            [[noreturn]] void throw_nested() const;
            exception_ptr nested_ptr() const noexcept;

        private:
            exception_ptr ptr_;
    };

    template<class E>
    [[noreturn]] void throw_with_nested(E&& e)
    {
        terminate();
    }

    template<class E>
    void rethrow_if_nested(const E& e)
    {
        auto casted = dynamic_cast<const nested_exception*>(&e);
        if (casted)
            casted->throw_nested();
    }
}

#endif

