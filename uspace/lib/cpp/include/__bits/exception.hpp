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

