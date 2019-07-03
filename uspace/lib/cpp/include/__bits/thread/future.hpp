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

#ifndef LIBCPP_BITS_THREAD_FUTURE
#define LIBCPP_BITS_THREAD_FUTURE

#include <__bits/thread/future_common.hpp>
#include <__bits/thread/shared_state.hpp>
#include <__bits/utility/forward_move.hpp>
#include <cassert>

namespace std
{
    /**
     * 30.6.6, class template future:
     */

    namespace aux
    {
        /**
         * Note: Because of shared_future, this base class
         *       does implement copy constructor and copy
         *       assignment operator. This means that the
         *       children (std::future) need to delete this
         *       constructor and operator themselves.
         */
        template<class R>
        class future_base
        {
            public:
                future_base() noexcept
                    : state_{nullptr}
                { /* DUMMY BODY */ }

                future_base(const future_base& rhs)
                    : state_{rhs.state_}
                {
                    state_->increment();
                }

                future_base(future_base&& rhs) noexcept
                    : state_{move(rhs.state_)}
                {
                    rhs.state_ = nullptr;
                }

                future_base(aux::shared_state<R>* state)
                    : state_{state}
                {
                    /**
                     * Note: This is a custom non-standard constructor that allows
                     *       us to create a future directly from a shared state. This
                     *       should never be a problem as aux::shared_state is a private
                     *       type and future has no constructor templates.
                     */
                }

                virtual ~future_base()
                {
                    release_state_();
                }

                future_base& operator=(const future_base& rhs)
                {
                    release_state_();
                    state_ = rhs.state_;

                    state_->increment();

                    return *this;
                }

                future_base& operator=(future_base&& rhs) noexcept
                {
                    release_state_();
                    state_ = move(rhs.state_);
                    rhs.state_ = nullptr;

                    return *this;
                }

                bool valid() const noexcept
                {
                    return state_ != nullptr;
                }

                void wait() const noexcept
                {
                    assert(state_);

                    state_->wait();
                }

                template<class Rep, class Period>
                future_status
                wait_for(const chrono::duration<Rep, Period>& rel_time) const
                {
                    assert(state_);

                    return state_->wait_for(rel_time);
                }

                template<class Clock, class Duration>
                future_status
                wait_until(
                    const chrono::time_point<Clock, Duration>& abs_time
                ) const
                {
                    assert(state_);

                    return state_->wait_until(abs_time);
                }

            protected:
                void release_state_()
                {
                    if (!state_)
                        return;

                    /**
                     * Note: This is the 'release' move described in
                     *       30.6.4 (5).
                     * Last reference to state -> destroy state.
                     * Decrement refcount of state otherwise.
                     * Will not block, unless all following hold:
                     *  1) State was created by call to std::async.
                     *  2) State is not yet ready.
                     *  3) This was the last reference to the shared state.
                     */
                    if (state_->decrement())
                    {
                        /**
                         * The destroy call handles the special case
                         * when 1) - 3) hold.
                         */
                        state_->destroy();
                        delete state_;
                        state_ = nullptr;
                    }
                }

                aux::shared_state<R>* state_;
        };
    }

    template<class R>
    class shared_future;

    template<class R>
    class future: public aux::future_base<aux::future_inner_t<R>>
    {
        friend class shared_future<R>;

        public:
            future() noexcept
                : aux::future_base<aux::future_inner_t<R>>{}
            { /* DUMMY BODY */ }

            future(const future&) = delete;

            future(future&& rhs) noexcept
                : aux::future_base<aux::future_inner_t<R>>{move(rhs)}
            { /* DUMMY BODY */ }

            future(aux::shared_state<aux::future_inner_t<R>>* state)
                : aux::future_base<aux::future_inner_t<R>>{state}
            { /* DUMMY BODY */ }

            future& operator=(const future&) = delete;

            future& operator=(future&& rhs) noexcept = default;

            shared_future<R> share()
            {
                return shared_future<R>{move(*this)};
            }

            R get()
            {
                assert(this->state_);

                this->wait();

                if (this->state_->has_exception())
                    this->state_->throw_stored_exception();

                if constexpr (!is_same_v<R, void>)
                {
                    if constexpr (is_reference_v<R>)
                    {
                        assert(this->state_->get());

                        return *this->state_->get();
                    }
                    else
                        return this->state_->get();
                }
            }

            /**
             * Useful for testing as we can check some information
             * otherwise unavailable to us without waiting, e.g.
             * to check whether the state is ready, its reference
             * count etc.
             */
            aux::shared_state<aux::future_inner_t<R>>* __state() noexcept
            {
                return this->state_;
            }
    };
}

#endif
