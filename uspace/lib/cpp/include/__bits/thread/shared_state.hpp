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

#ifndef LIBCPP_BITS_THREAD_SHARED_STATE
#define LIBCPP_BITS_THREAD_SHARED_STATE

/**
 * 30.6.4, shared state:
 */

#include <__bits/exception.hpp>
#include <__bits/functional/function.hpp>
#include <__bits/functional/invoke.hpp>
#include <__bits/refcount_obj.hpp>
#include <__bits/thread/future_common.hpp>
#include <__bits/thread/threading.hpp>
#include <cerrno>
#include <thread>
#include <tuple>

namespace std::aux
{
    class shared_state_base: public aux::refcount_obj
    {
        public:
            shared_state_base()
                : mutex_{}, condvar_{}, value_set_{false},
                  exception_{}, has_exception_{false}
            {
                threading::mutex::init(mutex_);
                threading::condvar::init(condvar_);
            }

            void destroy() override
            {
                /**
                 * Note: No need to act in this case, async shared
                 *       state is the object that needs to sometimes
                 *       invoke its payload.
                 */
            }

            void mark_set(bool set = true) noexcept
            {
                value_set_ = set;
            }

            bool is_set() const noexcept
            {
                return value_set_;
            }

            void set_exception(exception_ptr ptr, bool set = true)
            {
                exception_ = ptr;
                has_exception_ = set;
            }

            bool has_exception() const noexcept
            {
                return has_exception_;
            }

            void throw_stored_exception() const
            {
                if (has_exception_)
                    rethrow_exception(exception_);
            }

            virtual void wait() const
            {
                aux::threading::mutex::lock(
                    const_cast<aux::mutex_t&>(mutex_)
                );

                while (!value_set_)
                {
                    aux::threading::condvar::wait(
                        const_cast<aux::condvar_t&>(condvar_),
                        const_cast<aux::mutex_t&>(mutex_)
                    );
                }

                aux::threading::mutex::unlock(
                    const_cast<aux::mutex_t&>(mutex_)
                );
            }

            template<class Rep, class Period>
            future_status
            wait_for(const chrono::duration<Rep, Period>& rel_time) const
            {
                if (value_set_)
                    return future_status::ready;

                aux::threading::mutex::lock(
                    const_cast<aux::mutex_t&>(mutex_)
                );

                auto res = timed_wait_(
                    aux::threading::time::convert(rel_time)
                );

                aux::threading::mutex::unlock(
                    const_cast<aux::mutex_t&>(mutex_)
                );

                return res;
            }

            template<class Clock, class Duration>
            future_status
            wait_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
                if (value_set_)
                    return future_status::ready;

                aux::threading::mutex::lock(
                    const_cast<aux::mutex_t&>(mutex_)
                );

                auto res = timed_wait_(
                    aux::threading::time::convert(abs_time - Clock::now())
                );

                aux::threading::mutex::unlock(
                    const_cast<aux::mutex_t&>(mutex_)
                );

                return res;
            }

            ~shared_state_base() override = default;

        protected:
            aux::mutex_t mutex_;
            aux::condvar_t condvar_;

            bool value_set_;

            exception_ptr exception_;
            bool has_exception_;

            /**
             * Note: wait_for and wait_until are templates and as such
             *       cannot be virtual and overriden by the deferred_ and
             *       async_ children. However, we are using aux::time_unit_t
             *       in the end anyway, so we can work around that
             *       by using the 'template method' design pattern
             *       (i.e. by providing a virtual function called by these
             *       templates and then overriding that function in the
             *       children).
             */
            virtual future_status timed_wait_(aux::time_unit_t time) const
            {
                auto res = aux::threading::condvar::wait_for(
                    const_cast<aux::condvar_t&>(condvar_),
                    const_cast<aux::mutex_t&>(mutex_), time
                );

                return res == ETIMEOUT ? future_status::timeout
                                       : future_status::ready;
            }
    };

    template<class R>
    class shared_state: public shared_state_base
    {
        public:
            shared_state()
                : shared_state_base{}
            { /* DUMMY BODY */ }

            void set_value(const R& val, bool set)
            {
                /**
                 * Note: This is the 'mark ready' move described
                 *       in 30.6.4 (6).
                 */

                aux::threading::mutex::lock(mutex_);
                value_ = val;
                value_set_ = set;
                aux::threading::mutex::unlock(mutex_);

                if (set)
                    aux::threading::condvar::broadcast(condvar_);
            }

            void set_value(R&& val, bool set = true)
            {
                aux::threading::mutex::lock(mutex_);
                value_ = std::move(val);
                value_set_ = set;
                aux::threading::mutex::unlock(mutex_);

                if (set)
                    aux::threading::condvar::broadcast(condvar_);
            }

            R& get()
            {
                return value_;
            }

        protected:
            R value_;
    };

    template<>
    class shared_state<void>: public shared_state_base
    {
        public:
            shared_state()
                : shared_state_base{}
            { /* DUMMY BODY */ }

            void set_value()
            {
                value_set_ = true;
                aux::threading::condvar::broadcast(condvar_);
            }

            void get()
            { /* DUMMY BODY */ }
    };

    /**
     * We could make one state for both async and
     * deferred policies, but then we would be wasting
     * memory and the only benefit would be the ability
     * for additional implementation defined policies done
     * directly in that state (as opposed to making new
     * states for them).
     *
     * But since we have no plan (nor need) to make those,
     * this approach seems to be the best one.
     *
     * Also note that unlike the parent class shared_state,
     * we do not need to specialize these for void. This is because
     * the difference in class contents are handled by the parent
     * specialization and setting the value can be handled easily
     * by if constexpr and checking for the equivalence of the
     * R template parameter and void.
     */

    template<class R, class F, class... Args>
    class async_shared_state: public shared_state<R>
    {
        public:
            async_shared_state(F&& f, Args&&... args)
                : shared_state<R>{}, thread_{}
            {
                thread_ = thread{
                    [=](){
                        try
                        {
                            if constexpr (!is_same_v<R, void>)
                                this->set_value(invoke(f, args...));
                            else
                            {
                                invoke(f, args...);
                                this->mark_set(true);
                            }
                        }
                        catch(const exception& __exception)
                        {
                            this->set_exception(make_exception_ptr(__exception));
                        }
                    }
                };
            }

            void destroy() override
            {
                if (!this->is_set())
                    thread_.join();
            }

            void wait() const override
            {
                if (!this->is_set())
                    const_cast<thread&>(thread_).join();
            }

            ~async_shared_state() override
            {
                destroy();
            }

        protected:
            future_status timed_wait_(aux::time_unit_t time) const override
            {
                /**
                 * Note: Currently we have no timed join, but this
                 *       behaviour should be compliant.
                 */
                aux::threading::time::sleep(time);
                if (this->value_set_)
                    return future_status::ready;
                else
                    return future_status::timeout;
            }

        private:
            thread thread_;
    };

    template<class R, class F, class... Args>
    class deferred_shared_state: public shared_state<R>
    {
        public:
            template<class G>
            deferred_shared_state(G&& f, Args&&... args)
                : shared_state<R>{}, func_{forward<F>(f)},
                  args_{forward<Args>(args)...}
            { /* DUMMY BODY */ }

            void destroy() override
            {
                aux::threading::mutex::lock(this->mutex_);
                if (!this->is_set())
                    invoke_(make_index_sequence<sizeof...(Args)>{});
                aux::threading::mutex::unlock(this->mutex_);
            }

            void wait() const override
            {
                /**
                 * Note: Synchronization done in invoke_ -> set_value.
                 */
                if (!this->is_set())
                {
                    const_cast<
                        deferred_shared_state<R, F, Args...>*
                    >(this)->invoke_(make_index_sequence<sizeof...(Args)>{});
                }
            }

            ~deferred_shared_state() override
            {
                destroy();
            }

        protected:
            function<R(decay_t<Args>...)> func_;
            tuple<decay_t<Args>...> args_;

            template<size_t... Is>
            void invoke_(index_sequence<Is...>)
            {
                try
                {
                    if constexpr (!is_same_v<R, void>)
                        this->set_value(invoke(move(func_), get<Is>(move(args_))...));
                    else
                    {
                        invoke(move(func_), get<Is>(move(args_))...);
                        this->mark_set(true);
                    }
                }
                catch(const exception& __exception)
                {
                    this->set_exception(make_exception_ptr(__exception));
                }
            }

            future_status timed_wait_(aux::time_unit_t) const override
            {
                /**
                 * Note: Neither of the wait_ functions has any effect
                 *       for deferred functions spawned by async (which
                 *       are the only users of this state type).
                 */
                return future_status::deferred;
            }
    };

    /**
     * Note: The following two functions should:
     *   1) Increment refcount.
     *   2) Store ptr to a vector of shared_state_base ptrs
     *      (as those have ::mark_set member functions).
     *   3) If not done already, register a function
     *      executing all these in the thread_atexit function
     *      once that is implemented.
     */

    template<class R>
    void set_state_value_at_thread_exit(shared_state<R>* state)
    {
        // TODO: implement
        __unimplemented();
    }

    template<class R>
    void set_state_exception_at_thread_exit(shared_state<R>* state)
    {
        // TODO: implement
        __unimplemented();
    }
}

#endif
