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

#ifndef LIBCPP_INTERNAL_LOCALE_NUM_GET
#define LIBCPP_INTERNAL_LOCALE_NUM_GET

#include <internal/locale.hpp>
#include <ios>
#include <iterator>

namespace std
{
    /**
     * 22.4.2.1, class template num_get:
     */

    template<class Char, class InputIterator = istreambuf_iterator<Char>>
    class num_get: public locale::facet
    {
        public:
            using char_type = Char;
            using iter_type = InputIterator;

            explicit num_get(size_t refs = 0);

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, bool& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, long& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, long long& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, unsigned short& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, unsigned int& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, unsigned long& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, unsigned long long& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, float& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, double& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, long double& v) const
            {
                return do_get(in, end, base, err, v);
            }

            iter_type get(iter_type in, iter_type end, ios_base& base,
                          ios_base::iostate& err, void*& v) const
            {
                return do_get(in, end, base, err, v);
            }

            static locale::id id;

            ~num_get();

        protected:
            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, bool& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, long& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, long long& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned short& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned int& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned long& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned long long& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, float& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, double& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, long double& v) const
            {
                // TODO: implement
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, void*& v) const
            {
                // TODO: implement
            }
    };
}

#endif
