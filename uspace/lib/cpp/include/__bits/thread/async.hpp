/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_THREAD_ASYNC
#define LIBCPP_BITS_THREAD_ASYNC

#include <__bits/thread/future.hpp>
#include <__bits/thread/future_common.hpp>
#include <__bits/thread/shared_state.hpp>
#include <__bits/type_traits/result_of.hpp>
#include <__bits/utility/forward_move.hpp>
#include <cassert>

namespace std
{

    enum class launch
    {
        async = 1,
        deferred
    };

    namespace aux
    {
        /**
         * Note: The reason we keep the actual function
         *       within the aux namespace is that were the non-policy
         *       version of the function call the other one in the std
         *       namespace, we'd get resolution conflicts. This way
         *       aux::async is properly called even if std::async is
         *       called either with or without a launch policy.
         */
        template<class F, class... Args>
        future<result_of_t<decay_t<F>(decay_t<Args>...)>>
        async(launch policy, F&& f, Args&&... args)
        {
            using result_t = result_of_t<decay_t<F>(decay_t<Args>...)>;

            bool async = (static_cast<int>(policy) &
                          static_cast<int>(launch::async)) != 0;
            bool deferred = (static_cast<int>(policy) &
                             static_cast<int>(launch::deferred)) != 0;

            /**
             * Note: The case when async | deferred is set in policy
             *       is implementation defined, feel free to change.
             * Rationale: We chose the 'deferred' policy, because unlike
             *            the 'async' policy it carries no possible
             *            surprise ('async' can fail due to thread
             *            creation error).
             */
            if (async && deferred)
            {
                return future<result_t>{
                    new aux::deferred_shared_state<
                        result_t, F, Args...
                    >{forward<F>(f), forward<Args>(args)...}
                };
            }
            else if (async)
            {
                return future<result_t>{
                    new aux::async_shared_state<
                        result_t, F, Args...
                    >{forward<F>(f), forward<Args>(args)...}
                };
            }
            else if (deferred)
            {
               /**
                * Duplicated on purpose because of the default.
                * Do not remove!
                */
                return future<result_t>{
                    new aux::deferred_shared_state<
                        result_t, F, Args...
                    >{forward<F>(f), forward<Args>(args)...}
                };
            }

            /**
             * This is undefined behaviour, abandon ship!
             */
            abort();
        }
    }

    template<class F>
    decltype(auto) async(F&& f)
    {
        launch policy = static_cast<launch>(
            static_cast<int>(launch::async) |
            static_cast<int>(launch::deferred)
        );

        return aux::async(policy, forward<F>(f));
    }

    /**
     * The async(launch, F, Args...) and async(F, Args...)
     * overloards must not collide, so we check the first template
     * argument and handle the special case of just a functor
     * above.
     */
    template<class F, class Arg, class... Args>
    decltype(auto) async(F&& f, Arg&& arg, Args&&... args)
    {
        if constexpr (is_same_v<decay_t<F>, launch>)
            return aux::async(f, forward<Arg>(arg), forward<Args>(args)...);
        else
        {
            launch policy = static_cast<launch>(
                static_cast<int>(launch::async) |
                static_cast<int>(launch::deferred)
            );

            return aux::async(policy, forward<F>(f), forward<Arg>(arg), forward<Args>(args)...);
        }
    }
}

#endif
