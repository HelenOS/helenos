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
