/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
