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

#ifndef LIBCPP_BITS_TUPLE_TUPLE_OPS
#define LIBCPP_BITS_TUPLE_TUPLE_OPS

#include <utility>

namespace std
{ // forward declarations
    template<class... Ts>
    struct tuple;

    template<size_t I, class T>
    struct tuple_element;

    template<size_t I, class... Ts>
    constexpr typename tuple_element<I, tuple<Ts...>>::type& get(tuple<Ts...>& tpl) noexcept;
}

namespace std::aux
{
    template<size_t I, size_t N>
    struct tuple_ops
    {
        template<class T, class U>
        static void assign_copy(T& lhs, const U& rhs)
        {
            get<I>(lhs) = get<I>(rhs);

            tuple_ops<I + 1, N>::assign_copy(lhs, rhs);
        }

        template<class T, class U>
        static void assign_move(T& lhs, U&& rhs)
        {
            get<I>(lhs) = move(get<I>(rhs));

            tuple_ops<I + 1, N>::assign_move(lhs, move(rhs));
        }

        template<class T, class U>
        static void swap(T& lhs, U& rhs)
        {
            std::swap(get<I>(lhs), get<I>(rhs));

            tuple_ops<I + 1, N>::swap(lhs, rhs);
        }

        template<class T, class U>
        static bool eq(const T& lhs, const U& rhs)
        {
            return (get<I>(lhs) == get<I>(rhs)) && tuple_ops<I + 1, N>::eq(lhs, rhs);
        }

        template<class T, class U>
        static bool lt(const T& lhs, const U& rhs)
        {
            return (get<I>(lhs) < get<I>(rhs)) ||
                (!(get<I>(rhs) < get<I>(lhs)) && tuple_ops<I + 1, N>::lt(lhs, rhs));
        }
    };

    template<size_t N>
    struct tuple_ops<N, N>
    {
        template<class T, class U>
        static void assign_copy(T& lhs, const U& rhs)
        {
            get<N>(lhs) = get<N>(rhs);
        }

        template<class T, class U>
        static void assign_move(T& lhs, U&& rhs)
        {
            get<N>(lhs) = move(get<N>(rhs));
        }

        template<class T, class U>
        static void swap(T& lhs, U& rhs)
        {
            std::swap(get<N>(lhs), get<N>(rhs));
        }

        template<class T, class U>
        static bool eq(const T& lhs, const U& rhs)
        {
            return get<N>(lhs) == get<N>(rhs);
        }

        template<class T, class U>
        static bool lt(const T& lhs, const U& rhs)
        {
            return get<N>(lhs) < get<N>(rhs);
        }
    };
}

#endif
