/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_UTILITY_DECLVAL
#define LIBCPP_BITS_UTILITY_DECLVAL

#include <__bits/type_traits/references.hpp>

namespace std
{
    /**
     * 20.2.5, function template declval:
     * Note: This function only needs declaration, not
     *       implementation.
     */

    template<class T>
    add_rvalue_reference_t<T> declval() noexcept;
}

#endif
