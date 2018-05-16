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
