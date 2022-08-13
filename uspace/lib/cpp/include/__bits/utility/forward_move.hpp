/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_UTILITY_FORWARD_MOVE
#define LIBCPP_BITS_UTILITY_FORWARD_MOVE

#include <__bits/type_traits/references.hpp>

namespace std
{
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
}

#endif
