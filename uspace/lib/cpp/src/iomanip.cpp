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

#include <__bits/io/iomanip_objs.hpp>
#include <iomanip>

namespace std
{
    namespace aux
    {
        /**
         * Manipulator return types:
         */

        resetiosflags_t::resetiosflags_t(ios_base::fmtflags m)
            : mask{m}
        { /* DUMMY BODY */ }

        void resetiosflags_t::operator()(ios_base& str) const
        {
            str.setf(ios_base::fmtflags{0}, mask);
        }

        setiosflags_t::setiosflags_t(ios_base::fmtflags m)
            : mask{m}
        { /* DUMMY BODY */ }

        void setiosflags_t::operator()(ios_base& str) const
        {
            str.setf(mask);
        }

        setbase_t::setbase_t(int b)
            : base{b}
        { /* DUMMY BODY */ }

        void setbase_t::operator()(ios_base& str) const
        {
            str.setf(
                base == 8  ? ios_base::oct :
                base == 10 ? ios_base::dec :
                base == 16 ? ios_base::hex :
                ios_base::fmtflags{0},
                ios_base::basefield
            );
        }

        setprecision_t::setprecision_t(int p)
            : prec{p}
        { /* DUMMY BODY */ }

        void setprecision_t::operator()(ios_base& str) const
        {
            str.precision(prec);
        }

        setw_t::setw_t(int w)
            : width{w}
        { /* DUMMY BODY */ }

        void setw_t::operator()(ios_base& str) const
        {
            str.width(width);
        }
    }

    /**
     * Manipulators:
     */

    aux::manip_wrapper<aux::resetiosflags_t> resetiosflags(ios_base::fmtflags mask)
    {
        return aux::manip_wrapper<aux::resetiosflags_t>{mask};
    }

    aux::manip_wrapper<aux::setiosflags_t> setiosflags(ios_base::fmtflags mask)
    {
        return aux::manip_wrapper<aux::setiosflags_t>{mask};
    }

    aux::manip_wrapper<aux::setbase_t> setbase(int base)
    {
        return aux::manip_wrapper<aux::setbase_t>{base};
    }

    aux::manip_wrapper<aux::setprecision_t> setprecision(int prec)
    {
        return aux::manip_wrapper<aux::setprecision_t>{prec};
    }

    aux::manip_wrapper<aux::setw_t> setw(int width)
    {
        return aux::manip_wrapper<aux::setw_t>{width};
    }
}
