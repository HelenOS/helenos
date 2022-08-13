/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_MEMORY_ADDRESSOF
#define LIBCPP_BITS_MEMORY_ADDRESSOF

namespace std
{
    /**
     * 20.7.12, specialized algorithms:
     */

    template<class T>
    T* addressof(T& x) noexcept
    {
        return reinterpret_cast<T*>(
            &const_cast<char&>(
                reinterpret_cast<const volatile char&>(x)
        ));
    }
}

#endif
