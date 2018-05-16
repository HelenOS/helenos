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

#ifndef LIBCPP_BITS_TUPLE_TUPLE_CAT
#define LIBCPP_BITS_TUPLE_TUPLE_CAT

#include <utility>

namespace std
{ // forward declarations
    template<class... Ts>
    struct tuple;

    template<size_t I, class T>
    struct tuple_element;

    template<size_t I, class... Ts>
    constexpr typename tuple_element<I, tuple<Ts...>>::type& get(tuple<Ts...>& tpl) noexcept;

    template<class T, T...>
    struct integer_sequence;

    template<size_t... Is>
    using index_sequence = integer_sequence<size_t, Is...>;
}

namespace std::aux
{
    template<class...>
    struct tuple_append;

    template<class... Ts, class... Us, class... Vs>
    struct tuple_append<tuple<Ts...>, tuple<Us...>, Vs...>
        : type_is<tuple<Ts..., Us...>>
    { /* DUMMY BODY */ };

    template<class... Ts>
    using tuple_append_t = typename tuple_append<Ts...>::type;

    template<class...>
    struct remove_first_tuple;

    template<class... Ts, class... Tuples>
    struct remove_first_tuple<tuple<Ts...>, Tuples...>
        : type_is<Tuples...>
    { /* DUMMY BODY */ };

    template<class... Ts>
    using remove_first_tuple_t = typename remove_first_tuple<Ts...>::type;

    template<class...>
    struct get_first_tuple;

    template<class... Ts, class... Tuples>
    struct get_first_tuple<tuple<Ts...>, Tuples...>
        : type_is<tuple<Ts...>>
    { /* DUMMY BODY */ };

    template<class... Ts>
    using get_first_tuple_t = typename get_first_tuple<Ts...>::type;

    template<class...>
    struct tuple_cat_type_impl;

    template<class... Ts, class... Tuples>
    struct tuple_cat_type_impl<tuple<Ts...>, Tuples...>
        : type_is<
        typename tuple_cat_type_impl<
            tuple_append_t<tuple<Ts...>, get_first_tuple_t<Tuples...>>,
            remove_first_tuple_t<Tuples...>
        >::type
    >
    { /* DUMMY BODY */ };

    template<class... Ts>
    using tuple_cat_type_impl_t = typename tuple_cat_type_impl<Ts...>::type;

    template<class...>
    struct tuple_cat_type;

    template<class... Ts, class... Tuples>
    struct tuple_cat_type<tuple<Ts...>, Tuples...>
        : type_is<tuple_cat_type_impl_t<tuple<Ts...>, Tuples...>>
    { /* DUMMY BODY */ };

    template<class... Ts>
    using tuple_cat_type_t = typename tuple_cat_type<Ts...>::type;

    template<class...>
    struct concatenate_sequences;

    template<size_t... Is1, size_t... Is2>
    struct concatenate_sequences<index_sequence<Is1...>, index_sequence<Is2...>>
        : type_is<index_sequence<Is1..., Is2...>>
    { /* DUMMY BODY */ };

    template<class... Ts>
    using concatenate_sequences_t = typename concatenate_sequences<Ts...>::type;

    template<class...>
    struct append_indices;

    template<size_t... Is, class... Ts, class... Tuples>
    struct append_indices<index_sequence<Is...>, tuple<Ts...>, Tuples...>
        : append_indices<
        concatenate_sequences_t<
            index_sequence<Is...>, make_index_sequence<sizeof...(Ts)>
        >,
        Tuples...
    >
    { /* DUMMY BODY */ };

    template<class... Ts>
    using append_indices_t = typename append_indices<Ts...>::types;

    template<class...>
    struct generate_indices;

    template<class... Ts, class... Tuples>
    struct generate_indices<tuple<Ts...>, Tuples...>
        : type_is<
        append_indices_t<
            make_index_sequence<sizeof...(Ts)>,
            Tuples...
        >
    >
    { /* DUMMY BODY */ };

    template<class... Ts>
    using generate_indices_t = typename generate_indices<Ts...>::type;

    template<class... Tuples, size_t... Is1, size_t... Is2>
    tuple_cat_type_t<Tuples...> tuple_cat(Tuples&&... tpls,
                                          index_sequence<Is1...>,
                                          index_sequence<Is2...>)
    {
        return tuple_cat_type_t<Tuples...>{
            get<Is1...>(
                get<Is2...>(
                    forward<Tuples>(tpls)
                )
            )...
        };
    }
}

#endif
