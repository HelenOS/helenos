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

#ifndef LIBCPP_BITS_THREAD
#define LIBCPP_BITS_THREAD

#include <__bits/thread/threading.hpp>
#include <chrono>
#include <ostream>

namespace std
{
    namespace aux
    {
        template<class Callable>
        int thread_main(void*);

        /**
         * Fibrils in HelenOS are not joinable. They were
         * in the past, but that functionality was removed from them
         * so we created a workaround using a conditional variable that
         * comprises the following two wrapper classes.
         */
        class joinable_wrapper
        {
            public:
                joinable_wrapper()
                    : join_mtx_{}, join_cv_{},
                      finished_{false}, detached_{false}
                {
                    aux::threading::mutex::init(join_mtx_);
                    aux::threading::condvar::init(join_cv_);
                }

                void join()
                {
                    aux::threading::mutex::lock(join_mtx_);
                    while (!finished_)
                        aux::threading::condvar::wait(join_cv_, join_mtx_);
                    aux::threading::mutex::unlock(join_mtx_);
                }

                bool finished() const
                {
                    return finished_;
                }

                void detach()
                {
                    detached_ = true;
                }

                bool detached() const
                {
                    return detached_;
                }

            protected:
                aux::mutex_t join_mtx_;
                aux::condvar_t join_cv_;
                bool finished_;
                bool detached_;
        };

        template<class Callable>
        class callable_wrapper: public joinable_wrapper
        {
            public:
                callable_wrapper(Callable&& clbl)
                    : joinable_wrapper{}, callable_{forward<Callable>(clbl)}
                { /* DUMMY BODY */ }

                void operator()()
                {
                    callable_();

                    aux::threading::mutex::lock(join_mtx_);
                    finished_ = true;
                    aux::threading::mutex::unlock(join_mtx_);

                    aux::threading::condvar::broadcast(join_cv_);
                }

            private:
                Callable callable_;
        };
    }

    /**
     * 30.3.1, class thread:
     */

    class thread
    {
        public:
            class id;

            using native_handle_type = aux::thread_t*;

            /**
             * 30.3.1.2, thread constructors:
             * 30.3.1.3, thread destructor:
             * 30.3.1.4, thread assignment:
             */

            thread() noexcept;

            ~thread();

            // TODO: check the remark in the standard
            template<class F, class... Args>
            explicit thread(F&& f, Args&&... args)
                : id_{}
            {
                auto callable = [=](){
                    return f(forward<Args>(args)...);
                };

                auto callable_wrapper = new aux::callable_wrapper<decltype(callable)>{move(callable)};
                joinable_wrapper_ = static_cast<aux::joinable_wrapper*>(callable_wrapper);

                id_ = aux::threading::thread::create(
                    aux::thread_main<decltype(callable_wrapper)>,
                    *callable_wrapper
                );

                aux::threading::thread::start(id_);
                // TODO: fibrils are weird here, 2 returns with same thread ids
            }

            thread(const thread&) = delete;
            thread& operator=(const thread&) = delete;

            thread(thread&& other) noexcept;
            thread& operator=(thread&& other) noexcept;

            /**
             * 30.3.1.5, thread members:
             */

            void swap(thread& other) noexcept;

            bool joinable() const noexcept;

            void join();

            void detach();

            id get_id() const noexcept;

            native_handle_type native_handle();

            static unsigned hardware_concurrency() noexcept;

        private:
            aux::thread_t id_;
            aux::joinable_wrapper* joinable_wrapper_{nullptr};

            template<class Callable>
            friend int aux::thread_main(void*);
    };

    namespace aux
    {
        template<class CallablePtr>
        int thread_main(void* clbl)
        {
            if (!clbl)
                return 1;

            auto callable = static_cast<CallablePtr>(clbl);
            (*callable)();

            if (callable->detached())
                delete callable;

            return 0;
        }
    }

    void swap(thread& x, thread& y) noexcept;

    /**
     * 30.3.2, namespace this_thread:
     */

    namespace this_thread
    {
        thread::id get_id() noexcept;

        void yield() noexcept;

        template<class Clock, class Duration>
        void sleep_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
            auto now = Clock::now();

            auto time = aux::threading::time::convert(abs_time - now);
            aux::threading::time::sleep(time);
        }

        template<class Rep, class Period>
        void sleep_for(const chrono::duration<Rep, Period>& rel_time)
        {
            if (rel_time <= chrono::duration<Rep, Period>::zero())
                return;

            // TODO: timeouts?
            auto time = aux::threading::time::convert(rel_time);
            aux::threading::time::sleep(time);
        }
    }

    template<class T>
    struct hash;

    class thread::id
    {
        public:
            constexpr id() noexcept
                : id_{}
            { /* DUMMY BODY */ }

        private:
            aux::thread_t id_;

            id(aux::thread_t id)
                : id_{id}
            { /* DUMMY BODY */ }

            friend class thread;

            friend bool operator==(thread::id, thread::id) noexcept;
            friend bool operator!=(thread::id, thread::id) noexcept;
            friend bool operator<(thread::id, thread::id) noexcept;
            friend bool operator<=(thread::id, thread::id) noexcept;
            friend bool operator>(thread::id, thread::id) noexcept;
            friend bool operator>=(thread::id, thread::id) noexcept;

            template<class Char, class Traits>
            friend basic_ostream<Char, Traits>& operator<<(
                basic_ostream<Char, Traits>&, thread::id);

            friend struct hash<id>;

            friend id this_thread::get_id() noexcept;
    };

    bool operator==(thread::id lhs, thread::id rhs) noexcept;
    bool operator!=(thread::id lhs, thread::id rhs) noexcept;
    bool operator<(thread::id lhs, thread::id rhs) noexcept;
    bool operator<=(thread::id lhs, thread::id rhs) noexcept;
    bool operator>(thread::id lhs, thread::id rhs) noexcept;
    bool operator>=(thread::id lhs, thread::id rhs) noexcept;

    template<class Char, class Traits>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& out, thread::id id)
    {
        out << id.id_;

        return out;
    }

    template<> // TODO: implement
    struct hash<thread::id>;
}

#endif
