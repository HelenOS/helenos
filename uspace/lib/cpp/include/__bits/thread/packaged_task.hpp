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

#ifndef LIBCPP_BITS_THREAD_PACKAGED_TASK
#ifndef LIBCPP_BITS_THREAD_PACKAGED_TASK

namespace std
{
    /**
     * 30.6.9, class template packaged_task:
     */

    template<class>
    class packaged_task; // undefined

    template<class R, class... Args>
    class packaged_task<R(Args...)>
    {
        packaged_task() noexcept
        {}

        template<class F>
        explicit packaged_task(F&& f)
        {}

        template<class F, class Allocator>
        explicit packaged_task(allocator_arg_t, const Allocator& a, F&& f)
        {}

        ~packaged_task()
        {}

        packaged_task(const packaged_task&) = delete;
        packaged_task& operator=(const packaged_task&) = delete;

        packaged_task(packaged_task&& rhs)
        {}

        packaged_task& operator=(packaged_task&& rhs)
        {}

        void swap(packaged_task& other) noexcept
        {}

        bool valid() const noexcept
        {}

        future<R> get_future()
        {}

        void operator()(Args...)
        {}

        void make_ready_at_thread_exit(Args...)
        {}

        void reset()
        {}
    };

    template<class R, class... Args>
    void swap(packaged_task<R(Args...)>& lhs, packaged_task<R(Args...)>& rhs) noexcept
    {
        lhs.swap(rhs);
    };

    template<class R, class Alloc>
    struct uses_allocator<packaged_task<R>, Alloc>: true_type
    { /* DUMMY BODY */ };
}

#endif
