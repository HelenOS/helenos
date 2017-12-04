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

#ifndef LIBCPP_INTERNAL_LOCALE_NUM_PUT
#define LIBCPP_INTERNAL_LOCALE_NUM_PUT

#include <internal/locale.hpp>
#include <internal/locale/numpunct.hpp>
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

            explicit num_put(size_t refs = 0);

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

            ~num_put();

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
                // TODO: implement
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, long long v) const
            {
                // TODO: implement
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, unsigned long v) const
            {
                // TODO: implement
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, unsigned long long v) const
            {
                // TODO: implement
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, double v) const
            {
                // TODO: implement
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, long double v) const
            {
                // TODO: implement
            }

            iter_type do_put(iter_type it, ios_base& base, char_type fill, const void* v) const
            {
                // TODO: implement
            }
    };
}

#endif
