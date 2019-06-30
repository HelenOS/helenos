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

    template<class R>
    class promise
    {
        public:
            promise()
                : state_{new aux::shared_state<R>{}}
            { /* DUMMY BODY */ }

            template<class Allocator>
            promise(allocator_arg_t, const Allocator& a)
                : promise{}
            {
                // TODO: Use the allocator.
            }

            promise(promise&& rhs) noexcept
                : state_{}
            {
                state_ = rhs.state_;
                rhs.state_ = nullptr;
            }

            promise(const promise&) = delete;

            ~promise()
            {
                abandon_state_();
            }

            promise& operator=(promise&& rhs) noexcept
            {
                abandon_state_();
                promise{std::move(rhs)}.swap(*this);
            }

            promise& operator=(const promise&) = delete;

            void swap(promise& other) noexcept
            {
                std::swap(state_, other.state_);
            }

            future<R> get_future()
            {
                return future<R>{state_};
            }

            void set_value(const R& val)
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                state_->set_value(val, true);
            }

            void set_value(R&& val)
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                state_->set_value(std::forward<R>(val), true);
            }

            void set_exception(exception_ptr ptr)
            {
                assert(state_);

                state_->set_exception(ptr);
            }

            void set_value_at_thread_exit(const R& val)
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                state_->set_value(val, false);
                // TODO: schedule it to be set as ready when thread exits
            }

            void set_value_at_thread_exit(R&& val)
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                state_->set_value(std::forward<R>(val), false);
                // TODO: schedule it to be set as ready when thread exits
            }

            void set_exception_at_thread_exit(exception_ptr)
            {
                // TODO: No exception handling, no-op at this time.
            }

        private:
            void abandon_state_()
            {
                /**
                 * Note: This is the 'abandon' move described in
                 *       30.6.4 (7).
                 * 1) If state is not ready:
                 *   a) Store exception of type future_error with
                 *      error condition broken_promise.
                 *   b) Mark state as ready.
                 * 2) Rekease the state.
                 */
            }

            aux::shared_state<R>* state_;
    };

    template<class R>
    class promise<R&>
    {
        // TODO: Copy & modify once promise is done.
    };

    template<>
    class promise<void>
    {
        // TODO: Copy & modify once promise is done.
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
