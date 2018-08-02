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

#ifndef LIBCPP_BITS_IO_STREAMBUF
#define LIBCPP_BITS_IO_STREAMBUF

#include <ios>
#include <iosfwd>
#include <locale>

namespace std
{

    /**
     * 27.6.3, class template basic_streambuf:
     */

    template<class Char, class Traits>
    class basic_streambuf
    {
        public:
            using traits_type = Traits;
            using char_type   = Char;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            virtual ~basic_streambuf()
            { /* DUMMY BODY */ }

            /**
             * 27.6.3.2.1, locales:
             */

            locale pubimbue(const locale& loc)
            {
                imbue(loc);

                return locale_;
            }

            locale getloc() const
            {
                return locale_;
            }

            /**
             * 27.6.3.2.2, buffer and positioning:
             */

            basic_streambuf<Char, Traits>* pubsetbuf(char_type* s, streamsize n)
            {
                return setbuf(s, n);
            }

            pos_type pubseekoff(off_type off, ios_base::seekdir way,
                                ios_base::openmode which = ios_base::in | ios_base::out)
            {
                return seekoff(off, way, which);
            }

            pos_type pubseekpos(pos_type pos, ios_base::openmode which =
                                ios_base::in | ios_base::out)
            {
                return seekpos(pos, which);
            }

            int pubsync()
            {
                return sync();
            }

            /**
             * 27.6.3.2.3, get area:
             */

            streamsize in_avail()
            {
                if (read_avail_())
                    return egptr() - gptr();
                else
                    return showmanyc();
            }

            int_type snextc()
            {
                if (traits_type::eq(sbumpc(), traits_type::eof()))
                    return traits_type::eof();
                else
                    return sgetc();
            }

            int_type sbumpc()
            {
                if (read_avail_())
                    return traits_type::to_int_type(*input_next_++);
                else
                    return uflow();
            }

            int_type sgetc()
            {
                if (read_avail_())
                    return traits_type::to_int_type(*input_next_);
                else
                    return underflow();
            }

            streamsize sgetn(char_type* s, streamsize n)
            {
                return xsgetn(s, n);
            }

            /**
             * 27.6.2.4, putback:
             */

            int_type sputbackc(char_type c)
            {
                if (!putback_avail_() || traits_type::eq(c, gptr()[-1]))
                    return pbackfail(traits_type::to_int_type(c));
                else
                    return traits_type::to_int_type(*(--input_next_));
            }

            int_type sungetc()
            {
                if (!putback_avail_())
                    return pbackfail();
                else
                    return traits_type::to_int_type(*(--input_next_));
            }

            /**
             * 27.6.2.5, put area:
             */

            int_type sputc(char_type c)
            {
                if (!write_avail_())
                    return overflow(traits_type::to_int_type(c));
                else
                {
                    traits_type::assign(*output_next_++, c);

                    return traits_type::to_int_type(c);
                }
            }

            streamsize sputn(const char_type* s, streamsize n)
            {
                return xsputn(s, n);
            }

        protected:
            basic_streambuf()
                : input_begin_{}, input_next_{}, input_end_{},
                  output_begin_{}, output_next_{}, output_end_{},
                  locale_{locale()}
            { /* DUMMY BODY */ }

            basic_streambuf(const basic_streambuf& rhs)
            {
                input_begin_ = rhs.input_begin_;
                input_next_ = rhs.input_next_;
                input_end_ = rhs.input_end_;

                output_begin_ = rhs.output_begin_;
                output_next_ = rhs.output_next_;
                output_end_ = rhs.output_end_;

                locale_ = rhs.locale_;
            }

            basic_streambuf& operator=(const basic_streambuf& rhs)
            {
                input_begin_ = rhs.input_begin_;
                input_next_  = rhs.input_next_;
                input_end_   = rhs.input_end_;

                output_begin_ = rhs.output_begin_;
                output_next_  = rhs.output_next_;
                output_end_   = rhs.output_end_;

                locale_ = rhs.locale_;

                return *this;
            }

            void swap(basic_streambuf& rhs)
            {
                swap(input_begin_, rhs.input_begin_);
                swap(input_next_, rhs.input_next_);
                swap(input_end_, rhs.input_end_);

                swap(output_begin_, rhs.output_begin_);
                swap(output_next_, rhs.output_next_);
                swap(output_end_, rhs.output_end_);

                swap(locale_, rhs.locale_);
            }

            /**
             * 27.6.3.3.2, get area:
             */

            char_type* eback() const
            {
                return input_begin_;
            }

            char_type* gptr() const
            {
                return input_next_;
            }

            char_type* egptr() const
            {
                return input_end_;
            }

            void gbump(int n)
            {
                input_next_ += n;
            }

            void setg(char_type* gbeg, char_type* gnext, char_type* gend)
            {
                input_begin_ = gbeg;
                input_next_  = gnext;
                input_end_   = gend;
            }

            /**
             * 27.6.3.3.3, put area:
             */

            char_type* pbase() const
            {
                return output_begin_;
            }

            char_type* pptr() const
            {
                return output_next_;
            }

            char_type* epptr() const
            {
                return output_end_;
            }

            void pbump(int n)
            {
                output_next_ += n;
            }

            void setp(char_type* pbeg, char_type* pend)
            {
                output_begin_ = pbeg;
                output_next_  = pbeg;
                output_end_   = pend;
            }

            /**
             * 27.6.3.4.1, locales:
             */

            virtual void imbue(const locale& loc)
            { /* DUMMY BODY */ }

            /**
             * 27.6.3.4.2, buffer management and positioning:
             */

            virtual basic_streambuf<Char, Traits>*
            setbuf(char_type* s, streamsize n)
            {
                return this;
            }

            virtual pos_type seekoff(off_type off, ios_base::seekdir way,
                                     ios_base::openmode which = ios_base::in | ios_base::out)
            {
                return pos_type(off_type(-1));
            }

            virtual pos_type seekpos(pos_type pos, ios_base::openmode which =
                                     ios_base::in | ios_base::out)
            {
                return pos_type(off_type(-1));
            }

            virtual int sync()
            {
                return 0;
            }

            /**
             * 27.6.3.4.3, get area:
             */

            virtual streamsize showmanyc()
            {
                return 0;
            }

            virtual streamsize xsgetn(char_type* s, streamsize n)
            {
                if (!s || n == 0)
                    return 0;

                streamsize i{0};
                auto eof = traits_type::eof();
                for (; i <= n; ++i)
                {
                    if (!read_avail_() && traits_type::eq_int_type(uflow(), eof))
                        break;

                    *s++ = *input_next_++;
                }

                return i;
            }

            virtual int_type underflow()
            {
                return traits_type::eof();
            }

            virtual int_type uflow()
            {
                auto res = underflow();
                if (traits_type::eq_int_type(res, traits_type::eof()))
                    return traits_type::eof();
                else
                    return traits_type::to_int_type(*input_next_++);
            }

            /**
             * 27.6.3.4.4, putback:
             */

            virtual int_type pbackfail(int_type c = traits_type::eof())
            {
                return traits_type::eof();
            }

            /**
             * 27.6.3.4.5, put area:
             */

            virtual streamsize xsputn(const char_type* s, streamsize n)
            {
                if (!s || n == 0)
                    return 0;

                streamsize i{0};
                for (; i <= n; ++i)
                {
                    if (!write_avail_() && traits_type::eq_int_type(overflow(), traits_type::eof()))
                        break;

                    *output_next_++ = *s++;
                }

                return i;
            }

            virtual int_type overflow(int_type c = traits_type::eof())
            {
                return traits_type::eof();
            }

        protected:
            char_type* input_begin_;
            char_type* input_next_;
            char_type* input_end_;

            char_type* output_begin_;
            char_type* output_next_;
            char_type* output_end_;

            locale locale_;

            bool write_avail_() const
            {
                return output_next_ && output_next_ < output_end_;
            }

            bool putback_avail_() const
            {
                return input_next_ && input_begin_ < input_next_;
            }

            bool read_avail_() const
            {
                return input_next_ && input_next_ < input_end_;
            }
    };

    using streambuf  = basic_streambuf<char>;
    using wstreambuf = basic_streambuf<wchar_t>;
}

#endif
