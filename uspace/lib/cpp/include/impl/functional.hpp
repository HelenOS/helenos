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

#ifndef LIBCPP_FUNCTIONAL
#define LIBCPP_FUNCTIONAL

#include <internal/functional/conditional_function_typedefs.hpp>
#include <internal/functional/invoke.hpp>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace std
{
    /**
     * 20.9.3, invoke:
     */

    template<class F, class... Args>
    decltype(auto) invoke(F&& f, Args&&... args)
    {
        return aux::invoke(forward<F>(f)(forward<Args>(args)...));
    }

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

    /**
     * 20.9.11, member function adaptors:
     */

    namespace aux
    {
        template<class F>
        class mem_fn_t
            : public conditional_function_typedefs<remove_cv_t<remove_reference_t<F>>>
        {
            public:
                mem_fn_t(F f)
                    : func_{f}
                { /* DUMMY BODY */ }

                template<class... Args>
                decltype(auto) operator()(Args&&... args)
                {
                    return invoke(func_, forward<Args>(args)...);
                }

            private:
                F func_;
        };
    }

    template<class R, class T>
    aux::mem_fn_t<R T::*> mem_fn(R T::* f)
    {
        return aux::mem_fn_t<R T::*>{f};
    }

    /**
     * 20.9.13, hash function primary template:
     */

    namespace aux
    {
        template<class T>
        union converter
        {
            T value;
            uint64_t converted;
        };

        template<class T>
        T hash_(uint64_t x) noexcept
        {
            /**
             * Note: std::hash is used for indexing in
             *       unordered containers, not for cryptography.
             *       Because of this, we decided to simply convert
             *       the value to uin64_t, which will help us
             *       with testing (since in order to create
             *       a collision in a multiset or multimap
             *       we simply need 2 values that congruent
             *       by the size of the table.
             */
            return static_cast<T>(x);
        }

        template<class T>
        size_t hash(T x) noexcept
        {
            static_assert(is_arithmetic_v<T> || is_pointer_v<T>,
                          "invalid type passed to aux::hash");

            converter<T> conv;
            conv.value = x;

            return hash_<size_t>(conv.converted);
        }
    }

    template<class T>
    struct hash
    { /* DUMMY BODY */ };

    template<>
    struct hash<bool>
    {
        size_t operator()(bool x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = bool;
        using result_type   = size_t;
    };

    template<>
    struct hash<char>
    {
        size_t operator()(char x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = char;
        using result_type   = size_t;
    };

    template<>
    struct hash<signed char>
    {
        size_t operator()(signed char x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = signed char;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned char>
    {
        size_t operator()(unsigned char x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned char;
        using result_type   = size_t;
    };

    template<>
    struct hash<char16_t>
    {
        size_t operator()(char16_t x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = char16_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<char32_t>
    {
        size_t operator()(char32_t x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = char32_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<wchar_t>
    {
        size_t operator()(wchar_t x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = wchar_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<short>
    {
        size_t operator()(short x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = short;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned short>
    {
        size_t operator()(unsigned short x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned short;
        using result_type   = size_t;
    };

    template<>
    struct hash<int>
    {
        size_t operator()(int x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = int;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned int>
    {
        size_t operator()(unsigned int x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned int;
        using result_type   = size_t;
    };

    template<>
    struct hash<long>
    {
        size_t operator()(long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = long;
        using result_type   = size_t;
    };

    template<>
    struct hash<long long>
    {
        size_t operator()(long long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = long long;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned long>
    {
        size_t operator()(unsigned long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned long;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned long long>
    {
        size_t operator()(unsigned long long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned long long;
        using result_type   = size_t;
    };

    template<>
    struct hash<float>
    {
        size_t operator()(float x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = float;
        using result_type   = size_t;
    };

    template<>
    struct hash<double>
    {
        size_t operator()(double x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = double;
        using result_type   = size_t;
    };

    template<>
    struct hash<long double>
    {
        size_t operator()(long double x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = long double;
        using result_type   = size_t;
    };

    template<class T>
    struct hash<T*>
    {
        size_t operator()(T* x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = T*;
        using result_type   = size_t;
    };
}

#endif
