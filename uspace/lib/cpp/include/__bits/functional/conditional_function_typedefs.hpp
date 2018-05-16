/*
 * Copyright (c) 2018 Jaroslav Jindrak
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
