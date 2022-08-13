/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_THREAD_FUTURE_COMMON
#define LIBCPP_BITS_THREAD_FUTURE_COMMON

#include <__bits/aux.hpp>
#include <system_error>
#include <stdexcept>

namespace std
{
    /**
     * 30.6, futures:
     */

    enum class future_errc
    { // The 5001 start is to not collide with system_error's codes.
        broken_promise = 5001,
        future_already_retrieved,
        promise_already_satisfied,
        no_state
    };

    enum class future_status
    {
        ready,
        timeout,
        deferred
    };

    /**
     * 30.6.2, error handling:
     */

    template<>
    struct is_error_code_enum<future_errc>: true_type
    { /* DUMMY BODY */ };

    error_code make_error_code(future_errc) noexcept;
    error_condition make_error_condition(future_errc) noexcept;

    const error_category& future_category() noexcept;

    /**
     * 30.6.3, class future_error:
     */

    class future_error: public logic_error
    {
        public:
            future_error(error_code ec);

            const error_code& code() const noexcept;
            const char* what() const noexcept;

        private:
            error_code code_;
    };

    namespace aux
    {
        /**
         * Auxilliary metafunctions that let us avoid
         * specializations in some cases. They represent
         * the inner stored type and the return type of
         * the get() member function, respectively.
         */

        template<class T>
        struct future_inner: aux::type_is<T>
        { /* DUMMY BODY */ };

        template<class T>
        struct future_inner<T&>: aux::type_is<T*>
        { /* DUMMY BODY */ };

        template<class T>
        using future_inner_t = typename future_inner<T>::type;

        template<class T>
        struct future_return_shared: aux::type_is<const T&>
        { /* DUMMY BODY */ };

        template<class T>
        struct future_return_shared<T&>: aux::type_is<T&>
        { /* DUMMY BODY */ };

        template<>
        struct future_return_shared<void>: aux::type_is<void>
        { /* DUMMY BODY */ };

        template<class T>
        using future_return_shared_t = typename future_return_shared<T>::type;
    }
}

#endif
