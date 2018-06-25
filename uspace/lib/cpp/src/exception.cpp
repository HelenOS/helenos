/*
 * Copyright (c) 2018 Jaroslav Jindrak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
