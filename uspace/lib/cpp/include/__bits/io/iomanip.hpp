/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_IO_IOMANIP
#define LIBCPP_BITS_IO_IOMANIP

#include <__bits/io/iomanip_objs.hpp>
#include <iosfwd>

namespace std
{
    /**
     * 27.7.4, standard manipulators:
     */

    aux::manip_wrapper<aux::resetiosflags_t> resetiosflags(ios_base::fmtflags mask);
    aux::manip_wrapper<aux::setiosflags_t> setiosflags(ios_base::fmtflags mask);
    aux::manip_wrapper<aux::setbase_t> setbase(int base);

    template<class Char>
    aux::setfill_t<Char> setfill(Char c)
    {
        return aux::setfill_t<Char>{c};
    }

    aux::manip_wrapper<aux::setprecision_t> setprecision(int prec);
    aux::manip_wrapper<aux::setw_t> setw(int width);

    /**
     * 27.7.5, extended manipulators:
     */

    // TODO: implement
}

#endif
