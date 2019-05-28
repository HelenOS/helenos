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

#ifndef LIBCPP_BITS_IO_IOS
#define LIBCPP_BITS_IO_IOS

#include <__bits/locale/locale.hpp>
#include <__bits/locale/ctype.hpp>
#include <cstdlib>
#include <iosfwd>
#include <system_error>
#include <utility>
#include <vector>

namespace std
{
    using streamoff = long long;
    using streamsize = ::ssize_t;

    /**
     * 27.5.3, ios_base:
     */

    class ios_base
    {
        public:
            virtual ~ios_base();

            ios_base(const ios_base&) = delete;
            ios_base& operator=(const ios_base&) = delete;

            /**
             * 27.5.3.1.1, failure:
             */

            class failure: public system_error
            {
                // TODO: implement
            };

            /**
             * 27.5.3.1.2, fmtflags:
             */

            using fmtflags = uint16_t;
            static constexpr fmtflags boolalpha   = 0b0000'0000'0000'0001;
            static constexpr fmtflags dec         = 0b0000'0000'0000'0010;
            static constexpr fmtflags fixed       = 0b0000'0000'0000'0100;
            static constexpr fmtflags hex         = 0b0000'0000'0000'1000;
            static constexpr fmtflags internal    = 0b0000'0000'0001'0000;
            static constexpr fmtflags left        = 0b0000'0000'0010'0000;
            static constexpr fmtflags oct         = 0b0000'0000'0100'0000;
            static constexpr fmtflags right       = 0b0000'0000'1000'0000;
            static constexpr fmtflags scientific  = 0b0000'0001'0000'0000;
            static constexpr fmtflags showbase    = 0b0000'0010'0000'0000;
            static constexpr fmtflags showpoint   = 0b0000'0100'0000'0000;
            static constexpr fmtflags showpos     = 0b0000'1000'0000'0000;
            static constexpr fmtflags skipws      = 0b0001'0000'0000'0000;
            static constexpr fmtflags unitbuf     = 0b0010'0000'0000'0000;
            static constexpr fmtflags uppercase   = 0b0100'0000'0000'0000;
            static constexpr fmtflags adjustfield = left | right | internal;
            static constexpr fmtflags basefield   = dec  | oct   | hex;
            static constexpr fmtflags floatfield  = scientific   | fixed;

            /**
             * 27.5.3.1.3, iostate:
             */

            using iostate = uint8_t;
            static constexpr iostate badbit  = 0b0001;
            static constexpr iostate eofbit  = 0b0010;
            static constexpr iostate failbit = 0b0100;
            static constexpr iostate goodbit = 0b0000;

            /**
             * 27.5.3.1.4, openmode:
             */

            using openmode = uint8_t;
            static constexpr openmode app    = 0b00'0001;
            static constexpr openmode ate    = 0b00'0010;
            static constexpr openmode binary = 0b00'0100;
            static constexpr openmode in     = 0b00'1000;
            static constexpr openmode out    = 0b01'0000;
            static constexpr openmode trunc  = 0b10'0000;

            /**
             * 27.5.3.1.5, seekdir:
             */

            using seekdir = uint8_t;
            static constexpr seekdir beg = 0b001;
            static constexpr seekdir cur = 0b010;
            static constexpr seekdir end = 0b100;

            /**
             * 27.5.3.1.6, class Init:
             */

            class Init
            {
                public:
                    Init();
                    ~Init();

                private:
                    static int init_cnt_;
            };

            /**
             * 27.5.3.2, fmtflags state:
             */

            fmtflags flags() const;
            fmtflags flags(fmtflags fmtfl);
            fmtflags setf(fmtflags fmtfl);
            fmtflags setf(fmtflags fmtfl, fmtflags mask);
            void unsetf(fmtflags mask);

            streamsize precision() const;
            streamsize precision(streamsize prec);
            streamsize width() const;
            streamsize width(streamsize wide);

            /**
             * 27.5.3.3, locales:
             */

            locale imbue(const locale& loc);
            locale getloc() const;

            /**
             * 27.5.3.5, storage:
             */

            static int xalloc()
            {
                // TODO: Concurrent access to this function
                //       by multiple threads shall not result
                //       in a data race.
                return index_++;
            }

            long& iword(int index);
            void*& pword(int index);

            /**
             * 27.5.3.6, callbacks:
             */

            enum event
            {
                erase_event,
                imbue_event,
                copyfmt_event
            };

            using event_callback = void (*)(event, ios_base&, int);
            void register_callback(event_callback fn, int index);

            static bool sync_with_stdio(bool sync = true)
            {
                auto old = sync_;
                sync_ = sync;

                return old;
            }

        protected:
            ios_base();

            static int index_;
            static bool sync_;

            static long ierror_;
            static void* perror_;
            static constexpr size_t initial_size_{10};

            long* iarray_;
            void** parray_;
            size_t iarray_size_;
            size_t parray_size_;

            fmtflags flags_;
            streamsize precision_;
            streamsize width_;

            locale locale_;

            vector<pair<event_callback, int>> callbacks_;

        private:
            static constexpr size_t buffer_size_{64};
            char buffer_[buffer_size_];

            template<class Char, class Iterator>
            friend class num_put;

            template<class Char, class Iterator>
            friend class num_get;
    };

    /**
     * 27.5.4, fpos:
     */

    template<class State>
    class fpos
    {
        public:
            State state() const
            {
                return state_;
            }

            void state(State st)
            {
                state_ = st;
            }

        private:
            State state_;
    };

    /**
     * 27.5.4.2, fpos requirements:
     */

    // TODO: implement

    /**
     * 27.5.5, basic_ios:
     */

    template<class Char, class Traits>
    class basic_ios: public ios_base
    {
        public:
            using traits_type = Traits;
            using char_type   = Char;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            basic_ios(const basic_ios&) = delete;
            basic_ios& operator=(const basic_ios&) = delete;

            explicit operator bool() const
            {
                return !fail();
            }

            bool operator!() const
            {
                return fail();
            }

            iostate rdstate() const
            {
                return rdstate_;
            }

            void clear(iostate state = goodbit)
            {
                if (rdbuf_)
                    rdstate_ = state;
                else
                    rdstate_ = state | badbit;

                if (((state | (rdbuf_ ? goodbit : badbit)) & exceptions_) == 0)
                    return;
                // TODO: Else throw failure.
                return;
            }

            void setstate(iostate state)
            {
                clear(rdstate_ | state);
            }

            bool good() const
            {
                return rdstate_ == 0;
            }

            bool eof() const
            {
                return (rdstate_ & eofbit) != 0;
            }

            bool fail() const
            {
                return (rdstate_ & (failbit | badbit)) != 0;
            }

            bool bad() const
            {
                return (rdstate_ & badbit) != 0;
            }

            iostate exceptions() const
            {
                return exceptions_;
            }

            void exceptions(iostate except)
            {
                exceptions_ = except;
                clear(rdstate_);
            }

            /**
             * 27.5.5.2, constructor/destructor:
             */

            explicit basic_ios(basic_streambuf<Char, Traits>* sb)
            {
                init(sb);
            }

            virtual ~basic_ios()
            { /* DUMMY BODY */ }

            /**
             * 27.5.5.3, members:
             */

            basic_ostream<Char, Traits>* tie() const
            {
                return tie_;
            }

            basic_ostream<Char, Traits>* tie(basic_ostream<Char, Traits>* tiestr)
            {
                auto old = tie_;
                tie_ = tiestr;

                return old;
            }

            basic_streambuf<Char, Traits>* rdbuf() const
            {
                return rdbuf_;
            }

            basic_streambuf<Char, Traits>* rdbuf(basic_streambuf<Char, Traits>* sb)
            {
                auto old = rdbuf_;
                rdbuf_ = sb;

                clear();

                return old;
            }

            basic_ios& copyfmt(const basic_ios& rhs)
            {
                if (this == &rhs)
                    return *this;

                for (auto& callback: callbacks_)
                    callback.first(erase_event, *this, index_);

                tie_        = rhs.tie_;
                flags_      = rhs.flags_;
                width_      = rhs.width_;
                precision_  = rhs.precision_;
                fill_      = rhs.fill_;
                locale_     = rhs.locale_;

                delete[] iarray_;
                iarray_size_ = rhs.iarray_size_;
                iarray_ = new long[iarray_size_];

                for (size_t i = 0; i < iarray_size_; ++i)
                    iarray_[i] = rhs.iarray_[i];

                delete[] parray_;
                parray_size_ = rhs.parray_size_;
                parray_ = new long[parray_size_];

                for (size_t i = 0; i < parray_size_; ++i)
                    parray_[i] = rhs.parray_[i];

                for (auto& callback: callbacks_)
                    callback.first(copyfmt_event, *this, index_);

                exceptions(rhs.exceptions());
            }

            char_type fill() const
            {
                return fill_;
            }

            char_type fill(char_type c)
            {
                char_type old;
                traits_type::assign(old, fill_);
                traits_type::assign(fill_, c);

                return old;
            }

            locale imbue(const locale& loc)
            {
                auto res = ios_base::imbue(loc);

                if (rdbuf_)
                    rdbuf_->pubimbue(loc);

                return res;
            }

            char narrow(char_type c, char def) const
            {
                return use_facet<ctype<char_type>>(locale_).narrow(c, def);
            }

            char_type widen(char c) const
            {
                return use_facet<ctype<char_type>>(locale_).widen(c);
            }

        protected:
            basic_ios()
            { /* DUMMY BODY */ }

            void init(basic_streambuf<Char, Traits>* sb)
            {
                // Initialized according to Table 128.
                rdbuf_ = sb;
                tie_ = nullptr;
                rdstate_ = sb ? goodbit : badbit;
                exceptions_ = goodbit;
                flags(skipws | dec);

                width(0);
                precision(6);

                fill_ = widen(' ');
                locale_ = locale();

                iarray_ = nullptr;
                parray_ = nullptr;
            }

            void move(basic_ios& rhs)
            {
                rdbuf_      = nullptr; // rhs keeps it!
                tie_        = rhs.tie_;
                exceptions_ = rhs.exceptions_;
                flags_      = rhs.flags_;
                width_      = rhs.width_;
                precision_  = rhs.precision_;
                fill_       = rhs.fill_;
                locale_     = move(rhs.locale_);
                rdstate_    = rhs.rdstate_;
                callbacks_  = move(rhs.callbacks_);

                delete[] iarray_;
                iarray_      = rhs.iarray_;
                iarray_size_ = rhs.iarray_size_;

                delete[] parray_;
                parray_      = rhs.parray_;
                parray_size_ = rhs.parray_size_;

                rhs.tie_ = nullptr;
                rhs.iarray_ = nullptr;
                rhs.parray_ = nullptr;
            }

            void move(basic_ios&& rhs)
            {
                rdbuf_      = nullptr; // rhs keeps it!
                tie_        = rhs.tie_;
                exceptions_ = rhs.exceptions_;
                flags_      = rhs.flags_;
                width_      = rhs.width_;
                precision_  = rhs.precision_;
                fill_       = rhs.fill_;
                locale_     = move(rhs.locale_);
                rdstate_    = rhs.rdstate_;
                callbacks_.swap(rhs.callbacks_);

                delete[] iarray_;
                iarray_      = rhs.iarray_;
                iarray_size_ = rhs.iarray_size_;

                delete[] parray_;
                parray_      = rhs.parray_;
                parray_size_ = rhs.parray_size_;

                rhs.tie_ = nullptr;
                rhs.iarray_ = nullptr;
                rhs.parray_ = nullptr;
            }

            void swap(basic_ios& rhs) noexcept
            {
                // Swap everything but rdbuf_.
                swap(tie_, rhs.tie_);
                swap(exceptions_, rhs.exceptions_);
                swap(flags_, rhs.flags_);
                swap(width_, rhs.width_);
                swap(precision_, rhs.precision_);
                swap(fill_, rhs.fill_);
                swap(locale_, rhs.locale_);
                swap(rdstate_, rhs.rdstate_);
                swap(callbacks_, rhs.callbacks_);
                swap(iarray_);
                swap(iarray_size_);
                swap(parray_);
                swap(parray_size_);
            }

            void set_rdbuf(basic_streambuf<Char, Traits>* sb)
            {
                // No call to clear is intentional.
                rdbuf_ = sb;
            }

        private:
            basic_streambuf<Char, Traits>* rdbuf_;
            basic_ostream<Char, Traits>* tie_;
            iostate rdstate_;
            iostate exceptions_;
            char_type fill_;
    };

    /**
     * 27.5.6, ios_base manipulators:
     */

    /**
     * 27.5.6.1, fmtflags manipulators:
     */

    ios_base& boolalpha(ios_base& str);
    ios_base& noboolalpha(ios_base& str);
    ios_base& showbase(ios_base& str);
    ios_base& noshowbase(ios_base& str);
    ios_base& showpoint(ios_base& str);
    ios_base& noshowpoint(ios_base& str);
    ios_base& showpos(ios_base& str);
    ios_base& noshowpos(ios_base& str);
    ios_base& skipws(ios_base& str);
    ios_base& noskipws(ios_base& str);
    ios_base& uppercase(ios_base& str);
    ios_base& nouppercase(ios_base& str);
    ios_base& unitbuf(ios_base& str);
    ios_base& nounitbuf(ios_base& str);

    /**
     * 27.5.6.2, adjustfield manipulators:
     */

    ios_base& internal(ios_base& str);
    ios_base& left(ios_base& str);
    ios_base& right(ios_base& str);

    /**
     * 27.5.6.3, basefield manupulators:
     */

    ios_base& dec(ios_base& str);
    ios_base& hex(ios_base& str);
    ios_base& oct(ios_base& str);

    /**
     * 27.5.6.4, floatfield manupulators:
     */

    ios_base& fixed(ios_base& str);
    ios_base& scientific(ios_base& str);
    ios_base& hexfloat(ios_base& str);
    ios_base& defaultfloat(ios_base& str);

    /**
     * 27.5.6.5, error reporting:
     */

    // TODO: implement
}

#endif
