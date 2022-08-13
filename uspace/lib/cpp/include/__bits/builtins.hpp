/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_BUILTINS
#define LIBCPP_BITS_BUILTINS

/**
 * Note: While the primary target of libcpp is the
 *       g++ compiler, using builtin functions that would
 *       unnecessarily limit the library to any specific
 *       compiler is discouraged and as such all the used
 *       builtins should be available atleast in g++ and
 *       clang++.
 * GCC:  https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
 * LLVM: https://github.com/llvm-mirror/clang/blob/master/include/clang/Basic/Builtins.def
 *       (If anyone has a better link for LLVM, feel free to update it.)
 */

#include <cstdlib>

namespace std::aux
{
    template<class T>
    constexpr double log2(T val)
    {
        return __builtin_log2(static_cast<double>(val));
    }

    template<class T>
    constexpr double pow2(T exp)
    {
        return __builtin_pow(2.0, static_cast<double>(exp));
    }

    template<class T>
    constexpr size_t pow2u(T exp)
    {
        return static_cast<size_t>(pow2(exp));
    }

    template<class T, class U>
    constexpr T pow(T base, U exp)
    {
        return static_cast<T>(
            __builtin_pow(static_cast<double>(base), static_cast<double>(exp))
        );
    }

    template<class T>
    constexpr size_t ceil(T val)
    {
        return static_cast<size_t>(__builtin_ceil(static_cast<double>(val)));
    }

    template<class T>
    constexpr size_t floor(T val)
    {
        return static_cast<size_t>(__builtin_floor(static_cast<double>(val)));
    }
}

#endif
