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

#ifndef LIBCPP_MUTEX
#define LIBCPP_MUTEX

#include <internal/common.hpp>
#include <internal/thread.hpp>
#include <thread>

namespace std
{
    /**
     * 20.4.1.2.1, class mutex:
     */

    class mutex
    {
        public:
            constexpr mutex() noexcept
                : mtx_{}
            {
                aux::threading::mutex::init(mtx_);
            }

            ~mutex();

            mutex(const mutex&) = delete;
            mutex& operator=(const mutex&) = delete;

            void lock();
            bool try_lock();
            void unlock();

            using native_handle_type = aux::mutex_t*;
            native_handle_type native_handle();

        private:
            aux::mutex_t mtx_;
    };

    /**
     * 30.4.1.2.2, class recursive_mutex:
     */

    class recursive_mutex
    {
        public:
            constexpr recursive_mutex() noexcept;
            ~recursive_mutex();

            recursive_mutex(const recursive_mutex&) = delete;
            recursive_mutex& operator=(const recursive_mutex&) = delete;

            void lock();
            bool try_lock();
            void unlock();

            using native_handle_type = aux::mutex_t*;
            native_handle_type native_handle();

        private:
            aux::mutex_t mtx_;
            size_t lock_level_;
            thread::id owner_;
    };

    /**
     * 30.4.1.3.1, class timed_mutex:
     */

    // TODO: implement
    class timed_mutex;

    /**
     * 30.4.1.3.2, class recursive_timed_mutex:
     */

    // TODO: implement
    class recursive_timed_mutex;

    struct defer_lock_t
    { /* DUMMY BODY */ };

    struct try_to_lock_t
    { /* DUMMY BODY */ };

    struct adopt_lock_t
    { /* DUMMY BODY */ };

    constexpr defer_lock_t defer_lock
    { /* DUMMY BODY */ };

    constexpr try_to_lock_t try_to_lock
    { /* DUMMY BODY */ };

    constexpr adopt_lock_t adopt_lock
    { /* DUMMY BODY */ };

    /**
     * 30.4.2.1, class template lock_guard:
     */

    template<class Mutex>
    class lock_guard
    {
        public:
            using mutex_type = Mutex;

            explicit lock_guard(mutex_type& mtx)
                : mtx_{mtx}
            {
                mtx.lock();
            }

            lock_guard(mutex_type& mtx, adopt_lock_t)
                : mtx_{mtx}
            { /* DUMMY BODY */ }

            ~lock_guard()
            {
                mtx_.unlock();
            }

            lock_guard(const lock_guard&) = delete;
            lock_guard& operator=(const lock_guard&) = delete;

        private:
            mutex_type& mtx_;
    };

    template<class Mutex>
    class unique_lock;

    template<class Mutex>
    void swap(unique_lock<Mutex>& lhs, unique_lock<Mutex>& rhs) noexcept;

    template<class L1, class L2, class... L3>
    int try_lock(L1& l1, L2& l2, L3&... ls);

    template<class L1, class L2, class... L3>
    void lock(L1& l1, L2& l2, L3&... ls);

    struct once_flag
    {
        constexpr once_flag() noexcept;

        once_flag(const once_flag&) = delete;
        once_flag& operator=(const once_flag&) = delete;
    };

    template<class Callable, class... Args>
    void call_once(once_flag& flag, Callable&& func, Args&&... args);
}

#endif
