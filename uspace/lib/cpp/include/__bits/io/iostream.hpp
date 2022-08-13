/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_IO_IOSTREAM
#define LIBCPP_BITS_IO_IOSTREAM

#include <ios>
#include <streambuf>
#include <istream>
#include <ostream>

namespace std
{
    extern istream cin;
    extern ostream cout;

    // TODO: add the rest

    namespace aux
    {
        extern ios_base::Init init;
    }
}

#endif
