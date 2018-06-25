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

#ifndef LIBCPP_BITS_LOCALE_NUM_PUT
#define LIBCPP_BITS_LOCALE_NUM_PUT

#include <__bits/locale/locale.hpp>
#include <__bits/locale/numpunct.hpp>
#include <ios>
#include <iterator>

namespace std
{
    /**
     * 22.4.2.2, class template num_put:
     */

    template<class Char, class OutputIterator = ostreambuf_iterator<Char>>
    class num_put: public locale::facet
    {
        public:
            using char_type = Char;
            using iter_type = OutputIterator;

            explicit num_put(size_t refs = 0)
            { /* DUMMY BODY */ }

            iter_type put(iter_type it, ios_base& base, char_type fill, bool v) const
            {
                return do_put(it, base, fill, v);
            }

            iter_type put(iter_type it, ios_base& base, char_type fill, long v) const
            {
                return do_put(it, base, fill, v);
            }

            iter_type put(iter_type it, ios_base& base, char_type fill, long long v) const
            {
                return do_put(it, base, fill, v);
            }

            iter_type put(iter_type it, ios_base& base, char_type fill, unsigned long v) const
            {
                return do_put(it, base, fill, v);
            }

            iter_type put(iter_type it, ios_base& base, char_type fill, unsigned long long v) const
            {
                return do_put(it, base, fill, v);
            }

            iter_type put(iter_type it, ios_base& base, char_type fill, double v) const
            {
                return do_put(it, base, fill, v);
            }

            iter_type put(iter_type it, ios_base& base, char_type fill, long double v) const
            {
                return do_put(it, base, fill, v);
            }

            iter_type put(iter_type it, ios_base& base, char_type fill, const void* v) const
            {
                return do_put(it, base, fill, v);
            }

            ~num_put()
            { /* DUMMY BODY */ }

        protected:
            iter_type do_put(iter_type it, ios_base& base, char_type fill, bool v) const
            {
                auto loc = base.getloc();

                if ((base.flags() & ios_base::boolalpha) == 0)
                    return do_put(it, base, fill, (long)v);
                else
                {
                    auto s = v ? use_facet<numpunct<char_type>>(loc).truename()
                               : use_facet<numpunct<char_type>>(loc).falsename();
                    for (auto c: s)
                        *it++ = c;
                }

                return it;
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, long v) const
            {
                auto basefield = (base.flags() & ios_base::basefield);
                auto uppercase = (base.flags() & ios_base::uppercase);

                int ret{};
                if (basefield == ios_base::oct)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lo", v);
                else if ((basefield == ios_base::hex) && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lx", v);
                else if (basefield == ios_base::hex)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lX", v);
                else
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%ld", v);

                return put_adjusted_buffer_(it, base, fill, ret);
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, long long v) const
            {
                auto basefield = (base.flags() & ios_base::basefield);
                auto uppercase = (base.flags() & ios_base::uppercase);

                int ret{};
                if (basefield == ios_base::oct)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%llo", v);
                else if ((basefield == ios_base::hex) && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%llx", v);
                else if (basefield == ios_base::hex)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%llX", v);
                else
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lld", v);

                return put_adjusted_buffer_(it, base, fill, ret);
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, unsigned long v) const
            {
                auto basefield = (base.flags() & ios_base::basefield);
                auto uppercase = (base.flags() & ios_base::uppercase);

                int ret{};
                if (basefield == ios_base::oct)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lo", v);
                else if ((basefield == ios_base::hex) && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lx", v);
                else if (basefield == ios_base::hex)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lX", v);
                else
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%lu", v);

                return put_adjusted_buffer_(it, base, fill, ret);
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, unsigned long long v) const
            {
                auto basefield = (base.flags() & ios_base::basefield);
                auto uppercase = (base.flags() & ios_base::uppercase);

                // TODO: showbase
                int ret{};
                if (basefield == ios_base::oct)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%llo", v);
                else if ((basefield == ios_base::hex) && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%llx", v);
                else if (basefield == ios_base::hex)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%llX", v);
                else
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%llu", v);

                return put_adjusted_buffer_(it, base, fill, ret);
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, double v) const
            {
                auto floatfield = (base.flags() & ios_base::floatfield);
                auto uppercase = (base.flags() & ios_base::uppercase);

                // TODO: showbase
                // TODO: precision
                int ret{};
                if (floatfield == ios_base::fixed)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%f", v);
                else if (floatfield == ios_base::scientific && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%e", v);
                else if (floatfield == ios_base::scientific)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%E", v);
                else if (floatfield == (ios_base::fixed | ios_base::scientific) && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%a", v);
                else if (floatfield == (ios_base::fixed | ios_base::scientific))
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%A", v);
                else if (!uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%g", v);
                else
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%G", v);

                return put_adjusted_buffer_(it, base, fill, ret);
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, long double v) const
            {
                /**
                 * Note: Long double is not support at the moment in snprintf.
                 */
                auto floatfield = (base.flags() & ios_base::floatfield);
                auto uppercase = (base.flags() & ios_base::uppercase);

                // TODO: showbase
                // TODO: precision
                int ret{};
                if (floatfield == ios_base::fixed)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%Lf", v);
                else if (floatfield == ios_base::scientific && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%Le", v);
                else if (floatfield == ios_base::scientific)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%LE", v);
                else if (floatfield == (ios_base::fixed | ios_base::scientific) && !uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%La", v);
                else if (floatfield == (ios_base::fixed | ios_base::scientific))
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%LA", v);
                else if (!uppercase)
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%Lg", v);
                else
                    ret = snprintf(base.buffer_, ios_base::buffer_size_, "%LG", v);

                return put_adjusted_buffer_(it, base, fill, ret);
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, const void* v) const
            {
                int ret = snprintf(base.buffer_, ios_base::buffer_size_, "%p", v);

                return put_adjusted_buffer_(it, base, fill, ret);
            }

        private:
            iter_type put_adjusted_buffer_(iter_type it, ios_base& base, char_type fill, size_t size) const
            {
                auto adjustfield = (base.flags() & ios_base::adjustfield);

                size_t to_fill{};
                size_t width = static_cast<size_t>(base.width());

                if (base.width() > 0 && size < width)
                    to_fill = width - size;

                if (to_fill > 0)
                {
                    if (adjustfield == ios_base::left)
                    {
                        it = put_buffer_(it, base, fill, 0, size);
                        for (size_t i = 0; i < to_fill; ++i)
                            *it++ = fill;
                    }
                    else if (adjustfield == ios_base::right)
                    {
                        for (size_t i = 0; i < to_fill; ++i)
                            *it++ = fill;
                        it = put_buffer_(it, base, fill, 0, size);
                    }
                    else if (adjustfield == ios_base::internal)
                    {
                        // TODO: pad after - or 0x/0X
                    }
                    else
                    {
                        for (size_t i = 0; i < to_fill; ++i)
                            *it++ = fill;
                        it = put_buffer_(it, base, fill, 0, size);
                    }
                }
                else
                    it = put_buffer_(it, base, fill, 0, size);
                base.width(0);

                return it;
            }

            iter_type put_buffer_(iter_type it, ios_base& base, char_type fill, size_t start, size_t size) const
            {
                const auto& loc = base.getloc();
                const auto& ct = use_facet<ctype<char_type>>(loc);
                const auto& punct = use_facet<numpunct<char_type>>(loc);

                for (size_t i = start; i < size; ++i)
                {
                    if (base.buffer_[i] == '.')
                        *it++ = punct.decimal_point();
                    else
                        *it++ = ct.widen(base.buffer_[i]);
                    // TODO: Should do grouping & thousands_sep, but that's a low
                    //       priority for now.
                }

                return it;
            }
    };
}

#endif
