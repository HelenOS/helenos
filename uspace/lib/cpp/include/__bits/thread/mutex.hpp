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

#ifndef LIBCPP_BITS_THREAD_MUTEX
#define LIBCPP_BITS_THREAD_MUTEX

#include <__bits/thread/threading.hpp>
#include <functional>
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

            ~mutex() = default;

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
            constexpr recursive_mutex() noexcept
                : mtx_{}, lock_level_{}, owner_{}
            {
                aux::threading::mutex::init(mtx_);
            }

            ~recursive_mutex();

            recursive_mutex(const recursive_mutex&) = delete;
            recursive_mutex& operator=(const recursive_mutex&) = delete;

            void lock();
            bool try_lock() noexcept;
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

    class timed_mutex
    {
        public:
            timed_mutex() noexcept;
            ~timed_mutex();

            timed_mutex(const timed_mutex&) = delete;
            timed_mutex& operator=(const timed_mutex&) = delete;

            void lock();
            bool try_lock();
            void unlock();

            template<class Rep, class Period>
            bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
            {
                auto time = aux::threading::time::convert(rel_time);

                return aux::threading::mutex::try_lock_for(time);
            }

            template<class Clock, class Duration>
            bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
                auto dur = (abs_time - Clock::now());
                auto time = aux::threading::time::convert(dur);

                return aux::threading::mutex::try_lock_for(time);
            }

            using native_handle_type = aux::mutex_t*;
            native_handle_type native_handle();

        private:
            aux::mutex_t mtx_;
    };

    /**
     * 30.4.1.3.2, class recursive_timed_mutex:
     */

    class recursive_timed_mutex
    {
        public:
            recursive_timed_mutex() noexcept
                : mtx_{}, lock_level_{}, owner_{}
            {
                aux::threading::mutex::init(mtx_);
            }

            ~recursive_timed_mutex();

            recursive_timed_mutex(const recursive_timed_mutex&) = delete;
            recursive_timed_mutex& operator=(const recursive_timed_mutex&) = delete;

            void lock();
            bool try_lock() noexcept;
            void unlock();

            template<class Rep, class Period>
            bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
            {
                if (owner_ == this_thread::get_id())
                    return true;

                auto time = aux::threading::time::convert(rel_time);
                auto ret = aux::threading::mutex::try_lock_for(time);

                if (ret)
                    ++lock_level_;
                return ret;
            }

            template<class Clock, class Duration>
            bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
                if (owner_ == this_thread::get_id())
                    return true;

                auto dur = (abs_time - Clock::now());
                auto time = aux::threading::time::convert(dur);
                auto ret = aux::threading::mutex::try_lock_for(time);

                if (ret)
                    ++lock_level_;
                return ret;
            }

            using native_handle_type = aux::mutex_t*;
            native_handle_type native_handle();

        private:
            aux::mutex_t mtx_;
            size_t lock_level_;
            thread::id owner_;
    };

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
    class unique_lock
    {
        public:
            using mutex_type = Mutex;

            /**
             * 30.4.2.2.1, construction/copy/destroy:
             */

            unique_lock() noexcept
                : mtx_{nullptr}, owns_{false}
            { /* DUMMY BODY */ }

            explicit unique_lock(mutex_type& mtx)
                : mtx_{&mtx}, owns_{true}
            {
                mtx_->lock();
            }

            unique_lock(mutex_type& mtx, defer_lock_t) noexcept
                : mtx_{&mtx}, owns_{false}
            { /* DUMMY BODY */ }

            unique_lock(mutex_type& mtx, try_to_lock_t)
                : mtx_{&mtx}, owns_{}
            {
                owns_ = mtx_->try_lock();
            }

            unique_lock(mutex_type& mtx, adopt_lock_t)
                : mtx_{&mtx}, owns_{true}
            { /* DUMMY BODY */ }

            template<class Clock, class Duration>
            unique_lock(mutex_type& mtx, const chrono::time_point<Clock, Duration>& abs_time)
                : mtx_{&mtx}, owns_{}
            {
                owns_ = mtx_->try_lock_until(abs_time);
            }

            template<class Rep, class Period>
            unique_lock(mutex_type& mtx, const chrono::duration<Rep, Period>& rel_time)
                : mtx_{&mtx}, owns_{}
            {
                owns_ = mtx_->try_lock_for(rel_time);
            }

            ~unique_lock()
            {
                if (owns_)
                    mtx_->unlock();
            }

            unique_lock(const unique_lock&) = delete;
            unique_lock& operator=(const unique_lock&) = delete;

            unique_lock(unique_lock&& other) noexcept
                : mtx_{move(other.mtx_)}, owns_{move(other.owns_)}
            {
                other.mtx_ = nullptr;
                other.owns_ = false;
            }

            unique_lock& operator=(unique_lock&& other)
            {
                if (owns_)
                    mtx_->unlock();

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

                mtx_->lock();
                owns_ = true;
            }

            bool try_lock()
            {
                /**
                 * TODO:
                 * throw system_error operation_not_permitted if mtx_ == nullptr
                 * throw system_error resource_deadlock_would_occur if owns_ == true
                 */

                owns_ = mtx_->try_lock();

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

                owns_ = mtx_->try_lock_for(rel_time);

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

                owns_ = mtx_->try_lock_until(abs_time);

                return owns_;
            }

            void unlock()
            {
                /**
                 * TODO:
                 * throw system_error operation_not_permitted if owns_ == false
                 */

                mtx_->unlock();
            }

            /**
             * 30.4.2.2.3, modifiers:
             */

            void swap(unique_lock& other) noexcept
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
    void swap(unique_lock<Mutex>& lhs, unique_lock<Mutex>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    namespace aux
    {
        template<class L>
        int try_lock_tail(int idx, L& l)
        {
            if (!l.try_lock())
                return idx;
            else
                return -1;
        }

        template<class L1, class... L2>
        int try_lock_tail(int idx, L1& l1, L2&... ls)
        {
            if (!l1.try_lock())
                return idx;

            auto ret = try_lock_tail(idx + 1, ls...);
            if (ret != -1)
                l1.unlock();

            return ret;
        }
    }

    template<class L1, class L2, class... L3>
    int try_lock(L1& l1, L2& l2, L3&... ls)
    {
        return aux::try_lock_tail(0, l1, l2, ls...);
    }

    namespace aux
    {
        template<class L>
        bool lock_tail(L& l)
        {
            return l.try_lock();
        }

        template<class L1, class... L2>
        bool lock_tail(L1& l1, L2&... ls)
        {
            if (l1.try_lock())
            {
                auto ret = lock_tail(ls...);
                if (ret)
                    return true;

                l1.unlock();
            }

            return false;
        }
    }

    template<class L1, class L2, class... L3>
    void lock(L1& l1, L2& l2, L3&... ls)
    {
        do
        {
            l1.lock();

            if (aux::lock_tail(l2, ls...))
                return;
            l1.unlock();
        } while (true);
    }

    struct once_flag
    {
        constexpr once_flag() noexcept
            : called_{false}, mtx_{}
        { /* DUMMY BODY */ }

        once_flag(const once_flag&) = delete;
        once_flag& operator=(const once_flag&) = delete;

        private:
            bool called_;
            mutex mtx_;

            template<class Callable, class... Args>
            friend void call_once(once_flag&, Callable&&, Args&&...);
    };

    template<class Callable, class... Args>
    void call_once(once_flag& flag, Callable&& func, Args&&... args)
    {
        flag.mtx_.lock();
        if (!flag.called_)
        {
            // TODO: exception handling

            aux::INVOKE(forward<Callable>(func), forward<Args>(args)...);
            flag.called_ = true;
        }
        flag.mtx_.unlock();
    }
}

#endif
