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

#include <type_traits>

extern "C" {
#include <adt/hash.h>
}

namespace std
{
    namespace aux
    {
        /**
         * 20.9.2, requirements:
         */
        template<class R, class T, class T1, class... Ts>
        decltype(auto) invoke(R T::* f, T1&& t1, Args&&... args)
        {
            if constexpr (is_member_function_pointer_v<decltype(f)>)
            {
                if constexpr (is_base_of_v<T, remove_reference_t<T1>>)
                    // (1.1)
                    return (t1.*f)(forward<Args>(args)...);
                else
                    // (1.2)
                    return ((*t1).*f)(forward<Args>(args)...);
            }
            else if (is_member_object_pointer_v<decltype(f)> && sizeof...(args) == 0)
            {
                /**
                 * Note: Standard requires to N be equal to 1, but we take t1 directly
                 *       so we need sizeof...(args) to be 0.
                 */
                if constexpr (is_base_of_v<T, remove_reference_t<T1>>)
                    // (1.3)
                    return t1.*f;
                else
                    // (1.4)
                    return (*t1).*f;
            }
            static_assert(false, "invalid invoke");
        }

        template<class F, class... Args>
        decltype(auto) invoke(F&& f, Args&&... args)
        {
            // (1.5)
            return f(forward<Args>(args)...);
        }
    }

    /**
     * 20.9.3, invoke:
     */

    template<class F, class... Args>
    result_of_t<F&&(Args&&...)> invoke(F&& f, Args&&... args)
    {
        return aux::invoke(forward<F>(f)(forward<Args>(args)...));
    }

    /**
     * 20.9.4, reference_wrapper:
     */

    template<class T>
    class reference_wrapper;

    template<class T>
    reference_wrapper<T> ref(T& t) noexcept;

    template<class T>
    reference_wrapper<const T> cref(const T& t) noexcept;

    template<class T>
    void ref(const T&&) = delete;

    template<class T>
    void cref(const T&&) = delete;

    template<class T>
    reference_wrapper<T> ref(reference_wrapper<T> ref) noexcept;

    template<class T>
    reference_wrapper<const T> cref(reference_wrapper<T> ref) noexcept;

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
    }

    template<>
    struct plus<void>
    {
        template<class T, class U>
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& x) const
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
    struct equal_to;
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& x) const
            -> decltype(!forward<T>(x))
        {
            return !forward<T>(lhs);
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& lhs, U&& rhs) const
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
        constexpr auto operator(T&& x) const
            -> decltype(~forward<T>(x))
        {
            return ~forward<T>(lhs);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.9, negators:
     */

    template<class Predicate>
    class unary_negate;

    template<class Predicate>
    constexpr unary_negate<Predicate> not1(const Predicate& pred);

    template<class Predicate>
    class binary_negate;

    template<class Predicate>
    constexpr binary_negate<Predicate> not2(const Predicate& pred);

    /**
     * 20.9.10, bind:
     */

    template<class T>
    struct is_bind_expression;

    template<class T>
    struct is_placeholder;

    // TODO: void should be /unspecified/
    template<class F, class... Args>
    void bind(F&& f, Args&&... args);

    template<class R, class F, class... Args>
    void bind(F&& f, Args&&... args);

    namespace placeholders
    {
        /**
         * TODO: for X from 1 to implementation defined M
         * extern /unspecified/ _X;
         */
    }

    /**
     * 20.9.11, member function adaptors:
     */

    // TODO: void should be /unspecified/
    template<class R, class T>
    void mem_fn(R T::* f);

    /**
     * 20.9.12, polymorphic function adaptors:
     */

    class bad_function_call;

    template<class>
    class function; // undefined

    template<class R, class... Args>
    class function<R(Args...)>;

    template<class R, class... Args>
    void swap(function<R(Args...)>& f1, function<R(Args...)>& f2);

    template<class R, class... Args>
    bool operator==(const function<R(Args...)>&, nullptr_t) noexcept;

    template<class R, class... Args>
    bool operator==(nullptr_t, const function<R(Args...)>&) noexcept;

    template<class R, class... Args>
    bool operator!=(const function<R(Args...)>&, nullptr_t) noexcept;

    template<class R, class... Args>
    bool operator!=(nullptr_t, const function<R(Args...)>&) noexcept;

    /**
     * 20.9.13, hash function primary template:
     */

    template<class T>
    struct hash
    { /* DUMMY BODY */ };

    template<>
    struct hash<bool>
    {
        size_t operator()(bool x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = bool;
        using result_type   = size_t;
    };

    template<>
    struct hash<char>
    {
        size_t operator()(char x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = char;
        using result_type   = size_t;
    };

    template<>
    struct hash<signed char>
    {
        size_t operator()(signed char x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = signed char;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned char>
    {
        size_t operator()(unsigned char x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = unsigned char;
        using result_type   = size_t;
    };

    template<>
    struct hash<char16_t>
    {
        size_t operator()(char16_t x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = char16_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<char32_t>
    {
        size_t operator()(char32_t x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = char32_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<wchar_t>
    {
        size_t operator()(wchar_t x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = wchar_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<short>
    {
        size_t operator()(short x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = short;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned short>
    {
        size_t operator()(unsigned short x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = unsigned short;
        using result_type   = size_t;
    };

    template<>
    struct hash<int>
    {
        size_t operator()(int x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = int;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned int>
    {
        size_t operator()(unsigned int x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = unsigned int;
        using result_type   = size_t;
    };

    template<>
    struct hash<long>
    {
        size_t operator()(long x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = long;
        using result_type   = size_t;
    };

    template<>
    struct hash<long long>
    {
        size_t operator()(long long x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = long long;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned long>
    {
        size_t operator()(unsigned long x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = unsigned long;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned long long>
    {
        size_t operator()(unsigned long long x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = unsigned long long;
        using result_type   = size_t;
    };

    template<>
    struct hash<float>
    {
        size_t operator()(float x) const noexcept
        {
            return hash_mix(*(size_t*)(&x));
        }

        using argument_type = float;
        using result_type   = size_t;
    };

    template<>
    struct hash<double>;
    {
        size_t operator()(double x) const noexcept
        {
            return hash_mix(*(size_t*)(&x));
        }

        using argument_type = double;
        using result_type   = size_t;
    };

    template<>
    struct hash<long double>;
    {
        size_t operator()(long double x) const noexcept
        {
            return hash_mix(*(size_t*)(&x));
        }

        using argument_type = long double;
        using result_type   = size_t;
    };

    template<class T>
    struct hash<T*>
    {
        size_t operator()(T* x) const noexcept
        {
            return hash_mix(static_cast<size_t>(x));
        }

        using argument_type = T*;
        using result_type   = size_t;
    };
}

#endif
