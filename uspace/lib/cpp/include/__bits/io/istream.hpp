/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#ifndef LIBCPP_BITS_IO_ISTREAM
#define LIBCPP_BITS_IO_ISTREAM

#include <cassert>
#include <ios>
#include <iosfwd>
#include <limits>
#include <locale>
#include <ostream>
#include <utility>

namespace std
{

    /**
     * 27.7.2.1, class template basic_stream:
     */

    template<class Char, class Traits>
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
                : gcount_{0}
            {
                basic_ios<Char, Traits>::init(sb);
            }

            virtual ~basic_istream()
            { /* DUMMY BODY */ }

            /**
             * 27.7.2.1.3, prefix/suffix:
             */

            class sentry
            {
                public:
                    explicit sentry(basic_istream<Char, Traits>& is, bool noskipws = false)
                        : ok_{false}
                    {
                        if (!is.good())
                            is.setstate(ios_base::failbit);
                        else
                        {
                            if (is.tie())
                                is.tie()->flush();

                            if (!noskipws && ((is.flags() & ios_base::skipws) != 0))
                            {
                                const auto& ct = use_facet<ctype<Char>>(is.getloc());
                                while (true)
                                {
                                    auto i = is.rdbuf()->sgetc();
                                    if (Traits::eq_int_type(i, Traits::eof()))
                                    {
                                        is.setstate(ios_base::failbit | ios_base::eofbit);
                                        break;
                                    }

                                    auto c = Traits::to_char_type(i);
                                    if (!ct.is(ctype_base::space, c))
                                        break;
                                    else
                                        is.rdbuf()->sbumpc();
                                }
                            }
                        }

                        if (is.good())
                            ok_ = true;
                    }

                    ~sentry() = default;

                    explicit operator bool() const
                    {
                        return ok_;
                    }

                    sentry(const sentry&) = delete;
                    sentry& operator=(const sentry&) = delete;

                private:
                    using traits_type = Traits;
                    bool ok_;
            };

            /**
             * 27.7.2.2, formatted input:
             */

            basic_istream<Char, Traits>& operator>>(
                basic_istream<Char, Traits>& (*pf)(basic_istream<Char, Traits>&)
            )
            {
                return pf(*this);
            }

            basic_istream<Char, Traits>& operator>>(
                basic_ios<Char, Traits>& (*pf)(basic_ios<Char, Traits>&)
            )
            {
                pf(*this);

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(
                ios_base& (*pf)(ios_base&)
            )
            {
                pf(*this);

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(bool& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, x);
                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(short& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    long tmp{};
                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, tmp);

                    if (tmp < numeric_limits<short>::min())
                    {
                        err |= ios_base::failbit;
                        x = numeric_limits<short>::min();
                    }
                    else if (numeric_limits<short>::max() < tmp)
                    {
                        err |= ios_base::failbit;
                        x = numeric_limits<short>::max();
                    }
                    else
                        x = static_cast<short>(tmp);

                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(unsigned short& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, x);
                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(int& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    long tmp{};
                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, tmp);

                    if (tmp < numeric_limits<int>::min())
                    {
                        err |= ios_base::failbit;
                        x = numeric_limits<int>::min();
                    }
                    else if (numeric_limits<int>::max() < tmp)
                    {
                        err |= ios_base::failbit;
                        x = numeric_limits<int>::max();
                    }
                    else
                        x = static_cast<int>(tmp);

                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(unsigned int& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, x);
                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(long& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, x);
                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(unsigned long& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, x);
                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(long long& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, x);
                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(unsigned long long& x)
            {
                sentry sen{*this, false};

                if (sen)
                {
                    using num_get = num_get<Char, istreambuf_iterator<Char, Traits>>;
                    auto err = ios_base::goodbit;

                    auto loc = this->getloc();
                    use_facet<num_get>(loc).get(*this, 0, *this, err, x);
                    this->setstate(err);
                }

                return *this;
            }

            basic_istream<Char, Traits>& operator>>(float& x)
            {
                // TODO: implement
                __unimplemented();
                return *this;
            }

            basic_istream<Char, Traits>& operator>>(double& x)
            {
                // TODO: implement
                __unimplemented();
                return *this;
            }

            basic_istream<Char, Traits>& operator>>(long double& x)
            {
                // TODO: implement
                __unimplemented();
                return *this;
            }

            basic_istream<Char, Traits>& operator>>(void*& p)
            {
                // TODO: implement
                __unimplemented();
                return *this;
            }

            basic_istream<Char, Traits>& operator>>(basic_streambuf<Char, Traits>* sb)
            {
                if (!sb)
                {
                    this->setstate(ios_base::failbit);
                    return *this;
                }

                gcount_ = 0;
                sentry sen{*this, false};

                if (sen)
                {
                    while (true)
                    {
                        auto ic = this->rdbuf()->sgetc();
                        if (traits_type::eq_int_type(ic, traits_type::eof()))
                        {
                            this->setstate(ios_base::eofbit);
                            break;
                        }

                        auto res = sb->sputc(traits_type::to_char_type(ic));
                        if (traits_type::eq_int_type(res, traits_type::eof()))
                            break;

                        ++gcount_;
                        this->rdbuf()->sbumpc();
                    }
                }

                return *this;
            }

            /**
             * 27.7.2.3, unformatted input:
             * TODO: Once we have exceptions, implement
             *       27.7.2.3 paragraph 1.
             */

            streamsize gcount() const
            {
                return gcount_;
            }

            int_type get()
            {
                gcount_ = 0;
                sentry sen{*this, true};

                if (sen)
                {
                    auto res = this->rdbuf()->sbumpc();
                    if (!traits_type::eq_int_type(res, traits_type::eof()))
                    {
                        gcount_ = 1;
                        return res;
                    }

                    this->setstate(ios_base::failbit | ios_base::eofbit);
                }

                return traits_type::eof();
            }

            basic_istream<Char, Traits>& get(char_type& c)
            {
                auto res = get();
                if (res != traits_type::eof())
                    c = traits_type::to_char_type(res);

                return *this;
            }

            basic_istream<Char, Traits>& get(char_type* s, streamsize n, char_type delim)
            {
                gcount_ = 0;
                sentry sen{*this, true};

                if (sen && n > 0)
                {
                    while(gcount_ < n - 1)
                    {
                        auto c = this->rdbuf()->sbumpc();

                        if (traits_type::eq_int_type(c, traits_type::eof()))
                        {
                            this->setstate(ios_base::eofbit);
                            break;
                        }

                        s[gcount_++] = traits_type::to_char_type(c);

                        auto peek = traits_type::to_char_type(this->rdbuf()->sgetc());
                        if (traits_type::eq(peek, delim))
                            break;
                    }

                    if (gcount_ == 0)
                        this->setstate(ios_base::failbit);
                    s[n] = char_type{};
                }

                return *this;
            }

            basic_istream<Char, Traits>& get(char_type* s, streamsize n)
            {
                return get(s, n, this->widen('\n'));
            }

            basic_istream<Char, Traits>& get(basic_streambuf<Char, Traits>& sb)
            {
                get(sb, this->widen('\n'));
            }

            basic_istream<Char, Traits>& get(basic_streambuf<Char, Traits>& sb, char_type delim)
            {
                gcount_ = 0;
                sentry sen{*this, true};

                if (sen)
                {
                    while (true)
                    {
                        auto i = this->rdbuf()->sgetc();
                        if (traits_type::eq_int_type(i, traits_type::eof()))
                        {
                            this->setstate(ios_base::eofbit);
                            break;
                        }

                        auto c = traits_type::to_char_type(i);
                        if (traits_type::eq(c, delim))
                            break;

                        auto insert_ret = sb.sputc(c);
                        if (traits_type::eq_int_type(insert_ret, traits_type::eof()))
                            break;

                        this->rdbuf()->sbumpc();
                        ++gcount_;
                    }

                    if (gcount_ == 0)
                        this->setstate(ios_base::failbit);
                }

                return *this;
            }

            basic_istream<Char, Traits>& getline(char_type* s, streamsize n)
            {
                return getline(s, n, this->widen('\n'));
            }

            basic_istream<Char, Traits>& getline(char_type* s, streamsize n, char_type delim)
            {
                gcount_ = 0;
                sentry sen{*this, true};

                if (sen)
                {
                    while (true)
                    { // We have exactly specified order of checks, easier to do them in the body.
                        auto c = this->rdbuf()->sbumpc();

                        if (traits_type::eq_int_type(c, traits_type::eof()))
                        {
                            this->setstate(ios_base::eofbit);
                            break;
                        }

                        if (traits_type::eq_int_type(c, traits_type::to_int_type(delim)))
                            break;

                        if (n < 1 || gcount_ >= n - 1)
                        {
                            this->setstate(ios_base::failbit);
                            break;
                        }

                        s[gcount_++] = traits_type::to_char_type(c);
                    }

                    if (gcount_ == 0)
                        this->setstate(ios_base::failbit);
                    if (n > 0)
                        s[gcount_] = char_type{};
                }

                return *this;
            }

            basic_istream<Char, Traits>& ignore(streamsize n = 1, int_type delim = traits_type::eof())
            {
                sentry sen{*this, true};

                if (sen)
                {
                    streamsize i{};
                    while (n == numeric_limits<streamsize>::max() || i < n)
                    {
                        auto c = this->rdbuf()->sbumpc();

                        if (traits_type::eq_int_type(c, traits_type::eof()))
                        {
                            this->setstate(ios_base::eofbit);
                            break;
                        }

                        if (traits_type::eq_int_type(c, delim))
                            break;
                    }
                }

                return *this;
            }

            int_type peek()
            {
                sentry sen{*this, true};

                if (!this->good())
                    return traits_type::eof();
                else
                    return this->rdbuf()->sgetc();
            }

            basic_istream<Char, Traits>& read(char_type* s, streamsize n)
            {
                gcount_ = 0;
                sentry sen{*this, true};

                if (!this->good())
                {
                    this->setstate(ios_base::failbit);
                    return *this;
                }

                while (gcount_ < n)
                {
                    auto c = this->rdbuf()->sbumpc();
                    if (traits_type::eq_int_type(c, traits_type::eof()))
                    {
                        this->setstate(ios_base::failbit | ios_base::eofbit);
                        break;
                    }

                    s[gcount_++] = traits_type::to_char_type(c);
                }

                return *this;
            }

            streamsize readsome(char_type* s, streamsize n)
            {
                gcount_ = 0;
                sentry sen{*this, true};

                if (!this->good())
                {
                    this->setstate(ios_base::failbit);
                    return streamsize{};
                }

                auto avail = this->rdbuf()->in_avail();
                if (avail == -1)
                {
                    this->setstate(ios_base::eofbit);
                    return streamsize{};
                } else if (avail > 0)
                {
                    auto count = (avail < n ? avail : n);
                    while (gcount_ < count)
                        s[gcount_++] = traits_type::to_char_type(this->rdbuf()->sbumpc());
                }

                return gcount_;
            }

            basic_istream<Char, Traits>& putback(char_type c)
            {
                this->clear(this->rdstate() & (~ios_base::eofbit));

                gcount_ = 0;
                sentry sen{*this, true};

                if (!this->good())
                {
                    this->setstate(ios_base::failbit);
                    return *this;
                }

                if (this->rdbuf())
                {
                    auto ret = this->rdbuf()->sputbackc(c);
                    if (traits_type::eq_int_type(ret, traits_type::eof()))
                        this->setstate(ios_base::badbit);
                }
                else
                    this->setstate(ios_base::badbit);

                return *this;
            }

            basic_istream<Char, Traits>& unget()
            {
                this->clear(this->rdstate() & (~ios_base::eofbit));

                gcount_ = 0;
                sentry sen{*this, true};

                if (!this->good())
                {
                    this->setstate(ios_base::failbit);
                    return *this;
                }

                if (this->rdbuf())
                {
                    auto ret = this->rdbuf()->sungetc();
                    if (traits_type::eq_int_type(ret, traits_type::eof()))
                        this->setstate(ios_base::badbit);
                }
                else
                    this->setstate(ios_base::badbit);

                return *this;
            }

            int sync()
            {
                sentry s{*this, true};

                if (this->rdbuf())
                {
                    auto ret = this->rdbuf()->pubsync();
                    if (ret == -1)
                    {
                        this->setstate(ios_base::badbit);
                        return -1;
                    }
                    else
                        return 0;
                }
                else
                    return -1;
            }

            pos_type tellg()
            {
                sentry s{*this, true};

                if (this->fail())
                    return pos_type(-1);
                else
                    return this->rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
            }

            basic_istream<Char, Traits>& seekg(pos_type pos)
            {
                this->clear(this->rdstate() & (~ios_base::eofbit));

                sentry sen{*this, true};

                if (!this->fail())
                    this->rdbuf()->pubseekoff(pos, ios_base::in);
                else
                    this->setstate(ios_base::failbit);

                return *this;
            }

            basic_istream<Char, Traits>& seekg(off_type off, ios_base::seekdir dir)
            {
                sentry sen{*this, true};

                if (!this->fail())
                    this->rdbuf()->pubseekoff(off, dir, ios_base::in);
                else
                    this->setstate(ios_base::failbit);

                return *this;
            }

        protected:
            streamsize gcount_;

            basic_istream(const basic_istream&) = delete;

            basic_istream(basic_istream&& rhs)
            {
                gcount_ = rhs.gcout_;

                basic_ios<Char, Traits>::move(rhs);

                rhs.gcount_ = 0;
            }

            /**
             * 27.7.2.1.2, assign/swap:
             */

            basic_istream& operator=(const basic_istream& rhs) = delete;

            basic_istream& operator=(basic_istream&& rhs)
            {
                swap(rhs);

                return *this;
            }

            void swap(basic_istream& rhs)
            {
                basic_ios<Char, Traits>::swap(rhs);
                swap(gcount_, rhs.gcount_);
            }
    };

    /**
     * 27.7.2.2.3, character extraction templates:
     */

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            Char& c)
    {
        typename basic_istream<Char, Traits>::sentry sen{is, false};

        if (sen)
        {
            auto ic = is.rdbuf()->sgetc();
            if (Traits::eq_int_type(ic, Traits::eof()))
            {
                is.setstate(ios_base::failbit | ios_base::eofbit);
                return is;
            }

            c = Traits::to_char_type(is.rdbuf()->sbumpc());
        }

        return is;
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            unsigned char& c)
    {
        return is >> reinterpret_cast<char&>(c);
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            signed char& c)
    {
        return is >> reinterpret_cast<char&>(c);
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            Char* str)
    {
        typename basic_istream<Char, Traits>::sentry sen{is, false};

        if (sen)
        {
            const auto& ct = use_facet<ctype<Char>>(is.getloc());

            size_t n{};
            if (is.width() > 0)
                n = static_cast<size_t>(is.width());
            else
                n = (numeric_limits<size_t>::max() / sizeof(Char)) - sizeof(Char);

            size_t i{};
            for (; i < n - 1; ++i)
            {
                auto ic = is.rdbuf()->sgetc();
                if (Traits::eq_int_type(ic, Traits::eof()))
                    break;

                auto c = Traits::to_char_type(ic);
                if (ct.is(ctype_base::space, c))
                    break;

                str[i] = c;
                is.rdbuf()->sbumpc();
            }

            str[i] = Char{};
            if (i == 0)
                is.setstate(ios_base::failbit);
        }

        return is;
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            unsigned char* str)
    {
        return is >> reinterpret_cast<char*>(str);
    }

    template<class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            signed char* str)
    {
        return is >> reinterpret_cast<char*>(str);
    }

    /**
     * 27.7.2.4, standard basic_istream manipulators:
     */

    template<class Char, class Traits = char_traits<Char>>
    basic_istream<Char, Traits>& ws(basic_istream<Char, Traits>& is)
    {
        using sentry = typename basic_istream<Char, Traits>::sentry;
        sentry sen{is, true};

        if (sen)
        {
            const auto& ct = use_facet<ctype<Char>>(is.getloc());
            while (true)
            {
                auto i = is.rdbuf()->sgetc();
                if (Traits::eq_int_type(i, Traits::eof()))
                {
                    is.setstate(ios_base::eofbit);
                    break;
                }

                auto c = Traits::to_char_type(i);
                if (!ct.is(c, ct.space))
                    break;
                else
                    is.rdbuf()->sbumpc();
            }
        }

        return is;
    }

    using istream  = basic_istream<char>;
    using wistream = basic_istream<wchar_t>;

    /**
     * 27.7.2.5, class template basic_iostream:
     */

    template<class Char, class Traits>
    class basic_iostream
        : public basic_istream<Char, Traits>,
          public basic_ostream<Char, Traits>
    {
        public:
            using char_type   = Char;
            using traits_type = Traits;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            explicit basic_iostream(basic_streambuf<char_type, traits_type>* sb)
                : basic_istream<char_type, traits_type>(sb),
                  basic_ostream<char_type, traits_type>(sb)
            { /* DUMMY BODY */ }

            virtual ~basic_iostream()
            { /* DUMMY BODY */ }

        protected:
            basic_iostream(const basic_iostream&) = delete;
            basic_iostream& operator=(const basic_iostream&) = delete;

            basic_iostream(basic_iostream&& other)
                : basic_istream<char_type, traits_type>(move(other))
            { /* DUMMY BODY */ }

            basic_iostream& operator=(basic_iostream&& other)
            {
                swap(other);
            }

            void swap(basic_iostream& other)
            {
                basic_istream<char_type, traits_type>::swap(other);
            }
    };

    using iostream  = basic_iostream<char>;
    using wiostream = basic_iostream<wchar_t>;

    /**
     * 27.7.2.6, rvalue stream extraction:
     */

    template<class Char, class Traits, class T>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>&& is, T& x)
    {
        is >> x;

        return is;
    }
}

#endif
