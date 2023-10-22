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

#ifndef LIBCPP_BITS_THREAD_THREADING
#define LIBCPP_BITS_THREAD_THREADING

#include <chrono>

#include <fibril.h>
#include <fibril_synch.h>

namespace std::aux
{
    struct fibril_tag
    { /* DUMMY BODY */ };

    struct thread_tag
    { /* DUMMY BODY */ };

    template<class>
    struct threading_policy;

    template<>
    struct threading_policy<fibril_tag>
    {
        using mutex_type        = ::helenos::fibril_mutex_t;
        using thread_type       = ::helenos::fid_t;
        using condvar_type      = ::helenos::fibril_condvar_t;
        using time_unit         = ::helenos::usec_t;
        using shared_mutex_type = ::helenos::fibril_rwlock_t;

        struct thread
        {
            template<class Callable, class Payload>
            static thread_type create(Callable clbl, Payload& pld)
            {
                return ::helenos::fibril_create(clbl, (void*)&pld);
            }

            static void start(thread_type thr)
            {
                ::helenos::fibril_add_ready(thr);
            }

            static thread_type this_thread()
            {
                return ::helenos::fibril_get_id();
            }

            static void yield()
            {
                ::helenos::fibril_yield();
            }

            /**
             * Note: join & detach are performed at the C++
             *       level at the moment, but eventually should
             *       be moved here once joinable fibrils are in libc.
             */
        };

        struct mutex
        {
            static constexpr void init(mutex_type& mtx)
            {
                ::helenos::fibril_mutex_initialize(&mtx);
            }

            static void lock(mutex_type& mtx)
            {
                ::helenos::fibril_mutex_lock(&mtx);
            }

            static void unlock(mutex_type& mtx)
            {
                ::helenos::fibril_mutex_unlock(&mtx);
            }

            static bool try_lock(mutex_type& mtx)
            {
                return ::helenos::fibril_mutex_trylock(&mtx);
            }

            static bool try_lock_for(mutex_type& mtx, time_unit timeout)
            {
                // TODO: we need fibril_mutex_trylock_for() :/
                return try_lock(mtx);
            }
        };

        struct condvar
        {
            static void init(condvar_type& cv)
            {
                ::helenos::fibril_condvar_initialize(&cv);
            }

            static void wait(condvar_type& cv, mutex_type& mtx)
            {
                ::helenos::fibril_condvar_wait(&cv, &mtx);
            }

            static int wait_for(condvar_type& cv, mutex_type& mtx, time_unit timeout)
            {
                return ::helenos::fibril_condvar_wait_timeout(&cv, &mtx, timeout);
            }

            static void signal(condvar_type& cv)
            {
                ::helenos::fibril_condvar_signal(&cv);
            }

            static void broadcast(condvar_type& cv)
            {
                ::helenos::fibril_condvar_broadcast(&cv);
            }
        };

        struct time
        {
            template<class Rep, class Period>
            static time_unit convert(const std::chrono::duration<Rep, Period>& dur)
            {
                return std::chrono::duration_cast<std::chrono::duration<Rep, micro>>(dur).count();
            }

            static void sleep(time_unit time)
            {
                ::helenos::fibril_usleep(time);
            }
        };

        struct shared_mutex
        {
            static void init(shared_mutex_type& mtx)
            {
                ::helenos::fibril_rwlock_initialize(&mtx);
            }

            static void lock(shared_mutex_type& mtx)
            {
                ::helenos::fibril_rwlock_write_lock(&mtx);
            }

            static void unlock(shared_mutex_type& mtx)
            {
                ::helenos::fibril_rwlock_write_unlock(&mtx);
            }

            static void lock_shared(shared_mutex_type& mtx)
            {
                ::helenos::fibril_rwlock_read_lock(&mtx);
            }

            static void unlock_shared(shared_mutex_type& mtx)
            {
                ::helenos::fibril_rwlock_read_unlock(&mtx);
            }

            static bool try_lock(shared_mutex_type& mtx)
            {
                // TODO: rwlocks don't have try locking capabilities
                lock(mtx);

                return true;
            }

            static bool try_lock_shared(shared_mutex_type& mtx)
            {
                lock(mtx);

                return true;
            }

            static bool try_lock_for(shared_mutex_type& mtx, time_unit timeout)
            {
                return try_lock(mtx);
            }

            static bool try_lock_shared_for(shared_mutex_type& mtx, time_unit timeout)
            {
                return try_lock(mtx);
            }
        };
    };

    template<>
    struct threading_policy<thread_tag>
    {
        // TODO:
    };

    using default_tag = fibril_tag;
    using threading = threading_policy<default_tag>;

    using thread_t       = typename threading::thread_type;
    using mutex_t        = typename threading::mutex_type;
    using condvar_t      = typename threading::condvar_type;
    using time_unit_t    = typename threading::time_unit;
    using shared_mutex_t = typename threading::shared_mutex_type;
}

#endif
