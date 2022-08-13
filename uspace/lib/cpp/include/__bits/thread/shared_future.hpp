/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_THREAD_SHARED_FUTURE
#define LIBCPP_BITS_THREAD_SHARED_FUTURE

#include <__bits/thread/future.hpp>
#include <__bits/thread/future_common.hpp>
#include <type_traits>

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
    class shared_future: public aux::future_base<aux::future_inner_t<R>>
    {
        public:
            shared_future() noexcept = default;

            shared_future(const shared_future&) = default;

            shared_future(shared_future&&) noexcept = default;

            shared_future(future<R>&& rhs)
                : aux::future_base<aux::future_inner_t<R>>{move(rhs.state_)}
            {
                rhs.state_ = nullptr;
            }

            shared_future& operator=(const shared_future&) = default;

            shared_future& operator=(shared_future&&) noexcept = default;

            aux::future_return_shared_t<R> get() const
            {
                assert(this->state_);

                this->wait();

                if (this->state_->has_exception())
                    this->state_->throw_stored_exception();

                /**
                 * Using constexpr if and the future_inner and future_result
                 * metafunctions we can actually avoid having to create specializations
                 * for R& and void in this case.
                 */
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
