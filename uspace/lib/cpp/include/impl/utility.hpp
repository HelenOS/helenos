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

namespace std
{
    /**
     * 20.2.1, operators:
     */
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

    // TODO: swap
    // TODO: exchange

    /**
     * 20.2.4, forward/move helpers:
     */

    template<class T>
    inline constexpr T&& forward(remove_reference_t<T>& t) noexcept
    {
        return static_cast<T&&>(t);
    }

    template<class T>
    inline constexpr T&& forward(remove_reference_t<T>&& t) noexcept
    {
        // TODO: check if t is lvalue reference, if it is, the program
        //       is ill-formed according to the standard
        return static_cast<T&&>(t);
    }

    template<class T>
    inline constexpr remove_reference_t<T>&& move(T&&) noexcept
    {
        return static_cast<remove_reference_t<T>&&>(t);
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
}

#endif
