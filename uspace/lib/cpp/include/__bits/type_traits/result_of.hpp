/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_TYPE_TRAITS_RESULT_OF
#define LIBCPP_BITS_TYPE_TRAITS_RESULT_OF

#include <__bits/functional/invoke.hpp>

namespace std
{
    template<class>
    struct is_function;

    template<class>
    struct is_class;

    template<class>
    struct is_member_pointer;

    template<bool, class>
    struct enable_if;

    template<class>
    struct decay;

    template<class>
    struct remove_pointer;

    template<class>
    struct result_of
    { /* DUMMY BODY */ };

    template<class F, class... ArgTypes>
    struct result_of<F(ArgTypes...)>: aux::type_is<
        typename enable_if<
            is_function<typename decay<typename remove_pointer<F>::type>::type>::value ||
            is_class<typename decay<F>::type>::value ||
            is_member_pointer<typename decay<F>::type>::value,
            decltype(aux::INVOKE(declval<F>(), declval<ArgTypes>()...))
        >::type
    >
    { /* DUMMY BODY */ };

    template<class T>
    using result_of_t = typename result_of<T>::type;
}

#endif
