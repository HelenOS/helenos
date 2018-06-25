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

#ifndef LIBCPP_THREAD_SHARED_MUTEX
#define LIBCPP_THREAD_SHARED_MUTEX

#include <__bits/thread/threading.hpp>
#include <chrono>
#include <mutex>

namespace std
{
    /**
     * 30.4.1.4.1, class shared_timed_mutex:
     */

    class shared_timed_mutex
    {
        public:
            shared_timed_mutex() noexcept;
            ~shared_timed_mutex();

            shared_timed_mutex(const shared_timed_mutex&) = delete;
            shared_timed_mutex& operator=(const shared_timed_mutex&) = delete;

            void lock();
            bool try_lock();
            void unlock();

            template<class Rep, class Period>
            bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
            {
                auto time = aux::threading::time::convert(rel_time);

                return aux::threading::shared_mutex::try_lock_for(time);
            }

            template<class Clock, class Duration>
            bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
                auto dur = (abs_time - Clock::now());
                auto time = aux::threading::time::convert(dur);

                return aux::threading::shared_mutex::try_lock_for(time);
            }

            void lock_shared();
            bool try_lock_shared();
            void unlock_shared();

            template<class Rep, class Period>
            bool try_lock_shared_for(const chrono::duration<Rep, Period>& rel_time)
            {
                auto time = aux::threading::time::convert(rel_time);

                return aux::threading::shared_mutex::try_lock_shared_for(time);
            }

            template<class Clock, class Duration>
            bool try_lock_shared_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
                auto dur = (abs_time - Clock::now());
                auto time = aux::threading::time::convert(dur);

                return aux::threading::shared_mutex::try_lock_shared_for(time);
            }

            using native_handle_type = aux::shared_mutex_t*;
            native_handle_type native_handle();

        private:
            aux::shared_mutex_t mtx_;
    };

    /**
     * 30.4.2.3, class template shared_lock:
     */

    template<class Mutex>
    class shared_lock
    {
        public:
            using mutex_type = Mutex;

            /**
             * 30.4.2.2.1, construction/copy/destroy:
             */

            shared_lock() noexcept
                : mtx_{nullptr}, owns_{false}
            { /* DUMMY BODY */ }

            explicit shared_lock(mutex_type& mtx)
                : mtx_{&mtx}, owns_{true}
            {
                mtx_->lock_shared();
            }

            shared_lock(mutex_type& mtx, defer_lock_t) noexcept
                : mtx_{&mtx}, owns_{false}
            { /* DUMMY BODY */ }

            shared_lock(mutex_type& mtx, try_to_lock_t)
                : mtx_{&mtx}, owns_{}
            {
                owns_ = mtx_->try_lock_shared();
            }

            shared_lock(mutex_type& mtx, adopt_lock_t)
                : mtx_{&mtx}, owns_{true}
            { /* DUMMY BODY */ }

            template<class Clock, class Duration>
            shared_lock(mutex_type& mtx, const chrono::time_point<Clock, Duration>& abs_time)
                : mtx_{&mtx}, owns_{}
            {
                owns_ = mtx_->try_lock_shared_until(abs_time);
            }

            template<class Rep, class Period>
            shared_lock(mutex_type& mtx, const chrono::duration<Rep, Period>& rel_time)
                : mtx_{&mtx}, owns_{}
            {
                owns_ = mtx_->try_lock_shared_for(rel_time);
            }

            ~shared_lock()
            {
                if (owns_)
                    mtx_->unlock_shared();
            }

            shared_lock(const shared_lock&) = delete;
            shared_lock& operator=(const shared_lock&) = delete;

            shared_lock(shared_lock&& other) noexcept
                : mtx_{move(other.mtx_)}, owns_{move(other.owns_)}
            {
                other.mtx_ = nullptr;
                other.owns_ = false;
            }

            shared_lock& operator=(shared_lock&& other)
            {
                if (owns_)
                    mtx_->unlock_shared();

                mtx_ = move(other.mtx_);
                owns_ = move(other.owns_);

                other.mtx_ = nullptr;
                other.owns_ = false;
            }

            /**
             * 30.4.2.2.2, locking:
             */

            void lock()
            {
                /**
                 * TODO:
                 * throw system_error operation_not_permitted if mtx_ == nullptr
                 * throw system_error resource_deadlock_would_occur if owns_ == true
                 */

                mtx_->lock_shared();
                owns_ = true;
            }

            bool try_lock()
            {
                /**
                 * TODO:
                 * throw system_error operation_not_permitted if mtx_ == nullptr
                 * throw system_error resource_deadlock_would_occur if owns_ == true
                 */

                owns_ = mtx_->try_lock_shared();

                return owns_;
            }

            template<class Rep, class Period>
            bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
            {
                /**
                 * TODO:
                 * throw system_error operation_not_permitted if mtx_ == nullptr
                 * throw system_error resource_deadlock_would_occur if owns_ == true
                 */

                owns_ = mtx_->try_lock_shared_for(rel_time);

                return owns_;
            }

            template<class Clock, class Duration>
            bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
                /**
                 * TODO:
                 * throw system_error operation_not_permitted if mtx_ == nullptr
                 * throw system_error resource_deadlock_would_occur if owns_ == true
                 */

                owns_ = mtx_->try_lock_shared_until(abs_time);

                return owns_;
            }

            void unlock()
            {
                /**
                 * TODO:
                 * throw system_error operation_not_permitted if owns_ == false
                 */

                mtx_->unlock_shared();
            }

            /**
             * 30.4.2.2.3, modifiers:
             */

            void swap(shared_lock& other) noexcept
            {
                std::swap(mtx_, other.mtx_);
                std::swap(owns_, other.owns_);
            }

            mutex_type* release() noexcept
            {
                auto ret = mtx_;
                mtx_ = nullptr;
                owns_ = false;

                return ret;
            }

            /**
             * 30.4.2.2.4, observers:
             */

            bool owns_lock() const noexcept
            {
                return owns_;
            }

            explicit operator bool() const noexcept
            {
                return owns_;
            }

            mutex_type* mutex() const noexcept
            {
                return mtx_;
            }

        private:
            mutex_type* mtx_;
            bool owns_;
    };

    template<class Mutex>
    void swap(shared_lock<Mutex>& lhs, shared_lock<Mutex>& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}

#endif
