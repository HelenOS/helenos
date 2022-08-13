/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
