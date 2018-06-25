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
