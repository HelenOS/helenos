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

#ifndef LIBCPP_BITS_UTILITY
#define LIBCPP_BITS_UTILITY

#include <__bits/type_transformation.hpp>
#include <__bits/utility/forward_move.hpp>
#include <cstdint>
#include <type_traits>

namespace std
{
    /**
     * 20.2.1, operators:
     */

    namespace rel_ops
    {
        template<typename T>
        bool operator!=(const T& lhs, const T& rhs)
        {
            return !(lhs == rhs);
        }

        template<typename T>
        bool operator>(const T& lhs, const T& rhs)
        {
            return (rhs < lhs);
        }

        template<typename T>
        bool operator<=(const T& lhs, const T& rhs)
        {
            return !(rhs < lhs);
        }

        template<typename T>
        bool operator>=(const T& lhs, const T& rhs)
        {
            return !(lhs < rhs);
        }
    }

    /**
     * 20.2.2, swap:
     */

    template<class T>
    void swap(T& x, T& y)
        /* noexcept(is_nothrow_move_constructible<T>::value && */
        /*          is_nothrow_move_assignable<T>::value) */
    {
        T tmp{move(x)};
        x = move(y);
        y = move(tmp);
    }

    template<class F1, class F2>
    F2 swap_ranges(F1, F1, F2);

    template<class T, size_t N>
    void swap(T (&a)[N], T (&b)[N]) noexcept(noexcept(swap(*a, *b)))
    {
        swap_ranges(a, a + N, b);
    }

    /**
     * 20.2.3, exchange:
     */

    template<class T, class U = T>
    T exchange(T& obj, U&& new_val)
    {
        T old_val = move(obj);
        obj = forward<U>(new_val);

        return old_val;
    }

    /**
     * 20.5.2, class template integer_sequence:
     */

    template<class T, T... Is>
    struct integer_sequence
    {
        using value_type = T;

        static constexpr size_t size() noexcept
        {
            return sizeof...(Is);
        }

        using next = integer_sequence<T, Is..., sizeof...(Is)>;
    };

    template<std::size_t... Is>
    using index_sequence = integer_sequence<std::size_t, Is...>;

    /**
     * 20.5.3, alias template make_integer_sequence:
     */

    namespace aux
    {
        template<class T, uintmax_t N>
        struct make_integer_sequence
        {
            /**
             * Recursive to the bottom case below, appends sizeof...(Is) in
             * every next "call", building the sequence.
             */
            using type = typename make_integer_sequence<T, N - 1>::type::next;
        };

        template<class T>
        struct make_integer_sequence<T, std::uintmax_t(0)>
        {
            using type = integer_sequence<T>;
        };
    }


    /**
     * Problem: We can't specialize the N parameter because it is a value parameter
     *          depending on a type parameter.
     * Solution: According to the standard: if N is negative, the program is ill-formed,
     *           so we just recast it to uintmax_t :)
     */
    template<class T, T N>
    using make_integer_sequence = typename aux::make_integer_sequence<T, std::uintmax_t(N)>::type;

    template<size_t N>
    using make_index_sequence = make_integer_sequence<std::size_t, N>;

    /**
     * 20.3, pairs:
     */

    template<size_t, class>
    class tuple_element;

    template<size_t I, class T>
    using tuple_element_t = typename tuple_element<I, T>::type;

    template<class...>
    class tuple;

    template<size_t I, class... Ts>
    constexpr tuple_element_t<I, tuple<Ts...>>&& get(tuple<Ts...>&&) noexcept;

    namespace aux
    {
        template<class T, class... Args, size_t... Is>
        T from_tuple(tuple<Args...>&& tpl, index_sequence<Is...>)
        {
            return T{get<Is>(move(tpl))...};
        }

        template<class T, class... Args>
        T from_tuple(tuple<Args...>&& tpl)
        {
            return from_tuple<T>(move(tpl), make_index_sequence<sizeof...(Args)>{});
        }
    }

    struct piecewise_construct_t
    {
        explicit piecewise_construct_t() = default;
    };

    inline constexpr piecewise_construct_t piecewise_construct{};

    template<typename T1, typename T2>
    struct pair
    {
        using first_type  = T1;
        using second_type = T2;

        T1 first;
        T2 second;

        pair(const pair&) = default;
        pair(pair&&) = default;

        constexpr pair()
            : first{}, second{}
        { /* DUMMY BODY */ }

        constexpr pair(const T1& x, const T2& y)
            : first{x}, second{y}
        { /* DUMMY BODY */ }

        template<typename U, typename V>
        constexpr pair(U&& x, V&& y)
            : first(x), second(y)
        { /* DUMMY BODY */ }

        template<typename U, typename V>
        constexpr pair(const pair<U, V>& other)
            : first(other.first), second(other.second)
        { /* DUMMY BODY */ }

        template<typename U, typename V>
        constexpr pair(pair<U, V>&& other)
            : first(forward<first_type>(other.first)),
              second(forward<second_type>(other.second))
        { /* DUMMY BODY */ }

        template<class... Args1, class... Args2>
        pair(piecewise_construct_t, tuple<Args1...> first_args, tuple<Args2...> second_args)
            : first{aux::from_tuple<first_type>(move(first_args))},
              second{aux::from_tuple<second_type>(move(second_args))}
        { /* DUMMY BODY */ }

        pair& operator=(const pair& other)
        {
            first = other.first;
            second = other.second;

            return *this;
        }

        template<typename U, typename V>
        pair& operator=(const pair<U, V>& other)
        {
            first = other.first;
            second = other.second;

            return *this;
        }

        pair& operator=(pair&& other) noexcept
        {
            first = forward<first_type>(other.first);
            second = forward<second_type>(other.second);

            return *this;
        }

        template<typename U, typename V>
        pair& operator=(pair<U, V>&& other)
        {
            first = forward<first_type>(other.first);
            second = forward<second_type>(other.second);

            return *this;
        }

        void swap(pair& other) noexcept(
            noexcept(std::swap(first, other.first)) &&
            noexcept(std::swap(second, other.second))
        )
        {
            std::swap(first, other.first);
            std::swap(second, other.second);
        }
    };

    /**
     * 20.3.3, specialized algorithms:
     */

    template<class T1, class T2>
    constexpr bool operator==(const pair<T1, T2>& lhs,
                              const pair<T1, T2>& rhs)
    {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }

    template<class T1, class T2>
    constexpr bool operator<(const pair<T1, T2>& lhs,
                             const pair<T1, T2>& rhs)
    {
        return lhs.first < rhs.first ||
            (!(rhs.first < lhs.first) && lhs.second < rhs.second);
    }

    template<class T1, class T2>
    constexpr bool operator!=(const pair<T1, T2>& lhs,
                              const pair<T1, T2>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class T1, class T2>
    constexpr bool operator>(const pair<T1, T2>& lhs,
                             const pair<T1, T2>& rhs)
    {
        return rhs < lhs;
    }

    template<class T1, class T2>
    constexpr bool operator>=(const pair<T1, T2>& lhs,
                              const pair<T1, T2>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class T1, class T2>
    constexpr bool operator<=(const pair<T1, T2>& lhs,
                              const pair<T1, T2>& rhs)
    {
        return !(rhs < lhs);
    }

    template<class T1, class T2>
    constexpr void swap(pair<T1, T2>& lhs, pair<T1, T2>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class T1, class T2>
    constexpr auto make_pair(T1&& t1, T2&& t2)
    {
        return pair<
            aux::transform_tuple_types_t<T1>,
            aux::transform_tuple_types_t<T2>
        >{
            forward<T1>(t1), forward<T2>(t2)
        };
    }

    /**
     * 20.3.4, tuple-like access to pair:
     */

    template<class>
    struct tuple_size;

    template<class T1, class T2>
    struct tuple_size<pair<T1, T2>>
        : integral_constant<size_t, 2>
    { /* DUMMY BODY */ };

    template<size_t, class>
    struct tuple_element;

    template<class T1, class T2>
    struct tuple_element<0, pair<T1, T2>>
        : aux::type_is<T1>
    { /* DUMMY BODY */ };

    template<class T1, class T2>
    struct tuple_element<1, pair<T1, T2>>
        : aux::type_is<T2>
    { /* DUMMY BODY */ };

    template<size_t I, class T>
    using tuple_element_t = typename tuple_element<I, T>::type;

    template<size_t I, class T1, class T2>
    constexpr tuple_element_t<I, pair<T1, T2>>&
    get(pair<T1, T2>& p) noexcept
    {
        if constexpr (I == 0)
            return p.first;
        else
            return p.second;
    }

    template<size_t I, class T1, class T2>
    constexpr const tuple_element_t<I, pair<T1, T2>>&
    get(const pair<T1, T2>& p) noexcept
    {
        if constexpr (I == 0)
            return p.first;
        else
            return p.second;
    }

    template<size_t I, class T1, class T2>
    constexpr tuple_element_t<I, pair<T1, T2>>&&
    get(pair<T1, T2>&& p) noexcept
    {
        if constexpr (I == 0)
            return forward<T1>(p.first);
        else
            return forward<T2>(p.second);
    }

    template<class T, class U>
    constexpr T& get(pair<T, U>& p) noexcept
    {
        static_assert(!is_same_v<T, U>, "get(pair) requires distinct types");

        return get<0>(p);
    }

    template<class T, class U>
    constexpr const T& get(const pair<T, U>& p) noexcept
    {
        static_assert(!is_same_v<T, U>, "get(pair) requires distinct types");

        return get<0>(p);
    }

    template<class T, class U>
    constexpr T&& get(pair<T, U>&& p) noexcept
    {
        static_assert(!is_same_v<T, U>, "get(pair) requires distinct types");

        return get<0>(move(p));
    }

    template<class T, class U>
    constexpr T& get(pair<U, T>& p) noexcept
    {
        static_assert(!is_same_v<T, U>, "get(pair) requires distinct types");

        return get<1>(p);
    }

    template<class T, class U>
    constexpr const T& get(const pair<U, T>& p) noexcept
    {
        static_assert(!is_same_v<T, U>, "get(pair) requires distinct types");

        return get<1>(p);
    }

    template<class T, class U>
    constexpr T&& get(pair<U, T>&& p) noexcept
    {
        static_assert(!is_same_v<T, U>, "get(pair) requires distinct types");

        return get<1>(move(p));
    }
}

#endif
