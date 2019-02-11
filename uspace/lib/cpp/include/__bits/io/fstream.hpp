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

#ifndef LIBCPP_BITS_IO_FSTREAM
#define LIBCPP_BITS_IO_FSTREAM

#include <cassert>
#include <cstdio>
#include <ios>
#include <iosfwd>
#include <locale>
#include <streambuf>
#include <string>

namespace std
{
    /**
     * 27.9.1.1, class template basic_filebuf:
     */
    template<class Char, class Traits>
    class basic_filebuf: public basic_streambuf<Char, Traits>
    {
        public:
            using char_type   = Char;
            using traits_type = Traits;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            /**
             * 27.9.1.2, constructors/destructor:
             */

            basic_filebuf()
                : basic_streambuf<char_type, traits_type>{},
                  obuf_{nullptr}, ibuf_{nullptr}, mode_{}, file_{nullptr}
            { /* DUMMY BODY */ }

            basic_filebuf(const basic_filebuf&) = delete;

            basic_filebuf(basic_filebuf&& other)
                : mode_{other.mode_}
            {
                close();
                std::swap(obuf_, other.obuf_);
                std::swap(ibuf_, other.ibuf_);
                std::swap(file_, other.file_);

                basic_streambuf<char_type, traits_type>::swap(other);
            }

            virtual ~basic_filebuf()
            {
                // TODO: exception here caught and not rethrown
                close();
            }

            /**
             * 27.9.1.3: assign/swap:
             */

            basic_filebuf& operator=(const basic_filebuf&) = delete;

            basic_filebuf& operator=(basic_filebuf&& other)
            {
                close();
                swap(other);

                return *this;
            }

            void swap(basic_filebuf& rhs)
            {
                std::swap(mode_, rhs.mode_);
                std::swap(obuf_, rhs.obuf_);
                std::swap(ibuf_, rhs.ibuf_);

                basic_streambuf<char_type, traits_type>::swap(rhs);
            }

            /**
             * 27.9.1.4, members:
             */

            bool is_open() const
            {
                return file_ != nullptr;
            }

            basic_filebuf<char_type, traits_type>* open(const char* name, ios_base::openmode mode)
            {
                if (file_)
                    return nullptr;

                mode_ = mode;
                mode = (mode & (~ios_base::ate));
                const char* mode_str = get_mode_str_(mode);

                if (!mode_str)
                    return nullptr;

                file_ = fopen(name, mode_str);
                if (!file_)
                    return nullptr;

                if ((mode_ & ios_base::ate) != 0)
                {
                    if (fseek(file_, 0, SEEK_END) != 0)
                    {
                        close();
                        return nullptr;
                    }
                }

                if (!ibuf_ && mode_is_in_(mode_))
                    ibuf_ = new char_type[buf_size_];
                if (!obuf_ && mode_is_out_(mode_))
                    obuf_ = new char_type[buf_size_];
                init_();

                return this;
            }

            basic_filebuf<char_type, traits_type>* open(const string& name, ios_base::openmode mode)
            {
                return open(name.c_str(), mode);
            }

            basic_filebuf<char_type, traits_type>* close()
            {
                // TODO: caught exceptions are to be rethrown after closing the file
                if (!file_)
                    return nullptr;
                // TODO: deallocate buffers?

                if (this->output_next_ != this->output_begin_)
                    overflow(traits_type::eof());
                // TODO: unshift? (p. 1084 at the top)

                fclose(file_);
                file_ = nullptr;

                return this;
            }

        protected:
            /**
             * 27.9.1.5, overriden virtual functions:
             */

            int_type underflow() override
            {
                // TODO: use codecvt
                if (!mode_is_in_(mode_))
                    return traits_type::eof();

                if (!ibuf_)
                {
                    ibuf_ = new char_type[buf_size_];
                    this->input_begin_ = this->input_next_ = this->input_end_ = ibuf_;
                }

                off_type i{};
                if (this->input_next_ < this->input_end_)
                {
                    auto idx = static_cast<off_type>(this->input_next_ - this->input_begin_);
                    auto count = static_cast<off_type>(buf_size_) - idx;

                    for (; i < count; ++i, ++idx)
                        ibuf_[i] = ibuf_[idx];
                }

                for (; i < static_cast<off_type>(buf_size_); ++i)
                {
                    auto c = fgetc(file_);
                    if (c == traits_type::eof())
                        break;

                    ibuf_[i] = static_cast<char_type>(c);

                    if (ibuf_[i] == '\n')
                    {
                        ++i;
                        break;
                    }
                }

                this->input_next_ = this->input_begin_;
                this->input_end_ = this->input_begin_ + i;

                if (i == 0)
                    return traits_type::eof();

                return traits_type::to_int_type(*this->input_next_);
            }

            int_type pbackfail(int_type c = traits_type::eof()) override
            {
                auto cc = traits_type::to_char_type(c);
                if (!traits_type::eq_int_type(c, traits_type::eof()) &&
                    this->putback_avail_() &&
                    traits_type::eq(cc, this->gptr()[-1]))
                {
                    --this->input_next_;

                    return c;
                }
                else if (!traits_type::eq_int_type(c, traits_type::eof()) &&
                         this->putback_avail_() && (mode_ & ios_base::out) != 0)
                {
                    *--this->input_next_ = cc;

                    return c;
                }
                else if (traits_type::eq_int_type(c, traits_type::eof()) &&
                         this->putback_avail_())
                {
                    --this->input_next_;

                    return traits_type::not_eof(c);
                }

                return traits_type::eof();
            }

            int_type overflow(int_type c = traits_type::eof()) override
            {
                // TODO: use codecvt
                if (!mode_is_out_(mode_))
                    return traits_type::eof();

                auto count = static_cast<size_t>(this->output_next_ - this->output_begin_);
                fwrite(obuf_, sizeof(char_type), count, file_);
                fflush(file_);

                this->output_next_ = this->output_begin_;

                return traits_type::not_eof(c);
            }

            basic_streambuf<char_type, traits_type>*
            setbuf(char_type* s, streamsize n) override
            {
                // TODO: implement
                __unimplemented();
                return nullptr;
            }

            pos_type seekoff(off_type off, ios_base::seekdir dir,
                             ios_base::openmode mode = ios_base::in | ios_base::out) override
            {
                // TODO: implement
                __unimplemented();
                return pos_type{};
            }

            pos_type seekpos(pos_type pos,
                             ios_base::openmode mode = ios_base::in | ios_base::out) override
            {
                // TODO: implement
                __unimplemented();
                return pos_type{};
            }

            int sync() override
            {
                if (mode_is_out_(mode_))
                    overflow();

                return 0; // TODO: what is the correct return value?
            }

            void imbue(const locale& loc) override
            {
                // TODO: codecvt should be cached when we start using it
                basic_streambuf<char_type, traits_type>::imbue(loc);
            }

        private:
            char_type* obuf_;
            char_type* ibuf_;

            ios_base::openmode mode_;

            FILE* file_;

            static constexpr size_t buf_size_{128};

            const char* get_mode_str_(ios_base::openmode mode)
            {
                /**
                 * See table 132.
                 */
                switch (mode)
                {
                    case ios_base::out:
                    case ios_base::out | ios_base::trunc:
                        return "w";
                    case ios_base::out | ios_base::app:
                    case ios_base::app:
                        return "a";
                    case ios_base::in:
                        return "r";
                    case ios_base::in | ios_base::out:
                        return "r+";
                    case ios_base::in | ios_base::out | ios_base::trunc:
                        return "w+";
                    case ios_base::in | ios_base::out | ios_base::app:
                    case ios_base::in | ios_base::app:
                        return "a+";
                    case ios_base::binary | ios_base::out:
                    case ios_base::binary | ios_base::out | ios_base::trunc:
                        return "wb";
                    case ios_base::binary | ios_base::out | ios_base::app:
                    case ios_base::binary | ios_base::app:
                        return "ab";
                    case ios_base::binary | ios_base::in:
                        return "rb";
                    case ios_base::binary | ios_base::in | ios_base::out:
                        return "r+b";
                    case ios_base::binary | ios_base::in | ios_base::out | ios_base::trunc:
                        return "w+b";
                    case ios_base::binary | ios_base::in | ios_base::out | ios_base::app:
                    case ios_base::binary | ios_base::in | ios_base::app:
                        return "a+b";
                    default:
                        return nullptr;
                }
            }

            bool mode_is_in_(ios_base::openmode mode)
            {
                return (mode & ios_base::in) != 0;
            }

            bool mode_is_out_(ios_base::openmode mode)
            {
                return (mode & (ios_base::out | ios_base::app | ios_base::trunc)) != 0;
            }

            void init_()
            {
                if (ibuf_)
                    this->input_begin_ = this->input_next_ = this->input_end_ = ibuf_;

                if (obuf_)
                {
                    this->output_begin_ = this->output_next_ = obuf_;
                    this->output_end_ = obuf_ + buf_size_;
                }
            }
    };

    template<class Char, class Traits>
    void swap(basic_filebuf<Char, Traits>& lhs, basic_filebuf<Char, Traits>& rhs)
    {
        lhs.swap(rhs);
    }

    /**
     * 27.9.1.6, class template basic_ifstream:
     */

    template<class Char, class Traits>
    class basic_ifstream: public basic_istream<Char, Traits>
    {
        public:
            using char_type   = Char;
            using traits_type = Traits;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            /**
             * 27.9.1.7, constructors:
             */

            basic_ifstream()
                : basic_istream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            { /* DUMMY BODY */ }

            explicit basic_ifstream(const char* name, ios_base::openmode mode = ios_base::in)
                : basic_istream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            {
                if (!rdbuf_.open(name, mode | ios_base::in))
                    this->setstate(ios_base::failbit);
            }

            explicit basic_ifstream(const string& name, ios_base::openmode mode = ios_base::in)
                : basic_istream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            {
                if (!rdbuf_.open(name, mode | ios_base::in))
                    this->setstate(ios_base::failbit);
            }

            basic_ifstream(const basic_ifstream&) = delete;

            basic_ifstream(basic_ifstream&& other)
                : basic_istream<char_type, traits_type>{move(other)},
                  rdbuf_{other.rdbuf_}
            {
                basic_istream<char_type, traits_type>::set_rdbuf(&rdbuf_);
            }

            /**
             * 27.9.1.8, assign/swap:
             */

            basic_ifstream& operator=(const basic_ifstream&) = delete;

            basic_ifstream& operator=(basic_ifstream&& other)
            {
                swap(other);
            }

            void swap(basic_ifstream& rhs)
            {
                basic_istream<char_type, traits_type>::swap(rhs);
                rdbuf_.swap(rhs.rdbuf_);
            }

            /**
             * 27.9.1.9, members:
             */

            basic_filebuf<char_type, traits_type>* rdbuf() const
            {
                return const_cast<basic_filebuf<char_type, traits_type>*>(&rdbuf_);
            }

            bool is_open() const
            {
                return rdbuf_.is_open();
            }

            void open(const char* name, ios_base::openmode mode = ios_base::in)
            {
                if (!rdbuf_.open(name, mode | ios_base::in))
                    this->setstate(ios_base::failbit);
                else
                    this->clear();
            }

            void open(const string& name, ios_base::openmode mode = ios_base::in)
            {
                open(name.c_str(), mode);
            }

            void close()
            {
                if (!rdbuf_.close())
                    this->setstate(ios_base::failbit);
            }

        private:
            basic_filebuf<char_type, traits_type> rdbuf_;
    };

    template<class Char, class Traits>
    void swap(basic_ifstream<Char, Traits>& lhs,
              basic_ifstream<Char, Traits>& rhs)
    {
        lhs.swap(rhs);
    }

    /**
     * 27.9.1.10, class template basic_ofstream:
     */

    template<class Char, class Traits>
    class basic_ofstream: public basic_ostream<Char, Traits>
    {
        public:
            using char_type   = Char;
            using traits_type = Traits;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            /**
             * 27.9.1.11, constructors:
             */

            basic_ofstream()
                : basic_ostream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            { /* DUMMY BODY */ }

            explicit basic_ofstream(const char* name, ios_base::openmode mode = ios_base::out)
                : basic_ostream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            {
                if (!rdbuf_.open(name, mode | ios_base::out))
                    this->setstate(ios_base::failbit);
            }

            explicit basic_ofstream(const string& name, ios_base::openmode mode = ios_base::out)
                : basic_ostream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            {
                if (!rdbuf_.open(name, mode | ios_base::out))
                    this->setstate(ios_base::failbit);
            }

            basic_ofstream(const basic_ofstream&) = delete;

            basic_ofstream(basic_ofstream&& other)
                : basic_ostream<char_type, traits_type>{move(other)},
                  rdbuf_{other.rdbuf_}
            {
                basic_ostream<char_type, traits_type>::set_rdbuf(&rdbuf_);
            }

            /**
             * 27.9.1.12, assign/swap:
             */

            basic_ofstream& operator=(const basic_ofstream&) = delete;

            basic_ofstream& operator=(basic_ofstream&& other)
            {
                swap(other);
            }

            void swap(basic_ofstream& rhs)
            {
                basic_ostream<char_type, traits_type>::swap(rhs);
                rdbuf_.swap(rhs.rdbuf_);
            }

            /**
             * 27.9.1.13, members:
             */

            basic_filebuf<char_type, traits_type>* rdbuf() const
            {
                return const_cast<basic_filebuf<char_type, traits_type>*>(&rdbuf_);
            }

            bool is_open() const
            {
                return rdbuf_.is_open();
            }

            void open(const char* name, ios_base::openmode mode = ios_base::out)
            {
                if (!rdbuf_.open(name, mode | ios_base::out))
                    this->setstate(ios_base::failbit);
                else
                    this->clear();
            }

            void open(const string& name, ios_base::openmode mode = ios_base::out)
            {
                open(name.c_str(), mode);
            }

            void close()
            {
                if (!rdbuf_.close())
                    this->setstate(ios_base::failbit);
            }

        private:
            basic_filebuf<char_type, traits_type> rdbuf_;
    };

    template<class Char, class Traits>
    void swap(basic_ofstream<Char, Traits>& lhs,
              basic_ofstream<Char, Traits>& rhs)
    {
        lhs.swap(rhs);
    }

    /**
     * 27.9.1.14, class template basic_fstream:
     */

    template<class Char, class Traits>
    class basic_fstream: public basic_iostream<Char, Traits>
    {
        public:
            using char_type   = Char;
            using traits_type = Traits;
            using int_type    = typename traits_type::int_type;
            using pos_type    = typename traits_type::pos_type;
            using off_type    = typename traits_type::off_type;

            /**
             * 27.9.1.15, constructors:
             */

            basic_fstream()
                : basic_iostream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            { /* DUMMY BODY */ }

            explicit basic_fstream(const char* name,
                                   ios_base::openmode mode = ios_base::in | ios_base::out)
                : basic_iostream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            {
                if (!rdbuf_.open(name, mode))
                    this->setstate(ios_base::failbit);
            }

            explicit basic_fstream(const string& name,
                                   ios_base::openmode mode = ios_base::in | ios_base::out)
                : basic_iostream<char_type, traits_type>{&rdbuf_},
                  rdbuf_{}
            {
                if (!rdbuf_.open(name, mode))
                    this->setstate(ios_base::failbit);
            }

            basic_fstream(const basic_fstream&) = delete;

            basic_fstream(basic_fstream&& other)
                : basic_iostream<char_type, traits_type>{move(other)},
                  rdbuf_{other.rdbuf_}
            {
                basic_iostream<char_type, traits_type>::set_rdbuf(&rdbuf_);
            }

            /**
             * 27.9.1.16, assign/swap:
             */

            basic_fstream& operator=(const basic_fstream&) = delete;

            basic_fstream& operator=(basic_fstream&& other)
            {
                swap(other);
            }

            void swap(basic_fstream& rhs)
            {
                basic_iostream<char_type, traits_type>::swap(rhs);
                rdbuf_.swap(rhs.rdbuf_);
            }

            /**
             * 27.9.1.17, members:
             */

            basic_filebuf<char_type, traits_type>* rdbuf() const
            {
                return const_cast<basic_filebuf<char_type, traits_type>*>(&rdbuf_);
            }

            bool is_open() const
            {
                return rdbuf_.is_open();
            }

            void open(const char* name,
                      ios_base::openmode mode = ios_base::in | ios_base::out)
            {
                if (!rdbuf_.open(name, mode))
                    this->setstate(ios_base::failbit);
                else
                    this->clear();
            }

            void open(const string& name,
                      ios_base::openmode mode = ios_base::in | ios_base::out)
            {
                open(name.c_str(), mode);
            }

            void close()
            {
                if (!rdbuf_.close())
                    this->setstate(ios_base::failbit);
            }

        private:
            basic_filebuf<char_type, traits_type> rdbuf_;
    };

    template<class Char, class Traits>
    void swap(basic_fstream<Char, Traits>& lhs,
              basic_fstream<Char, Traits>& rhs)
    {
        lhs.swap(rhs);
    }
}

#endif
