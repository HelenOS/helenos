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

#ifndef LIBCPP_STREAMBUF
#define LIBCPP_STREAMBUF

#include <ios>
#include <iosfwd>
#include <locale>

namespace std
{

    /**
     * 27.6.3, class template basic_streambuf:
     */

    template<class Char, class Traits = character_traits<Char>>
    class basic_streambuf
    {
        public:
            using traits_type = Traits;
            using char_type   = Char;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            virtual ~basic_streambuf()
            {
                // TODO: implement
            }

            /**
             * 27.6.3.2.1, locales:
             */

            locale pubimbue(const locale& loc)
            {
                // TODO: implement
            }

            locale& getloc() const
            {
                // TODO: implement
            }

            /**
             * 27.6.3.2.2, buffer and positioning:
             */

            basic_streambuf<Char, Traits>* pubsetbuf(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            pos_type pubseekoff(off_type off, ios_base::seekdir way,
                                ios_base::openmode which = ios_base::in | ios_base::out)
            {
                // TODO: implement
            }

            pos_type pubseekpos(pos_type pos, ios_base::openmode which =
                                ios_base::in | ios_base::out)
            {
                // TODO: implement
            }

            int pubsync()
            {
                // TODO: implement
            }

            /**
             * 27.6.3.2.3, get area:
             */

            streamsize in_avail()
            {
                // TODO: implement
            }

            int_type snextc()
            {
                // TODO: implement
            }

            int_type sbumpc()
            {
                // TODO: implement
            }

            int_type sgetc()
            {
                // TODO: implement
            }

            streamsize sgetn(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            /**
             * 27.6.2.4, putback:
             */

            int_type sputbackc(char_type c)
            {
                // TODO: implement
            }

            int_type sungetc()
            {
                // TODO: implement
            }

            /**
             * 27.6.2.5, put area:
             */

            int_type sputc(char_type c)
            {
                // TODO: implement
            }

            streamsize sputn(const char_type* s, streamsize n)
            {
                // TODO: implement
            }

        protected:
            basic_streambuf()
            {
                // TODO: implement
            }

            basic_streambuf(const basic_streambuf& rhs)
            {
                // TODO: implement
            }

            basic_strambuf& operator=(const basic_streambuf& rhs)
            {
                // TODO: implement
            }

            void swap(basic_streambuf& rhs)
            {
                // TODO: implement
            }

            /**
             * 27.6.3.3.2, get area:
             */

            char_type* eback() const
            {
                // TODO: implement
            }

            char_type* gptr() const
            {
                // TODO: implement
            }

            char_type* egptr() const
            {
                // TODO: implement
            }

            void gbump(int n)
            {
                // TODO: implement
            }

            void setg(char_type* gbeg, char_type* gnext, char_type* gend)
            {
                // TODO: implement
            }

            /**
             * 27.6.3.3.3, put area:
             */

            char_type* pbase() const
            {
                // TODO: implement
            }

            char_type* pptr() const
            {
                // TODO: implement
            }

            char_type* epptr() const
            {
                // TODO: implement
            }

            void pbump(int n)
            {
                // TODO: implement
            }

            void setp(char_type* pbeg, char_type* pend)
            {
                // TODO: implement
            }

            /**
             * 27.6.3.4.1, locales:
             */

            virtual void imbue(const locale& loc)
            {
                // TODO: implement
            }

            /**
             * 27.6.3.4.2, buffer management and positioning:
             */

            virtual basic_streambuf<Char, Traits>*
            setbuf(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            virtual pos_type seekoff(off_type off, ios_base::seekdir way,
                                     ios_base::openmode which = ios_base::in | ops_base::out)
            {
                // TODO: implement
            }

            virtual pos_type seekpos(pos_type pos, ios_base::openmode which =
                                     ios_base::in | ios_base::out)
            {
                // TODO: implement
            }

            virtual int sync()
            {
                // TODO: implement
            }

            /**
             * 27.6.3.4.3, get area:
             */

            virtual streamsize showmanyc()
            {
                // TODO: implement
            }

            virtual streamsize xsgetn(char_type* s, streamsize n)
            {
                // TODO: implement
            }

            virtual int_type underflow()
            {
                // TODO: implement
            }

            virtual int_type uflow()
            {
                // TODO: implement
            }

            /**
             * 27.6.3.4.4, putback:
             */

            virtual int_type pbackfail(int_type c = traits_type::eof())
            {
                // TODO: implement
            }

            /**
             * 27.6.3.4.5, put area:
             */

            virtual streamsize xsputn(const char_type* s, streamsize n)
            {
                // TODO: implement
            }

            virtual int_type overflow(int_type c = traits_type::eof())
            {
                // TODO: implement
            }

        private:
            // TODO: implement
    };

    using streambuf  = basic_streambuf<char>;
    using wstreambuf = basic_streambuf<wchar_t>;
}

#endif
