/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_FUNCTIONAL_HASH
#define LIBCPP_BITS_FUNCTIONAL_HASH

#include <cstdlib>
#include <cstdint>

namespace std
{
    template<class>
    struct is_arithmetic;

    template<class>
    struct is_pointer;

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
            static_assert(is_arithmetic<T>::value || is_pointer<T>::value,
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
