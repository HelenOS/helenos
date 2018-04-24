/*
 * Copyright (c) 2017 Jaroslav Jindrak
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

#ifndef LIBCPP_UTILITY
#define LIBCPP_UTILITY

#include <cstdint>
#include <internal/type_transformation.hpp>
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
     * 20.2.4, forward/move helpers:
     */

    template<class T>
    constexpr T&& forward(remove_reference_t<T>& t) noexcept
    {
        return static_cast<T&&>(t);
    }

    template<class T>
    constexpr T&& forward(remove_reference_t<T>&& t) noexcept
    {
        return static_cast<T&&>(t);
    }

    template<class T>
    constexpr remove_reference_t<T>&& move(T&& t) noexcept
    {
        return static_cast<remove_reference_t<T>&&>(t);
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

    template<class T, size_t N>
    void swap(T (&a)[N], T (&b)[N]) noexcept(noexcept(swap(*a, *b)))
    {
        // TODO: Use swap_ranges(a, a + N, b); when implemented.
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
     * 20.2.5, function template declval:
     * Note: This function only needs declaration, not
     *       implementation.
     */

    template<class T>
    add_rvalue_reference_t<T> declval() noexcept;

    /**
     * 20.3, pairs:
     */

    struct piecewise_construct_t
    {
        explicit piecewise_construct_t() = default;
    };

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

        /* TODO: need tuple, piecewise_construct_t
        template<class... Args1, class... Args2>
        pair(piecewise_construct_t, tuple<Args1...> first_args, tuple<Args2...> second_args)
        {
            // TODO:
        }
        */

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
    };

    /**
     * 20.3.3, specialized algorithms:
     */

    // TODO: implement

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

    // TODO: implement

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
}

#endif
