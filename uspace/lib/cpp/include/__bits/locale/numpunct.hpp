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

#ifndef LIBCPP_BITS_LOCALE_NUMPUNCT
#define LIBCPP_BITS_LOCALE_NUMPUNCT

#include <__bits/locale/locale.hpp>
#include <string>

namespace std
{
    /**
     * 22.4.3.1, class template numpunct:
     */

    template<class Char>
    class numpunct: public locale::facet
    {
        public:
            using char_type   = Char;
            using string_type = basic_string<Char>;

            explicit numpunct(size_t refs = 0);

            char_type decimal_point() const
            {
                return do_decimal_point();
            }

            char_type thousands_sep() const
            {
                return do_thousands_sep();
            }

            string_type grouping() const
            {
                return do_grouping();
            }

            string_type truename() const
            {
                return do_truename();
            }

            string_type falsename() const
            {
                return do_falsename();
            }

            ~numpunct();

        protected:
            char_type do_decimal_point() const;
            char_type do_thousands_sep() const;
            string_type do_grouping() const;
            string_type do_truename() const;
            string_type do_falsename() const;
    };

    template<>
    class numpunct<char>: public locale::facet
    {
        public:
            using char_type   = char;
            using string_type = basic_string<char>;

            explicit numpunct(size_t refs = 0)
            { /* DUMMY BODY */ }

            char_type decimal_point() const
            {
                return do_decimal_point();
            }

            char_type thousands_sep() const
            {
                return do_thousands_sep();
            }

            string_type grouping() const
            {
                return do_grouping();
            }

            string_type truename() const
            {
                return do_truename();
            }

            string_type falsename() const
            {
                return do_falsename();
            }

            ~numpunct()
            { /* DUMMY BODY */ }

        protected:
            char_type do_decimal_point() const
            {
                return '.';
            }

            char_type do_thousands_sep() const
            {
                return ',';
            }

            string_type do_grouping() const
            {
                return "";
            }

            string_type do_truename() const
            {
                return "true";
            }

            string_type do_falsename() const
            {
                return "false";
            }
    };

    template<>
    class numpunct<wchar_t>: public locale::facet
    {
        public:
            using char_type   = wchar_t;
            using string_type = basic_string<wchar_t>;

            explicit numpunct(size_t refs = 0);

            char_type decimal_point() const
            {
                return do_decimal_point();
            }

            char_type thousands_sep() const
            {
                return do_thousands_sep();
            }

            string_type grouping() const
            {
                return do_grouping();
            }

            string_type truename() const
            {
                return do_truename();
            }

            string_type falsename() const
            {
                return do_falsename();
            }

            ~numpunct();

        protected:
            char_type do_decimal_point() const
            {
                return L'.';
            }

            char_type do_thousands_sep() const
            {
                return L',';
            }

            string_type do_grouping() const
            {
                return L"";
            }

            string_type do_truename() const
            {
                return L"true";
            }

            string_type do_falsename() const
            {
                return L"false";
            }
    };
}

#endif
