/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_FUNCTION_CONDITIONAL_FUNCTION_TYPEDEFS
#define LIBCPP_BITS_FUNCTION_CONDITIONAL_FUNCTION_TYPEDEFS

namespace std::aux
{
    /**
     * Note: Some types with possible function-like behavior
     *       (e.g. function, reference_wrapper, aux::mem_fn_t)
     *       have to define a type argument type or two types
     *       first_argument_type and second_argument_type when
     *       their invocation takes one or two arguments, respectively.
     *
     *       The easiest way to conditionally define a type is using
     *       inheritance and the types below serve exactly for that
     *       purpose.
     *
     *       If we have a type that is specialized as R(Args...)
     *       for some return type R and argument types Args...,
     *       we can simply (publicly) inherit from
     *       conditional_function_typedefs<Args...>. If it is
     *       templated by some generic type T, we can inherit from
     *       conditional_function_typedefs<remove_cv_t<remove_reference_t<T>>>
     *       so that the correct type below gets chosen.
     */

    template<class... Args>
    struct conditional_function_typedefs
    { /* DUMMY BODY */ };

    template<class A>
    struct conditional_function_typedefs<A>
    {
        using argument_type = A;
    };

    template<class A1, class A2>
    struct conditional_function_typedefs<A1, A2>
    {
        using first_argument_type  = A1;
        using second_argument_type = A2;
    };

    template<class R, class... Args>
    struct conditional_function_typedefs<R(Args...)>
        : conditional_function_typedefs<Args...>
    { /* DUMMY BODY */ };
}

#endif
