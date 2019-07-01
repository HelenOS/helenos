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
                    : promise_base{}
                {
                    // TODO: Use the allocator.
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
                    promise_base{std::move(rhs)}.swap(*this);

                    return *this;
                }

                promise_base& operator=(const promise_base&) = delete;

                void swap(promise_base& other) noexcept
                {
                    std::swap(state_, other.state_);
                }

                future<R> get_future()
                {
                    return future<R>{state_};
                }

                void set_exception(exception_ptr ptr)
                {
                    assert(state_);

                    state_->set_exception(ptr);
                }

                void set_exception_at_thread_exit(exception_ptr)
                {
                    // TODO: No exception handling, no-op at this time.
                }

            protected:
                void abandon_state_()
                {
                    /**
                     * Note: This is the 'abandon' move described in
                     *       30.6.4 (7).
                     * 1) If state is not ready:
                     *   a) Store exception of type future_error with
                     *      error condition broken_promise.
                     *   b) Mark state as ready.
                     * 2) Release the state.
                     */
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
            promise(allocator_arg_t, const Allocator& a)
                : aux::promise_base<R>{}
            {
                // TODO: Use the allocator.
            }

            promise(promise&& rhs) noexcept
                : aux::promise_base<R>{move(rhs)}
            { /* DUMMY BODY */ }

            promise(const promise&) = delete;

            ~promise() override = default;

            promise& operator=(promise&& rhs) noexcept = default;

            promise& operator=(const promise&) = delete;

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

                this->state_->set_value(std::forward<R>(val), true);
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

                this->state_->set_value(val, false);
                // TODO: schedule it to be set as ready when thread exits
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

                this->state_->set_value(std::forward<R>(val), false);
                // TODO: schedule it to be set as ready when thread exits
            }
    };

    template<class R>
    class promise<R&>: public aux::promise_base<R>
    { // TODO: I'm afraid we will need aux::shared_state<R&> specialization for this :/
        public:
            promise()
                : aux::promise_base<R&>{}
            { /* DUMMY BODY */ }

            template<class Allocator>
            promise(allocator_arg_t, const Allocator& a)
                : aux::promise_base<R&>{}
            {
                // TODO: Use the allocator.
            }

            promise(promise&& rhs) noexcept
                : aux::promise_base<R&>{move(rhs)}
            { /* DUMMY BODY */ }

            promise(const promise&) = delete;

            ~promise() override = default;

            promise& operator=(promise&& rhs) noexcept = default;

            promise& operator=(const promise&) = delete;
    };

    template<>
    class promise<void>: public aux::promise_base<void>
    {
        public:
            promise()
                : aux::promise_base<void>{}
            { /* DUMMY BODY */ }

            template<class Allocator>
            promise(allocator_arg_t, const Allocator& a)
                : aux::promise_base<void>{}
            {
                // TODO: Use the allocator.
            }

            promise(promise&& rhs) noexcept
                : aux::promise_base<void>{move(rhs)}
            { /* DUMMY BODY */ }

            promise(const promise&) = delete;

            ~promise() override = default;

            promise& operator=(promise&& rhs) noexcept = default;

            promise& operator=(const promise&) = delete;

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
