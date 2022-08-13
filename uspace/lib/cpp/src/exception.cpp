/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdlib>
#include <exception>

namespace std
{
    const char* exception::what() const noexcept
    {
        return "std::exception";
    }

    const char* bad_exception::what() const noexcept
    {
        return "std::bad_exception";
    }

    namespace aux
    {
        terminate_handler term_handler{nullptr};
        unexpected_handler unex_handler{nullptr};
    }

    terminate_handler get_terminate() noexcept
    {
        return aux::term_handler;
    }

    terminate_handler set_terminate(terminate_handler h) noexcept
    {
        auto res = aux::term_handler;
        aux::term_handler = h;

        return res;
    }

    [[noreturn]] void terminate() noexcept
    {
        if (aux::term_handler)
            aux::term_handler();

        abort();
    }

    bool uncaught_exception() noexcept
    {
        /**
         * Note: The implementation of these two
         *       functions currently uses our mock
         *       exception handling system.
         */
        return aux::exception_thrown;
    }

    int uncaught_exceptins() noexcept
    {
        return aux::exception_thrown ? 1 : 0;
    }

    unexpected_handler get_unexpected() noexcept
    {
        return aux::unex_handler;
    }

    unexpected_handler set_unexpected(unexpected_handler h) noexcept
    {
        auto res = aux::unex_handler;
        aux::unex_handler = h;

        return res;
    }

    [[noreturn]] void unexpected()
    {
        if (aux::unex_handler)
            aux::unex_handler();

        terminate();
    }

    exception_ptr current_exception() noexcept
    {
        return exception_ptr{};
    }

    [[noreturn]] void rethrow_exception(exception_ptr)
    {
        terminate();
    }

    [[noreturn]] void nested_exception::throw_nested() const
    {
        terminate();
    }

    exception_ptr nested_exception::nested_ptr() const noexcept
    {
        return ptr_;
    }
}
