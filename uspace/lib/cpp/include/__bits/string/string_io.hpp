/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_STRING_IO
#define LIBCPP_BITS_STRING_IO

#include <__bits/string/string.hpp>
#include <ios>

namespace std
{
    /**
     * 21.4.8.9, inserters and extractors:
     */

    template<class Char, class Traits, class Allocator>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            basic_string<Char, Traits, Allocator>& str)
    {
        using sentry = typename basic_istream<Char, Traits>::sentry;
        sentry sen{is, false};

        if (sen)
        {
            str.erase();

            auto max_size = is.width();
            if (max_size <= 0)
                max_size = static_cast<streamsize>(str.max_size());

            streamsize i{};
            for(; i < max_size; ++i)
            {
                auto ic = is.rdbuf()->sgetc();
                if (Traits::eq_int_type(ic, Traits::eof()))
                {
                    is.setstate(ios_base::eofbit);
                    break;
                }

                auto c = Traits::to_char_type(ic);
                if(isspace(c, is.getloc()))
                    break;

                str.push_back(c);
                is.rdbuf()->sbumpc();
            }

            if (i == 0)
                is.setstate(ios_base::failbit);
        }
        else
            is.setstate(ios_base::failbit);

        return is;
    }

    template<class Char, class Traits, class Allocator>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os,
                                            const basic_string<Char, Traits, Allocator>& str)
    {
        // TODO: determine padding as described in 27.7.3.6.1
        using sentry = typename basic_ostream<Char, Traits>::sentry;
        sentry sen{os};

        if (sen)
        {
            auto width = os.width();
            auto size = str.size();

            size_t to_pad{};
            if (width > 0)
                to_pad = (static_cast<size_t>(width) - size);

            if (to_pad > 0)
            {
                if ((os.flags() & ios_base::adjustfield) != ios_base::left)
                {
                    for (std::size_t i = 0; i < to_pad; ++i)
                        os.put(os.fill());
                }

                os.rdbuf()->sputn(str.data(), size);

                if ((os.flags() & ios_base::adjustfield) == ios_base::left)
                {
                    for (std::size_t i = 0; i < to_pad; ++i)
                        os.put(os.fill());
                }
            }
            else
                os.rdbuf()->sputn(str.data(), size);

            os.width(0);
        }

        return os;
    }

    template<class Char, class Traits, class Allocator>
    basic_istream<Char, Traits>& getline(basic_istream<Char, Traits>& is,
                                         basic_string<Char, Traits, Allocator>& str,
                                         Char delim)
    {
        typename basic_istream<Char, Traits>::sentry sen{is, true};

        if (sen)
        {
            str.clear();
            streamsize count{};

            while (true)
            {
                auto ic = is.rdbuf()->sbumpc();
                if (Traits::eq_int_type(ic, Traits::eof()))
                {
                    is.setstate(ios_base::eofbit);
                    break;
                }

                auto c = Traits::to_char_type(ic);
                if (Traits::eq(c, delim))
                    break;

                str.push_back(c);
                ++count;

                if (count >= static_cast<streamsize>(str.max_size()))
                {
                    is.setstate(ios_base::failbit);
                    break;
                }
            }

            if (count == 0)
                is.setstate(ios_base::failbit);
        }
        else
            is.setstate(ios_base::failbit);

        return is;
    }

    template<class Char, class Traits, class Allocator>
    basic_istream<Char, Traits>& getline(basic_istream<Char, Traits>&& is,
                                         basic_string<Char, Traits, Allocator>& str,
                                         Char delim)
    {
        return getline(is, str, delim);
    }

    template<class Char, class Traits, class Allocator>
    basic_istream<Char, Traits>& getline(basic_istream<Char, Traits>& is,
                                         basic_string<Char, Traits, Allocator>& str)
    {
        return getline(is, str, is.widen('\n'));
    }

    template<class Char, class Traits, class Allocator>
    basic_istream<Char, Traits>& getline(basic_istream<Char, Traits>&& is,
                                         basic_string<Char, Traits, Allocator>& str)
    {
        return getline(is, str, is.widen('\n'));
    }
}

#endif
