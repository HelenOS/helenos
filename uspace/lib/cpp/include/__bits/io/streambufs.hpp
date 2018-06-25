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

#ifndef LIBCPP_BITS_IO_STREAMBUFS
#define LIBCPP_BITS_IO_STREAMBUFS

#include <iosfwd>
#include <cstdio>
#include <streambuf>

namespace std::aux
{
    template<class Char, class Traits = char_traits<Char>>
    class stdin_streambuf : public basic_streambuf<Char, Traits>
    {
        public:
            stdin_streambuf()
                : basic_streambuf<Char, Traits>{}, buffer_{nullptr}
            { /* DUMMY BODY */ }

            virtual ~stdin_streambuf()
            {
                if (buffer_)
                    delete[] buffer_;
            }

        protected:
            using traits_type = Traits;
            using char_type   = typename traits_type::char_type;
            using int_type    = typename traits_type::int_type;
            using off_type    = typename traits_type::off_type;

            using basic_streambuf<Char, Traits>::input_begin_;
            using basic_streambuf<Char, Traits>::input_next_;
            using basic_streambuf<Char, Traits>::input_end_;

            int_type underflow() override
            {
                if (!this->gptr())
                {
                    buffer_ = new char_type[buf_size_];
                    input_begin_ = input_next_ = input_end_ = buffer_;
                }

                off_type i{};
                if (input_next_ < input_end_)
                {
                    auto idx = static_cast<off_type>(input_next_ - input_begin_);
                    auto count = buf_size_ - idx;

                    for (; i < count; ++i, ++idx)
                        buffer_[i] = buffer_[idx];
                }

                for (; i < buf_size_; ++i)
                {
                    auto c = fgetc(in_);
                    putchar(c); // TODO: Temporary source of feedback.
                    if (c == traits_type::eof())
                        break;

                    buffer_[i] = static_cast<char_type>(c);

                    if (buffer_[i] == '\n')
                    {
                        ++i;
                        break;
                    }
                }

                input_next_ = input_begin_;
                input_end_ = input_begin_ + i;

                if (i == 0)
                    return traits_type::eof();

                return traits_type::to_int_type(*input_next_);
            }

            int_type uflow() override
            {
                auto res = underflow();
                ++input_next_;

                return res;
            }

            void imbue(const locale& loc)
            {
                this->locale_ = loc;
            }

        private:
            FILE* in_{stdin};

            char_type* buffer_;

            static constexpr off_type buf_size_{128};
    };

    template<class Char, class Traits = char_traits<Char>>
    class stdout_streambuf: public basic_streambuf<Char, Traits>
    {
        public:
            stdout_streambuf()
                : basic_streambuf<Char, Traits>{}
            { /* DUMMY BODY */ }

            virtual ~stdout_streambuf()
            { /* DUMMY BODY */ }

        protected:
            using traits_type = Traits;
            using char_type   = typename traits_type::char_type;
            using int_type    = typename traits_type::int_type;
            using off_type    = typename traits_type::off_type;

            int_type overflow(int_type c = traits_type::eof()) override
            {
                if (!traits_type::eq_int_type(c, traits_type::eof()))
                {
                    auto cc = traits_type::to_char_type(c);
                    fwrite(&cc, sizeof(char_type), 1, out_);
                }

                return traits_type::not_eof(c);
            }

            streamsize xsputn(const char_type* s, streamsize n) override
            {
                return fwrite(s, sizeof(char_type), n, out_);
            }

            int sync() override
            {
                if (fflush(out_))
                    return -1;
                return 0;
            }

        private:
            FILE* out_{stdout};
    };
}

#endif
