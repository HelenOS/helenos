/*
 * Copyright (c) 2017 Jaroslav Jindrak
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

#ifndef LIBCPP_ISTREAM
#define LIBCPP_ISTREAM

#include <iosfwd>

namespace std
{

    /**
     * 27.7.2.1, class template basic_stream:
     */

    template<class Char, class Traits = char_traits<Char>>
    class basic_istream: virtual public basic_ios<Char, Traits>
    {
        public:
            using traits_type = Traits;
            using char_type   = Char;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            /**
             * 27.7.2.1.1, constructor/destructor:
             */

            explicit basic_istream(basic_streambuf<Char, Traits>* sb)
            {
                // TODO: implement
            }

            virtual ~basic_stream()
            {
                // TODO: implement
            }

            /**
             * 27.7.2.1.3, prefix/suffix:
             */

            class sentry;

            /**
             * 27.7.2.2, formatted input:
             */

            basic_istream<Char, Traits> operator>>(
                basic_istream<Char, Traits>& (*pf)(basic_istream<Char, Traits>&)
            )
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(
                basic_ios<Char, Traits>& (*pf)(basic_ios<Char, Traits>&)
            )
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(
                ios_base& (*pf)(ios_base&)
            )
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(bool& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(short& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(unsigned short& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(int& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(unsigned int& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(long& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(unsigned long& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(long long& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(unsigned long long& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(float& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(double& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(long double& x)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(void*& p)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits> operator>>(basic_streambuf<Char, Traits>* sb)
            {
                // TODO: implement
            }

            /**
             * 27.7.2.3, unformatted input:
             */

            streamsize gcount() const
            {
                // TODO: implement
            }

            int_type get()
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& get(char_type& c)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& get(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& get(char_type* s, streamsize n, char_type delim)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& get(basic_streambuf<Char, Traits>& sb)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& get(basic_streambuf<Char, Traits>& sb, char_type delim)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& getline(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& getline(char_type* s, streamsize n, char_type delim)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& ignore(streamsize n = 1, int_type delim = traits_type::eof())
            {
                // TODO: implement
            }

            int_type peek()
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& read(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            streamsize readsome(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& putback(char_type c)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& unget()
            {
                // TODO: implement
            }

            int sync()
            {
                // TODO: implement
            }

            pos_type tellg()
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& seekg(pos_type pos)
            {
                // TODO: implement
            }

            basic_istream<Char, Traits>& seekg(off_type off, ios_base::seekdir way)
            {
                // TODO: implement
            }

        protected:
            basic_istream(const basic_istream&) = delete;

            basic_istream(basic_istream&& rhs)
            {
                // TODO: implement
            }

            /**
             * 27.7.2.1.2, assign/swap:
             */

            basic_istream& operator=(const basic_istream& rhs) = delete;

            basic_istream& operator=(basic_istream&& rhs)
            {
                // TODO: implement
            }

            void swap(basic_stream& rhs)
            {
                // TODO: implement
            }
    };

    /**
     * 27.7.2.2.3, character extraction templates:
     */

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            Char& c)
    {
        // TODO: implement
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            unsigned char& c)
    {
        // TODO: implement
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            signed char& c)
    {
        // TODO: implement
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            Char* c)
    {
        // TODO: implement
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            unsigned char* c)
    {
        // TODO: implement
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            signed char* c)
    {
        // TODO: implement
    }

    using istream  = basic_istream<char>;
    using wistream = basic_istream<wchar_t>;

    template<class Char, class Traits = char_traits<Char>>
    class basic_iostream;

    using iostream  = basic_iostream<char>;
    using wiostream = basic_iostream<wchar_t>;

    template<class Char, class Traits = char_traits<Char>>
    basic_istream<Char, Traits>& ws(basic_istream<Char, Traits>& is);

    template<class Char, class Tratis = char_traits<Char>>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is, T& x);
}

#endif
