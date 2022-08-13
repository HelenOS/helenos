/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_MEMORY_ALLOCATOR_ARG
#define LIBCPP_BITS_MEMORY_ALLOCATOR_ARG

namespace std
{
    /**
     * 20.7.6, allocator argument tag:
     */

    struct allocator_arg_t
    { /* DUMMY BODY */ };

    constexpr allocator_arg_t allocator_arg{};
}

#endif
