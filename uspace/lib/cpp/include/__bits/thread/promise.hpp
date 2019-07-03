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

#ifndef LIBCPP_BITS_THREAD_PROMISE
#define LIBCPP_BITS_THREAD_PROMISE

#include <__bits/exception.hpp>
#include <__bits/memory/allocator_traits.hpp>
#include <__bits/thread/future.hpp>
#include <__bits/thread/shared_state.hpp>
#include <__bits/aux.hpp>
#include <utility>

namespace std
{
    /**
     * 30.6.5, class template promise:
     */

    namespace aux
    {
        template<class R>
        class promise_base
        {
            public:
                promise_base()
                    : state_{new aux::shared_state<R>{}}
                { /* DUMMY BODY */ }

                template<class Allocator>
                promise_base(allocator_arg_t, const Allocator& a)
                    : state_{}
                {
                    typename allocator_traits<
                        Allocator
                    >::template rebind_alloc<aux::shared_state<R>> rebound{a};

                    state_ = rebound.allocate(1);
                    rebound.construct(state_);
                }

                promise_base(promise_base&& rhs) noexcept
                    : state_{}
                {
                    state_ = rhs.state_;
                    rhs.state_ = nullptr;
                }

                promise_base(const promise_base&) = delete;

                virtual ~promise_base()
                {
                    this->abandon_state_();
                }

                promise_base& operator=(promise_base&& rhs) noexcept
                {
                    abandon_state_();
                    promise_base{move(rhs)}.swap(*this);

                    return *this;
                }

                promise_base& operator=(const promise_base&) = delete;

                void swap(promise_base& other) noexcept
                {
                    std::swap(state_, other.state_);
                }

                void set_exception(exception_ptr ptr)
                {
                    assert(state_);

                    state_->set_exception(ptr);
                }

                void set_exception_at_thread_exit(exception_ptr ptr)
                {
                    assert(state_);

                    state_->set_exception_ptr(ptr, false);
                    aux::set_state_exception_at_thread_exit(state_);
                }

                /**
                 * Useful for testing as we can check some information
                 * otherwise unavailable to us without waiting, e.g.
                 * to check whether the state is ready, its reference
                 * count etc.
                 */
                aux::shared_state<R>* __state()
                {
                    return state_;
                }

            protected:
                void abandon_state_()
                {
                    if (!state_)
                        return;

                    /**
                     * Note: This is the 'abandon' move described in
                     *       30.6.4 (7).
                     * 1) If state is not ready:
                     *   a) Store exception of type future_error with
                     *      error condition broken_promise.
                     *   b) Mark state as ready.
                     * 2) Release the state.
                     */

                    if (!state_->is_set())
                    {
                        state_->set_exception(make_exception_ptr(
                            future_error{make_error_code(future_errc::broken_promise)}
                        ));
                        state_->mark_set(true);
                    }

                    if (state_->decrement())
                    {
                        state_->destroy();
                        delete state_;
                        state_ = nullptr;
                    }
                }

                aux::shared_state<R>* state_;
        };
    }

    template<class R>
    class promise: public aux::promise_base<R>
    {
        public:
            promise()
                : aux::promise_base<R>{}
            { /* DUMMY BODY */ }

            template<class Allocator>
            promise(allocator_arg_t tag, const Allocator& a)
                : aux::promise_base<R>{tag, a}
            { /* DUMMY BODY */ }

            promise(promise&& rhs) noexcept
                : aux::promise_base<R>{move(rhs)}
            { /* DUMMY BODY */ }

            promise(const promise&) = delete;

            ~promise() override = default;

            promise& operator=(promise&& rhs) noexcept = default;

            promise& operator=(const promise&) = delete;

            future<R> get_future()
            {
                assert(this->state_);

                /**
                 * Note: Future constructor that takes a shared
                 *       state as its argument does not call increment
                 *       because a future can be created as the only
                 *       owner of that state (e.g. when created by
                 *       std::async), so we have to do it here.
                 */
                this->state_->increment();

                return future<R>{this->state_};
            }

            void set_value(const R& val)
            {
                if (!this->state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (this->state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                this->state_->set_value(val, true);
            }

            void set_value(R&& val)
            {
                if (!this->state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (this->state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                this->state_->set_value(forward<R>(val), true);
            }

            void set_value_at_thread_exit(const R& val)
            {
                if (!this->state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (this->state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                try
                {
                    this->state_->set_value(val, false);
                    aux::set_state_value_at_thread_exit(this->state_);
                }
                catch(const exception& __exception)
                {
                    this->state_->set_exception(make_exception_ptr(__exception), false);
                    aux::set_state_exception_at_thread_exit(this->state_);
                }
            }

            void set_value_at_thread_exit(R&& val)
            {
                if (!this->state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (this->state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                try
                {
                    this->state_->set_value(forward<R>(val), false);
                    aux::set_state_value_at_thread_exit(this->state_);
                }
                catch(const exception& __exception)
                {
                    this->state_->set_exception(make_exception_ptr(__exception), false);
                    aux::set_state_exception_at_thread_exit(this->state_);
                }
            }
    };

    template<class R>
    class promise<R&>: public aux::promise_base<R*>
    {
        public:
            promise()
                : aux::promise_base<R*>{}
            { /* DUMMY BODY */ }

            template<class Allocator>
            promise(allocator_arg_t tag, const Allocator& a)
                : aux::promise_base<R*>{tag, a}
            { /* DUMMY BODY */ }

            promise(promise&& rhs) noexcept
                : aux::promise_base<R*>{move(rhs)}
            { /* DUMMY BODY */ }

            promise(const promise&) = delete;

            ~promise() override = default;

            promise& operator=(promise&& rhs) noexcept = default;

            promise& operator=(const promise&) = delete;

            future<R&> get_future()
            {
                assert(this->state_);

                /**
                 * Note: Future constructor that takes a shared
                 *       state as its argument does not call increment
                 *       because a future can be created as the only
                 *       owner of that state (e.g. when created by
                 *       std::async), so we have to do it here.
                 */
                this->state_->increment();

                return future<R&>{this->state_};
            }

            void set_value(R& val)
            {
                if (!this->state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (this->state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                this->state_->set_value(&val, true);
            }

            void set_value_at_thread_exit(R& val)
            {
                if (!this->state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (this->state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                try
                {
                    this->state_->set_value(&val, false);
                    aux::set_state_value_at_thread_exit(this->state_);
                }
                catch(const exception& __exception)
                {
                    this->state_->set_exception(make_exception_ptr(__exception), false);
                    aux::set_state_exception_at_thread_exit(this->state_);
                }
            }
    };

    template<>
    class promise<void>: public aux::promise_base<void>
    {
        public:
            promise()
                : aux::promise_base<void>{}
            { /* DUMMY BODY */ }

            template<class Allocator>
            promise(allocator_arg_t tag, const Allocator& a)
                : aux::promise_base<void>{tag, a}
            { /* DUMMY BODY */ }

            promise(promise&& rhs) noexcept
                : aux::promise_base<void>{move(rhs)}
            { /* DUMMY BODY */ }

            promise(const promise&) = delete;

            ~promise() override = default;

            promise& operator=(promise&& rhs) noexcept = default;

            promise& operator=(const promise&) = delete;

            future<void> get_future()
            {
                assert(this->state_);

                /**
                 * Note: Future constructor that takes a shared
                 *       state as its argument does not call increment
                 *       because a future can be created as the only
                 *       owner of that state (e.g. when created by
                 *       std::async), so we have to do it here.
                 */
                this->state_->increment();

                return future<void>{this->state_};
            }

            void set_value()
            {
                if (!this->state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (this->state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                this->state_->set_value();
            }
    };

    template<class R>
    void swap(promise<R>& lhs, promise<R>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template<class R, class Alloc>
    struct uses_allocator<promise<R>, Alloc>: true_type
    { /* DUMMY BODY */ };
}

#endif
