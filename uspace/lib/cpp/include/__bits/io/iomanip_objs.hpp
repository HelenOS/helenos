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

#ifndef LIBCPP_BITS_IO_IOMANIP_OBJS
#define LIBCPP_BITS_IO_IOMANIP_OBJS

#include <ios>
#include <utility>

namespace std::aux
{
    /**
     * Note: This allows us to lower the amount
     *       of code duplication in this module
     *       as we can avoid operator<</>> overloading
     *       for the manipulators.
     */
    template<class Manipulator>
    struct manip_wrapper
    {
        template<class... Args>
        manip_wrapper(Args&&... args)
            : manipulator{forward<Args>(args)...}
        { /* DUMMY BODY */ }

        void operator()(ios_base& str)
        {
            manipulator(str);
        }

        Manipulator manipulator;
    };

    template<class Char, class Traits, class Manipulator>
    basic_ostream<Char, Traits>& operator<<(
        basic_ostream<Char, Traits>& os, manip_wrapper<Manipulator> manip
    )
    {
        manip(os);

        return os;
    }

    template<class Char, class Traits, class Manipulator>
    basic_istream<Char, Traits>& operator>>(
        basic_istream<Char, Traits>& is, manip_wrapper<Manipulator> manip
    )
    {
        manip(is);

        return is;
    }

    struct resetiosflags_t
    {
        resetiosflags_t(ios_base::fmtflags m);

        void operator()(ios_base& str) const;

        ios_base::fmtflags mask;
    };

    struct setiosflags_t
    {
        setiosflags_t(ios_base::fmtflags m);

        void operator()(ios_base& str) const;

        ios_base::fmtflags mask;
    };

    struct setbase_t
    {
        setbase_t(int b);

        void operator()(ios_base& str) const;

        int base;
    };

    /**
     * Note: setfill is only for basic_ostream so
     *       this is the only case where we overload
     *       the appropriate operator again.
     */
    template<class Char>
    struct setfill_t
    {
        setfill_t(Char c)
            : fill{c}
        { /* DUMMY BODY */ }

        template<class Traits>
        void operator()(basic_ios<Char, Traits>& str) const
        {
            str.fill(fill);
        }

        Char fill;
    };

    template<class Char, class Traits>
    basic_ostream<Char, Traits>& operator<<(
        basic_ostream<Char, Traits>& is,
        setfill_t<Char> manip
    )
    {
        manip(is);

        return is;
    }

    struct setprecision_t
    {
        setprecision_t(int);

        void operator()(ios_base&) const;

        int prec;
    };

    struct setw_t
    {
        setw_t(int);

        void operator()(ios_base&) const;

        int width;
    };
}

#endif
