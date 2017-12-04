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

#ifndef LIBCPP_OSTREAM
#define LIBCPP_OSTREAM

#include <ios>
#include <iosfwd>
#include <locale>

namespace std
{
    /**
     * 27.7.3.1, class template basic_ostream:
     */

    template<class Char, class Traits>
    class basic_ostream: virtual public basic_ios<Char, Traits>
    {
        public:
            using char_type   = Char;
            using traits_type = Traits;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            /**
             * 27.7.3.2, constructor/destructor:
             */

            explicit basic_ostream(basic_streambuf<char_type, traits_type>* sb)
            {
                basic_ios<Char, Traits>::init(sb);
            }

            virtual ~basic_ostream()
            { /* DUMMY BODY */ }

            /**
             * 27.7.3.4, prefix/suffix:
             */

            class sentry
            {
                public:
                    explicit sentry(basic_ostream<Char, Traits>& os)
                        : os_{os}, ok_{false}
                    {
                        if (os.good())
                        {
                            if (os.tie())
                                os.tie()->flush();
                        }

                        ok_ = os.good();
                    }

                    ~sentry()
                    {
                        if ((os_.flags() & ios_base::unitbuf) && os_.good())
                        {
                            auto ret = os_.rdbuf()->pubsync();
                            (void)ret;
                            // TODO: if ret == -1, set badbit in rdstate
                        }
                    }

                    explicit operator bool() const
                    {
                        return ok_;
                    }

                    sentry(const sentry&) = delete;
                    sentry& operator=(const sentry&) = delete;

                private:
                    basic_ostream<Char, Traits>& os_;
                    bool ok_;
            };

            /**
             * 27.7.3.6, formatted output:
             */

            basic_ostream<Char, Traits>& operator<<(
                basic_ostream<Char, Traits>& (*pf)(basic_ostream<Char, Traits>&)
            )
            {
                return pf(*this);
            }

            basic_ostream<Char, Traits>& operator<<(
                basic_ios<Char, Traits>& (*pf)(basic_ios<Char, Traits>&)
            )
            {
                pf(*this);

                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(
                ios_base& (*pf)(ios_base&)
            )
            {
                pf(*this);

                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(bool x)
            {
                // TODO: sentry etc
                /* bool failed = use_facet< */
                /*     num_put<char_type, ostreambuf_iterator<char_type, traits_type>> */
                /* >(this->getloc()).put(*this, *this, this->fill(), x).failed(); */

                /* if (failed) */
                /*     this->setstate(ios_base::badbit); */

                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(short x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(unsigned short x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(int x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(unsigned int x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(long x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(unsigned long x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(long long x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(unsigned long long x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(float x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(double x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(long double x)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(const void* p)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& operator<<(basic_streambuf<Char, Traits>* sb)
            {
                // TODO: implement
                return *this;
            }

            /**
             * 27.7.3.7, unformatted output:
             * TODO: when we have exceptions, complete these
             */

            basic_ostream<Char, Traits>& put(char_type c)
            {
                sentry sen{*this};

                if (sen)
                {
                    auto ret = this->rdbuf()->sputc(c);
                    if (traits_type::eq_int_type(ret, traits_type::eof()))
                        this->setstate(ios_base::badbit);
                }

                return *this;
            }

            basic_ostream<Char, Traits>& write(const char_type* s, streamsize n)
            {
                sentry sen{*this};

                if (sen)
                {
                    for (streamsize i = 0; i < n; ++i)
                    {
                        auto ret = this->rdbuf()->sputc(s[i]);
                        if (traits_type::eq_int_type(ret, traits_type::eof()))
                        {
                            this->setstate(ios_base::badbit);
                            break;
                        }
                    }
                }

                return *this;
            }

            basic_ostream<Char, Traits>& flush()
            {
                if (this->rdbuf())
                {
                    sentry sen{*this};

                    if (sen)
                    {
                        auto ret = this->rdbuf()->pubsync();
                        if (ret == -1)
                            this->setstate(ios_base::badbit);
                    }
                }

                return *this;
            }

            /**
             * 27.7.3.5, seeks:
             */

            pos_type tellp()
            {
                // TODO: implement
                return pos_type{};
            }

            basic_ostream<Char, Traits>& seekp(pos_type pos)
            {
                // TODO: implement
                return *this;
            }

            basic_ostream<Char, Traits>& seekp(off_type off, ios_base::seekdir dir)
            {
                // TODO: implement
                return *this;
            }

        protected:
            basic_ostream(const basic_ostream&) = delete;

            basic_ostream(basic_ostream&& other)
            {
                basic_ios<Char, Traits>::move(other);
            }

            /**
             * 27.7.3.3, assign/swap:
             */

            basic_ostream& operator=(const basic_ostream&) = delete;

            basic_ostream& operator=(basic_ostream&& other)
            {
                swap(other);

                return *this;
            }

            void swap(basic_ostream& rhs)
            {
                basic_ios<Char, Traits>::swap(rhs);
            }
    };

    using ostream  = basic_ostream<char>;
    using wostream = basic_ostream<wchar_t>;

    template<class Char, class Traits = char_traits<Char>>
    basic_ostream<Char, Traits>& endl(basic_ostream<Char, Traits>& os)
    {
        os.put('\n');
        os.flush();

        return os;
    }

    template<class Char, class Traits = char_traits<Char>>
    basic_ostream<Char, Traits>& ends(basic_ostream<Char, Traits>& os);

    template<class Char, class Traits = char_traits<Char>>
    basic_ostream<Char, Traits>& flush(basic_ostream<Char, Traits>& os);

    template<class Char, class Traits = char_traits<Char>, class T>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os, const T& x);
}

#endif

