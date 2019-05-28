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

#ifndef LIBCPP_BITS_THREAD_CONDITION_VARIABLE
#define LIBCPP_BITS_THREAD_CONDITION_VARIABLE

#include <__bits/thread/threading.hpp>
#include <mutex>

namespace std
{
    enum class cv_status
    {
        no_timeout,
        timeout
    };

    namespace aux
    {
        template<class Clock, class Duration>
        aux::time_unit_t time_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
            return aux::threading::time::convert(abs_time - Clock::now());
        }
    }

    /**
     * 30.5.1, class condition_variable:
     */

    class condition_variable
    {
        public:
            condition_variable();
            ~condition_variable();

            condition_variable(const condition_variable&) = delete;
            condition_variable& operator=(const condition_variable&) = delete;

            void notify_one() noexcept;
            void notify_all() noexcept;

            void wait(unique_lock<mutex>&);

            template<class Predicate>
            void wait(unique_lock<mutex>& lock, Predicate pred)
            {
                /**
                 * Note: lock is supposed to be locked here,
                 *       so no need to lock it.
                 */
                while (!pred())
                    wait(lock);
            }

            template<class Clock, class Duration>
            cv_status wait_until(unique_lock<mutex>& lock,
                                 const chrono::time_point<Clock, Duration>& abs_time)
            {
                auto ret = aux::threading::condvar::wait_for(
                    cv_, *lock.mutex()->native_handle(), aux::time_until(abs_time)
                );

                if (ret == EOK)
                    return cv_status::no_timeout;
                else
                    return cv_status::timeout;
            }

            template<class Clock, class Duration, class Predicate>
            bool wait_until(unique_lock<mutex>& lock,
                            const chrono::time_point<Clock, Duration>& abs_time,
                            Predicate pred)
            {
                while (!pred())
                {
                    if (wait_until(lock, abs_time) == cv_status::timeout)
                        return pred();
                }

                return true;
            }

            template<class Rep, class Period>
            cv_status wait_for(unique_lock<mutex>& lock,
                               const chrono::duration<Rep, Period>& rel_time)
            {
                return wait_until(
                    lock, chrono::steady_clock::now() + rel_time
                );
            }

            template<class Rep, class Period, class Predicate>
            bool wait_for(unique_lock<mutex>& lock,
                          const chrono::duration<Rep, Period>& rel_time,
                          Predicate pred)
            {
                return wait_until(
                    lock, chrono::steady_clock::now() + rel_time,
                    move(pred)
                );
            }

            using native_handle_type = aux::condvar_t*;
            native_handle_type native_handle();

        private:
            aux::condvar_t cv_;
    };

    /**
     * 30.5.2, class condition_variable_any:
     */

    class condition_variable_any
    {
        public:
            condition_variable_any();
            ~condition_variable_any();

            condition_variable_any(const condition_variable_any&) = delete;
            condition_variable_any& operator=(const condition_variable_any&) = delete;

            void notify_one() noexcept;
            void notify_all() noexcept;

            template<class Lock>
            void wait(Lock& lock)
            {
                aux::threading::condvar::wait(cv_, *lock.native_handle());
            }

            template<class Lock, class Predicate>
            void wait(Lock& lock, Predicate pred)
            {
                while (!pred())
                    wait(lock);
            }

            template<class Lock, class Clock, class Duration>
            cv_status wait_until(Lock& lock,
                                 const chrono::time_point<Clock, Duration>& abs_time)
            {
                auto ret = aux::threading::condvar::wait_for(
                    cv_, *lock.mutex()->native_handle(), aux::time_until(abs_time)
                );

                if (ret == EOK)
                    return cv_status::no_timeout;
                else
                    return cv_status::timeout;
            }

            template<class Lock, class Clock, class Duration, class Predicate>
            bool wait_until(Lock& lock,
                            const chrono::time_point<Clock, Duration>& abs_time,
                            Predicate pred)
            {
                while (!pred())
                {
                    if (wait_until(lock, abs_time) == cv_status::timeout)
                        return pred();
                }

                return true;
            }

            template<class Lock, class Rep, class Period>
            cv_status wait_for(Lock& lock,
                               const chrono::duration<Rep, Period>& rel_time)
            {
                return wait_until(
                    lock, chrono::steady_clock::now() + rel_time
                );
            }

            template<class Lock, class Rep, class Period, class Predicate>
            bool wait_for(Lock& lock,
                          const chrono::duration<Rep, Period>& rel_time,
                          Predicate pred)
            {
                return wait_until(
                    lock, chrono::steady_clock::now() + rel_time,
                    move(pred)
                );
            }

            using native_handle_type = aux::condvar_t*;
            native_handle_type native_handle();

        private:
            aux::condvar_t cv_;
    };

    void notify_all_at_thread_exit(condition_variable&, unique_lock<mutex>&);
}

#endif
