/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
