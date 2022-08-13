/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_THREAD_PACKAGED_TASK
#define LIBCPP_BITS_THREAD_PACKAGED_TASK

#include <__bits/exception.hpp>
#include <__bits/functional/function.hpp>
#include <__bits/memory/allocator_traits.hpp>
#include <__bits/thread/future.hpp>
#include <__bits/thread/future_common.hpp>
#include <__bits/thread/shared_state.hpp>
#include <type_traits>
#include <utility>

namespace std
{
    /**
     * 30.6.9, class template packaged_task:
     */

    /**
     * Note: The base template is not defined because
     *       we require the template parameter to be
     *       a callable object (e.g. a function). This
     *       is achieved by the R(Args...) specialization
     *       below.
     */
    template<class>
    class packaged_task;

    template<class R, class... Args>
    class packaged_task<R(Args...)>
    {
        public:
            packaged_task() noexcept
                : func_{}, state_{}
            { /* DUMMY BODY */ }

            template<
                class F, enable_if_t<
                    !is_same_v<
                        decay_t<F>, packaged_task<R(Args...)>
                    >, int
                > = 0
            >
            explicit packaged_task(F&& f)
                : func_{forward<F>(f)}, state_{new aux::shared_state<R>{}}
            { /* DUMMY BODY */ }

            template<
                class F, class Allocator, enable_if_t<
                    is_same_v<
                        decay_t<F>, packaged_task<R(Args...)>
                    >, int
                > = 0
            >
            explicit packaged_task(allocator_arg_t, const Allocator& a, F&& f)
                : func_{forward<F>(f)}, state_{}
            {
                typename allocator_traits<
                    Allocator
                >::template rebind_alloc<aux::shared_state<R>> rebound{a};

                state_ = rebound.allocate(1);
                rebound.construct(state_);
            }

            ~packaged_task()
            {
                if (state_)
                {
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
            }

            packaged_task(const packaged_task&) = delete;
            packaged_task& operator=(const packaged_task&) = delete;

            packaged_task(packaged_task&& rhs)
                : func_{move(rhs.func_)}, state_{move(rhs.state_)}
            { /* DUMMY BODY */ }

            packaged_task& operator=(packaged_task&& rhs)
            {
                if (state_)
                {
                    if (state_->decrement())
                    {
                        state_->destroy();
                        delete state_;
                        state_ = nullptr;
                    }
                }

                func_ = move(rhs.func_);
                state_ = move(rhs.state_);
                rhs.state_ = nullptr;

                return *this;
            }

            void swap(packaged_task& other) noexcept
            {
                std::swap(func_, other.func_);
                std::swap(state_, other.state_);
            }

            bool valid() const noexcept
            {
                return state_ != nullptr;
            }

            future<R> get_future()
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};

                state_->increment();

                return future<R>{state_};
            }

            /**
             * Note: This is how the signature is in the standard,
             *       should be investigated and verified.
             */
            void operator()(Args... args)
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                try
                {
                    state_->set_value(invoke(func_, args...));
                }
                catch(const exception& __exception)
                {
                    state_->set_exception(make_exception_ptr(__exception));
                }
            }

            void make_ready_at_thread_exit(Args... args)
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};
                if (state_->is_set())
                {
                    throw future_error{
                        make_error_code(future_errc::promise_already_satisfied)
                    };
                }

                try
                {
                    state_->set_value(invoke(func_, args...), false);
                    aux::set_state_value_at_thread_exit(this->state_);
                }
                catch(const exception& __exception)
                {
                    state_->set_exception(make_exception_ptr(__exception), false);
                    aux::set_state_exception_at_thread_exit(this->state_);
                }
            }

            void reset()
            {
                if (!state_)
                    throw future_error{make_error_code(future_errc::no_state)};

                *this = packaged_task{move(func_)};
            }

        private:
            function<R(Args...)> func_;

            aux::shared_state<R>* state_;
    };

    template<class R, class... Args>
    void swap(packaged_task<R(Args...)>& lhs, packaged_task<R(Args...)>& rhs) noexcept
    {
        lhs.swap(rhs);
    };

    template<class R, class Alloc>
    struct uses_allocator<packaged_task<R>, Alloc>: true_type
    { /* DUMMY BODY */ };
}

#endif
