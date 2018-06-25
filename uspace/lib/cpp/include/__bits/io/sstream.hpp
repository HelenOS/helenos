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

#ifndef LIBCPP_BITS_IO_SSTREAM
#define LIBCPP_BITS_IO_SSTREAM

#include <ios>
#include <iosfwd>
#include <iostream>
#include <streambuf>
#include <string>

namespace std
{
    /**
     * 27.8.2, class template basic_stringbuf:
     */

    template<class Char, class Traits, class Allocator>
    class basic_stringbuf: public basic_streambuf<Char, Traits>
    {
        public:
            using char_type      = Char;
            using traits_type    = Traits;
            using allocator_type = Allocator;
            using int_type       = typename traits_type::int_type;
            using pos_type       = typename traits_type::pos_type;
            using off_type       = typename traits_type::off_type;

            /**
             * 27.8.2.1, constructors:
             */

            explicit basic_stringbuf(ios_base::openmode mode = ios_base::in | ios_base::out)
                : basic_streambuf<char_type, traits_type>(), mode_{mode}, str_{}
            {
                init_();
            }

            explicit basic_stringbuf(const basic_string<char_type, traits_type, allocator_type> str,
                                     ios_base::openmode mode = ios_base::in | ios_base::out)
                : basic_streambuf<char_type, traits_type>(), mode_{mode}, str_{str}
            {
                init_();
            }

            basic_stringbuf(const basic_stringbuf&) = delete;

            basic_stringbuf(basic_stringbuf&& other)
                : mode_{move(other.mode_)}, str_{move(other.str_)}
            {
                basic_streambuf<char_type, traits_type>::swap(other);
            }

            /**
             * 27.8.2.2, assign and swap:
             */

            basic_stringbuf& operator=(const basic_stringbuf&) = delete;

            basic_stringbuf& operator=(basic_stringbuf&& other)
            {
                swap(other);
            }

            void swap(basic_stringbuf& rhs)
            {
                std::swap(mode_, rhs.mode_);
                std::swap(str_, rhs.str_);

                basic_streambuf<char_type, traits_type>::swap(rhs);
            }

            /**
             * 27.8.2.3, get and set:
             */

            basic_string<char_type, traits_type, allocator_type> str() const
            {
                if (mode_ & ios_base::out)
                    return basic_string<char_type, traits_type, allocator_type>{
                        this->output_begin_, this->output_next_ - 1, str_.get_allocator()
                    };
                else if (mode_ == ios_base::in)
                    return basic_string<char_type, traits_type, allocator_type>{
                        this->eback(), this->egptr(), str_.get_allocator()
                    };
                else
                    return basic_string<char_type, traits_type, allocator_type>{};
            }

            void str(const basic_string<char_type, traits_type, allocator_type>& str)
            {
                str_ = str;
                init_();
            }

        protected:
            /**
             * 27.8.2.4, overriden virtual functions:
             */

            int_type underflow() override
            {
                if (this->read_avail_())
                    return traits_type::to_int_type(*this->gptr());
                else
                    return traits_type::eof();
            }

            int_type pbackfail(int_type c = traits_type::eof()) override
            {
                if (!traits_type::eq_int_type(c, traits_type::eof()) &&
                    this->putback_avail_() &&
                    traits_type::eq(traits_type::to_char_type(c), this->gptr()[-1]))
                {
                    --this->input_next_;

                    return c;
                }
                else if (!traits_type::eq_int_type(c, traits_type::eof()) &&
                         this->putback_avail_() && (mode_ & ios_base::out) != 0)
                {
                    *--this->input_next_ = c;

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
                if ((mode_ & ios_base::out) == 0)
                    return traits_type::eof();

                if (!traits_type::eq_int_type(c, traits_type::eof()))
                {
                    if (this->output_next_ < this->output_end_)
                        return c;

                    auto size = static_cast<size_t>(this->output_next_ - this->output_begin_);
                    str_.size_ = size;

                    str_.push_back(traits_type::to_char_type(c));
                    init_();

                    return c;
                }
                else if (traits_type::eq_int_type(c, traits_type::eof()))
                    return traits_type::not_eof(c);

                return traits_type::eof();
            }

            basic_streambuf<char_type, traits_type>* setbuf(char_type* str, streamsize n) override
            {
                if (!str && n == 0)
                    return this;

                str_.assign(str, str + static_cast<size_t>(n));

                return this;
            }

            pos_type seekoff(off_type off, ios_base::seekdir dir,
                             ios_base::openmode mode = ios_base::in | ios_base::out)
            {
                if ((mode & ios_base::in) == ios_base::in)
                {
                    if (!this->input_next_)
                        return pos_type(off_type(-1));

                    return seekoff_(off, this->input_begin_, this->input_next_, this->input_end_, dir);
                }
                else if ((mode & ios_base::out) == ios_base::out)
                {
                    if (!this->output_next_)
                        return pos_type(off_type(-1));

                    return seekoff_(off, this->output_begin_, this->output_next_, this->output_end_, dir);
                }
                else if ((mode & (ios_base::in | ios_base::out)) == (ios_base::in | ios_base::out) &&
                          (dir == ios_base::beg || dir == ios_base::end))
                {
                    if (!this->input_next_ || !this->output_next_)
                        return pos_type(off_type(-1));

                    seekoff_(off, this->input_begin_, this->input_next_, this->input_end_, dir);
                    return seekoff_(off, this->output_begin_, this->output_next_, this->output_end_, dir);
                }

                return pos_type(off_type(-1));
            }

            pos_type seekpos(pos_type pos, ios_base::openmode mode = ios_base::in | ios_base::out)
            {
                return seekoff(off_type(pos), ios_base::beg, mode);
            }

        private:
            ios_base::openmode mode_;
            basic_string<char_type, traits_type, allocator_type> str_;

            void init_()
            {
                if ((mode_ & ios_base::in) != 0)
                {
                    this->input_begin_ = this->input_next_ = str_.begin();
                    this->input_end_ = str_.end();
                }

                if ((mode_ & ios_base::out) != 0)
                {
                    this->output_begin_ = str_.begin();
                    this->output_next_ = str_.end();
                    this->output_end_ = str_.begin() + str_.size() + 1;
                }
            }

            bool ensure_free_space_(size_t n = 1)
            {
                str_.ensure_free_space_(n);
                this->output_end_ = str_.begin() + str_.capacity();

                return true;
            }

            pos_type seekoff_(off_type off, char_type* begin, char_type*& next, char_type* end,
                          ios_base::seekdir dir)
            {
                off_type new_off{};

                if (dir == ios_base::beg)
                    new_off = 0;
                else if (dir == ios_base::cur)
                    new_off = static_cast<off_type>(next - begin);
                else if (dir == ios_base::end)
                    new_off = static_cast<off_type>(end - begin);

                if (new_off + off < 0 || begin + new_off + off >= end)
                    return pos_type(off_type(-1));
                else
                    next = begin + new_off + off;

                return pos_type(new_off);
            }
    };

    template<class Char, class Traits, class Allocator>
    void swap(basic_stringbuf<Char, Traits, Allocator>& lhs,
              basic_stringbuf<Char, Traits, Allocator>& rhs)
    {
        lhs.swap(rhs);
    }

    /**
     * 27.8.3, class template basic_istringstream:
     */

    template<class Char, class Traits, class Allocator>
    class basic_istringstream: public basic_istream<Char, Traits>
    {
        public:
            using char_type      = Char;
            using traits_type    = Traits;
            using allocator_type = Allocator;
            using int_type       = typename traits_type::int_type;
            using pos_type       = typename traits_type::pos_type;
            using off_type       = typename traits_type::off_type;

            /**
             * 27.8.3.1, constructors:
             */

            explicit basic_istringstream(ios_base::openmode mode = ios_base::in)
                : basic_istream<char_type, traits_type>(&sb_),
                  sb_(mode | ios_base::in)
            { /* DUMMY BODY */ }

            explicit basic_istringstream(const basic_string<char_type, traits_type, allocator_type> str,
                                         ios_base::openmode mode = ios_base::in)
                : basic_istream<char_type, traits_type>{&sb_},
                  sb_(str, mode | ios_base::in)
            { /* DUMMY BODY */ }

            basic_istringstream(const basic_istringstream&) = delete;

            basic_istringstream(basic_istringstream&& other)
                : basic_istream<char_type, traits_type>{move(other)},
                  sb_{move(other.sb_)}
            {
                basic_istream<char_type, traits_type>::set_rdbuf(&sb_);
            }

            /**
             * 27.8.3.2, assign and swap:
             */

            basic_istringstream& operator=(const basic_istringstream&) = delete;

            basic_istringstream& operator=(basic_istringstream&& other)
            {
                swap(other);
            }

            void swap(basic_istringstream& rhs)
            {
                basic_istream<char_type, traits_type>::swap(rhs);
                sb_.swap(rhs.sb_);
            }

            /**
             * 27.8.3.3, members:
             */

            basic_stringbuf<char_type, traits_type, allocator_type>* rdbuf() const
            {
                return const_cast<basic_stringbuf<char_type, traits_type, allocator_type>*>(&sb_);
            }

            basic_string<char_type, traits_type, allocator_type> str() const
            {
                return sb_.str();
            }

            void str(const basic_string<char_type, traits_type, allocator_type>& str)
            {
                sb_.str(str);
            }

        private:
            basic_stringbuf<char_type, traits_type, allocator_type> sb_;
    };

    template<class Char, class Traits, class Allocator>
    void swap(basic_istringstream<Char, Traits, Allocator>& lhs,
              basic_istringstream<Char, Traits, Allocator>& rhs)
    {
        lhs.swap(rhs);
    }

    template<class Char, class Traits, class Allocator>
    class basic_ostringstream: public basic_ostream<Char, Traits>
    {
        public:
            using char_type      = Char;
            using traits_type    = Traits;
            using allocator_type = Allocator;
            using int_type       = typename traits_type::int_type;
            using pos_type       = typename traits_type::pos_type;
            using off_type       = typename traits_type::off_type;

            /**
             * 27.8.4.1, constructors:
             */

            explicit basic_ostringstream(ios_base::openmode mode = ios_base::out)
                : basic_ostream<char_type, traits_type>(&sb_),
                  sb_(mode | ios_base::out)
            { /* DUMMY BODY */ }

            explicit basic_ostringstream(const basic_string<char_type, traits_type, allocator_type> str,
                                         ios_base::openmode mode = ios_base::out)
                : basic_ostream<char_type, traits_type>{&sb_},
                  sb_(str, mode | ios_base::out)
            { /* DUMMY BODY */ }

            basic_ostringstream(const basic_ostringstream&) = delete;

            basic_ostringstream(basic_ostringstream&& other)
                : basic_ostream<char_type, traits_type>{move(other)},
                  sb_{move(other.sb_)}
            {
                basic_ostream<char_type, traits_type>::set_rdbuf(&sb_);
            }

            /**
             * 27.8.4.2, assign and swap:
             */

            basic_ostringstream& operator=(const basic_ostringstream&) = delete;

            basic_ostringstream& operator=(basic_ostringstream&& other)
            {
                swap(other);
            }

            void swap(basic_ostringstream& rhs)
            {
                basic_ostream<char_type, traits_type>::swap(rhs);
                sb_.swap(rhs.sb_);
            }

            /**
             * 27.8.3.3, members:
             */

            basic_stringbuf<char_type, traits_type, allocator_type>* rdbuf() const
            {
                return const_cast<basic_stringbuf<char_type, traits_type, allocator_type>*>(&sb_);
            }

            basic_string<char_type, traits_type, allocator_type> str() const
            {
                return sb_.str();
            }

            void str(const basic_string<char_type, traits_type, allocator_type>& str)
            {
                sb_.str(str);
            }

        private:
            basic_stringbuf<char_type, traits_type, allocator_type> sb_;
    };

    /**
     * 27.8.5, class template basic_stringstream:
     */

    template<class Char, class Traits, class Allocator>
    class basic_stringstream: public basic_iostream<Char, Traits>
    {
        public:
            using char_type      = Char;
            using traits_type    = Traits;
            using allocator_type = Allocator;
            using int_type       = typename traits_type::int_type;
            using pos_type       = typename traits_type::pos_type;
            using off_type       = typename traits_type::off_type;

            /**
             * 27.8.5.1, constructors:
             */

            explicit basic_stringstream(ios_base::openmode mode = ios_base::in | ios_base::out)
                : basic_iostream<char_type, traits_type>(&sb_),
                  sb_(mode | ios_base::out)
            { /* DUMMY BODY */ }

            explicit basic_stringstream(const basic_string<char_type, traits_type, allocator_type> str,
                                         ios_base::openmode mode = ios_base::in | ios_base::out)
                : basic_iostream<char_type, traits_type>{&sb_},
                  sb_(str, mode | ios_base::out)
            { /* DUMMY BODY */ }

            basic_stringstream(const basic_stringstream&) = delete;

            basic_stringstream(basic_stringstream&& other)
                : basic_iostream<char_type, traits_type>{move(other)},
                  sb_{move(other.sb_)}
            {
                basic_iostream<char_type, traits_type>::set_rdbuf(&sb_);
            }

            /**
             * 27.8.5.2, assign and swap:
             */

            basic_stringstream& operator=(const basic_stringstream&) = delete;

            basic_stringstream& operator=(basic_stringstream&& other)
            {
                swap(other);
            }

            void swap(basic_stringstream& rhs)
            {
                basic_iostream<char_type, traits_type>::swap(rhs);
                sb_.swap(rhs.sb_);
            }

            /**
             * 27.8.5.3, members:
             */

            basic_stringbuf<char_type, traits_type, allocator_type>* rdbuf() const
            {
                return const_cast<basic_stringbuf<char_type, traits_type, allocator_type>*>(&sb_);
            }

            basic_string<char_type, traits_type, allocator_type> str() const
            {
                return sb_.str();
            }

            void str(const basic_string<char_type, traits_type, allocator_type>& str)
            {
                sb_.str(str);
            }

        private:
            basic_stringbuf<char_type, traits_type, allocator_type> sb_;
    };
}

#endif
