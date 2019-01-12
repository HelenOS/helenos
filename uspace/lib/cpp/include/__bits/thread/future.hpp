/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#ifndef LIBCPP_BITS_THREAD_FUTURE
#define LIBCPP_BITS_THREAD_FUTURE

#include <cassert>
#include <memory>
#include <system_error>
#include <type_traits>

namespace std
{
    /**
     * 30.6, futures:
     */

    enum class future_errc
    { // The 5001 start is to not collide with system_error's codes.
        broken_promise = 5001,
        future_already_retrieved,
        promise_already_satisfied,
        no_state
    };

    enum class launch
    {
        async,
        deferred
    };

    enum class future_status
    {
        ready,
        timeout,
        deferred
    };

    /**
     * 30.6.2, error handling:
     */

    template<>
    struct is_error_code_enum<future_errc>: true_type
    { /* DUMMY BODY */ };

    error_code make_error_code(future_errc) noexcept;
    error_condition make_error_condition(future_errc) noexcept;

    const error_category& future_category() noexcept;

    /**
     * 30.6.3, class future_error:
     */

    class future_error: public logic_error
    {
        public:
            future_error(error_code ec);

            const error_code& code() const noexcept;

        private:
            error_code code_;
    };

    /**
     * 30.6.4, shared state:
     */

    template<class R>
    class promise
    {
    };

    template<class R>
    class promise<R&>
    {
    };

    template<>
    class promise<void>
    {
    };

    template<class R>
    void swap(promise<R>& lhs, promise<R>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template<class R, class Alloc>
    struct uses_allocator<promise<R>, Alloc>: true_type
    { /* DUMMY BODY */ };

    template<class R>
    class future
    {
    };

    template<class R>
    class future<R&>
    {
    };

    template<>
    class future<void>
    {
    };

    template<class R>
    class shared_future
    {
    };

    template<class R>
    class shared_future<R&>
    {
    };

    template<>
    class shared_future<void>
    {
    };

    template<class>
    class packaged_task; // undefined

    template<class R, class... Args>
    class packaged_task<R(Args...)>
    {
    };

    template<class R, class... Args>
    void swap(packaged_task<R(Args...)>& lhs, packaged_task<R(Args...)>& rhs) noexcept
    {
        lhs.swap(rhs);
    };

    template<class R, class Alloc>
    struct uses_allocator<packaged_task<R>, Alloc>: true_type
    { /* DUMMY BODY */ };

    template<class F, class... Args>
    future<result_of_t<decay_t<F>(decay_t<Args>...)>>
    async(F&& f, Args&&... args)
    {
        // TODO: implement
        __unimplemented();
    }

    template<class F, class... Args>
    future<result_of_t<decay_t<F>(decay_t<Args>...)>>
    async(launch, F&& f, Args&&... args)
    {
        // TODO: implement
        __unimplemented();
    }
}

#endif
