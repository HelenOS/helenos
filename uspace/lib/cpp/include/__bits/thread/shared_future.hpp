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

#ifndef LIBCPP_BITS_THREAD_SHARED_FUTURE
#define LIBCPP_BITS_THREAD_SHARED_FUTURE

#include <__bits/thread/future.hpp>

/**
 * Note: We do synchronization directly on the shared
 *       state, this means that with the exception of
 *       some member functions (which are only defined
 *       for specializations anyway) our future and
 *       shared_future are basically identical. Because
 *       of that, we can use aux::future_base for
 *       shared_future as well.
 */

namespace std
{
    /**
     * 30.6.7, class template shared_future:
     */

    template<class R>
    class shared_future: public aux::future_base<R>
    {
        public:
            shared_future() noexcept = default;

            shared_future(const shared_future&) = default;

            shared_future(shared_future&&) noexcept = default;

            shared_future(future<R>&& rhs)
                : aux::future_base<R>{move(rhs.state_)}
            {
                rhs.state_ = nullptr;
            }

            shared_future& operator=(const shared_future&) = default;

            shared_future& operator=(shared_future&&) noexcept = default;

            const R& get() const
            {
                assert(this->state_);

                this->wait();

                if (this->state_->has_exception())
                    this->state_->throw_stored_exception();

                return move(this->state_->get());
            }
    };

    template<class R>
    class shared_future<R&>: public aux::future_base<R*>
    {
        public:
            shared_future() noexcept = default;

            shared_future(const shared_future&) = default;

            shared_future(shared_future&&) noexcept = default;

            shared_future(future<R&>&& rhs)
                : aux::future_base<R*>{move(rhs.state_)}
            {
                rhs.state_ = nullptr;
            }

            shared_future& operator=(const shared_future&) = default;

            shared_future& operator=(shared_future&&) noexcept = default;

            R& get() const
            {
                assert(this->state_);

                this->wait();

                if (this->state_->has_exception())
                    this->state_->throw_stored_exception();

                assert(this->state_->get());
                return *this->state_->get();
            }
    };

    template<>
    class shared_future<void>: public aux::future_base<void>
    {
        public:
            shared_future() noexcept = default;

            shared_future(const shared_future&) = default;

            shared_future(shared_future&&) noexcept = default;

            shared_future(future<void>&& rhs)
                : aux::future_base<void>{move(rhs.state_)}
            {
                rhs.state_ = nullptr;
            }

            shared_future& operator=(const shared_future&) = default;

            shared_future& operator=(shared_future&&) noexcept = default;

            void get() const
            {
                assert(this->state_);

                this->wait();

                if (this->state_->has_exception())
                    this->state_->throw_stored_exception();
            }
    };

    shared_future<void> future<void>::share()
    {
        return shared_future<void>{move(*this)};
    }
}

#endif
