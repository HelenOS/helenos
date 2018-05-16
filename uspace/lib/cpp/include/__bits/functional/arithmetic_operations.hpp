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

#ifndef LIBCPP_BITS_FUNCTIONAL_ARITHMETIC_OPERATIONS
#define LIBCPP_BITS_FUNCTIONAL_ARITHMETIC_OPERATIONS

#include <__bits/utility/forward_move.hpp>
#include <type_traits>

namespace std
{
    /**
     * 20.9.5, arithmetic operations:
     */

    template<class T = void>
    struct plus
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs + rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct minus
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs - rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct multiplies
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs * rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct divides
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs / rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct modulus
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs % rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct negate
    {
        constexpr T operator()(const T& x) const
        {
            return -x;
        }

        using argument_type = T;
        using result_type   = T;
    };

    namespace aux
    {
        /**
         * Used by some functions like std::set::find to determine
         * whether a functor is transparent.
         */
        struct transparent_t;

        template<class T, class = void>
        struct is_transparent: false_type
        { /* DUMMY BODY */ };

        template<class T>
        struct is_transparent<T, void_t<typename T::is_transparent>>
            : true_type
        { /* DUMMY BODY */ };

        template<class T>
        inline constexpr bool is_transparent_v = is_transparent<T>::value;
    }

    template<>
    struct plus<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) + forward<U>(rhs))
        {
            return forward<T>(lhs) + forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct minus<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) - forward<U>(rhs))
        {
            return forward<T>(lhs) - forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct multiplies<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) * forward<U>(rhs))
        {
            return forward<T>(lhs) * forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct divides<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) / forward<U>(rhs))
        {
            return forward<T>(lhs) / forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct modulus<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) % forward<U>(rhs))
        {
            return forward<T>(lhs) % forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct negate<void>
    {
        template<class T>
        constexpr auto operator()(T&& x) const
            -> decltype(-forward<T>(x))
        {
            return -forward<T>(x);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.6, comparisons:
     */

    template<class T = void>
    struct equal_to
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs == rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct not_equal_to
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs != rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct greater
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs > rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct less
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs < rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct greater_equal
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs >= rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct less_equal
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs <= rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<>
    struct equal_to<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) == forward<U>(rhs))
        {
            return forward<T>(lhs) == forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct not_equal_to<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) != forward<U>(rhs))
        {
            return forward<T>(lhs) != forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct greater<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) > forward<U>(rhs))
        {
            return forward<T>(lhs) > forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct less<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) < forward<U>(rhs))
        {
            return forward<T>(lhs) < forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct greater_equal<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) >= forward<U>(rhs))
        {
            return forward<T>(lhs) >= forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct less_equal<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) <= forward<U>(rhs))
        {
            return forward<T>(lhs) <= forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.7, logical operations:
     */

    template<class T = void>
    struct logical_and
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs && rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct logical_or
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs || rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct logical_not
    {
        constexpr bool operator()(const T& x) const
        {
            return !x;
        }

        using argument_type = T;
        using result_type   = bool;
    };

    template<>
    struct logical_and<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) && forward<U>(rhs))
        {
            return forward<T>(lhs) && forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct logical_or<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) || forward<U>(rhs))
        {
            return forward<T>(lhs) || forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct logical_not<void>
    {
        template<class T>
        constexpr auto operator()(T&& x) const
            -> decltype(!forward<T>(x))
        {
            return !forward<T>(x);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.8, bitwise operations:
     */

    template<class T = void>
    struct bit_and
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs & rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct bit_or
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs | rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct bit_xor
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs ^ rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct bit_not
    {
        constexpr bool operator()(const T& x) const
        {
            return ~x;
        }

        using argument_type = T;
        using result_type   = T;
    };

    template<>
    struct bit_and<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) & forward<U>(rhs))
        {
            return forward<T>(lhs) & forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct bit_or<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) | forward<U>(rhs))
        {
            return forward<T>(lhs) | forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct bit_xor<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) ^ forward<U>(rhs))
        {
            return forward<T>(lhs) ^ forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct bit_not<void>
    {
        template<class T>
        constexpr auto operator()(T&& x) const
            -> decltype(~forward<T>(x))
        {
            return ~forward<T>(x);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.9, negators:
     */

    template<class Predicate>
    class unary_negate
    {
        public:
            using result_type   = bool;
            using argument_type = typename Predicate::argument_type;

            constexpr explicit unary_negate(const Predicate& pred)
                : pred_{pred}
            { /* DUMMY BODY */ }

            constexpr result_type operator()(const argument_type& arg)
            {
                return !pred_(arg);
            }

        private:
            Predicate pred_;
    };

    template<class Predicate>
    constexpr unary_negate<Predicate> not1(const Predicate& pred)
    {
        return unary_negate<Predicate>{pred};
    }

    template<class Predicate>
    class binary_negate
    {
        public:
            using result_type          = bool;
            using first_argument_type  = typename Predicate::first_argument_type;
            using second_argument_type = typename Predicate::second_argument_type;

            constexpr explicit binary_negate(const Predicate& pred)
                : pred_{pred}
            { /* DUMMY BODY */ }

            constexpr result_type operator()(const first_argument_type& arg1,
                                             const second_argument_type& arg2)
            {
                return !pred_(arg1, arg2);
            }

        private:
            Predicate pred_;
    };

    template<class Predicate>
    constexpr binary_negate<Predicate> not2(const Predicate& pred);
}

#endif
