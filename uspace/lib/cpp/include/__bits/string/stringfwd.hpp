/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_STRING_STRINGFWD
#define LIBCPP_BITS_STRING_STRINGFWD

namespace std
{
    template<class Char>
    struct char_traits;

    template<class T>
    struct allocator;

    template<
        class Char,
        class Traits = char_traits<Char>,
        class Allocator = allocator<Char>
    >
    class basic_string;

    using string  = basic_string<char>;
    using wstring = basic_string<wchar_t>;

}

#endif
