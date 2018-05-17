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
