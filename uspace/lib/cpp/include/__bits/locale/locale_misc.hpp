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

#ifndef LIBCPP_BITS_LOCALE_MISC
#define LIBCPP_BITS_LOCALE_MISC

#include <__bits/locale/locale.hpp>
#include <__bits/locale/ctype.hpp>
#include <__bits/locale/num_get.hpp>
#include <__bits/locale/num_put.hpp>
#include <__bits/locale/numpunct.hpp>
#include <cstdlib>
#include <ios>
#include <iosfwd>
#include <string>

namespace std
{
    /**
     * Note: This is a very simplistic implementation of <locale>.
     *       We have a single locale that has all of its facets.
     *       This should behave correctly on the outside but will prevent
     *       us from using multiple locales so if that becomes necessary
     *       in the future, TODO: implement :)
     */

    /**
     * 22.3.3, convenience interfaces:
     */

    /**
     * 22.3.3.1, character classification:
     */

    template<class Char>
    bool isspace(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::space, c);
    }

    template<class Char>
    bool isprint(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::print, c);
    }

    template<class Char>
    bool iscntrl(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::cntrl, c);
    }

    template<class Char>
    bool isupper(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::upper, c);
    }

    template<class Char>
    bool islower(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::lower, c);
    }

    template<class Char>
    bool isalpha(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::alpha, c);
    }

    template<class Char>
    bool isdigit(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::digit, c);
    }

    template<class Char>
    bool ispunct(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::punct, c);
    }

    template<class Char>
    bool isxdigit(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::xdigit, c);
    }

    template<class Char>
    bool isalnum(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::alnum, c);
    }

    template<class Char>
    bool isgraph(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::graph, c);
    }

    template<class Char>
    bool isblank(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::blank, c);
    }

    /**
     * 22.3.3.2, conversions:
     */

    /**
     * 22.3.3.2.1, character conversions:
     */

    template<class Char>
    Char toupper(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).toupper(c);
    }

    template<class Char>
    Char tolower(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).tolower(c);
    }

    /**
     * 22.3.3.2.2, string conversions:
     */

    // TODO: implement
}

#endif
