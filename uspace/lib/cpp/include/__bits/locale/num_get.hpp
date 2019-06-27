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

#ifndef LIBCPP_BITS_LOCALE_NUM_GET
#define LIBCPP_BITS_LOCALE_NUM_GET

#include <__bits/locale/locale.hpp>
#include <__bits/locale/numpunct.hpp>
#include <cerrno>
#include <cstring>
#include <ios>
#include <iterator>
#include <limits>

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

            explicit num_get(size_t refs = 0)
            { /* DUMMY BODY */ }

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

            ~num_get()
            { /* DUMMY BODY */ }

        protected:
            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, bool& v) const
            {
                if (in == end)
                {
                    err = ios_base::eofbit;
                    return in;
                }

                if ((base.flags() & ios_base::boolalpha) == 0)
                {
                    int8_t tmp{};
                    in = get_integral_<int64_t>(in, end, base, err, tmp);

                    if (tmp == 0)
                        v = false;
                    else if (tmp == 1)
                        v = true;
                    else
                    {
                        v = true;
                        err = ios_base::failbit;
                    }
                }
                else
                {
                    /**
                     * We track both truename() and falsename()
                     * at the same time, once either is matched
                     * or the input is too big without a match
                     * or the input ends the conversion ends
                     * and the result is deduced.
                     */

                    auto loc = base.getloc();
                    const auto& nt = use_facet<numpunct<char_type>>(loc);

                    auto true_target = nt.truename();
                    auto false_target = nt.falsename();

                    if (true_target == "" || false_target == "")
                    {
                        v = (err == ios_base::failbit);
                        return in;
                    }

                    auto true_str = true_target;
                    auto false_str = false_target;

                    size_t i{};
                    while (true)
                    {
                        if (true_str.size() <= i && false_str.size() <= i)
                            break;
                        auto c = *in++;

                        if (i < true_str.size())
                            true_str[i] = c;
                        if (i < false_str.size())
                            false_str[i] = c;
                        ++i;

                        if (in == end || *in == '\n')
                        {
                            err = ios_base::eofbit;
                            break;
                        }
                    }

                    if (i == true_str.size() && true_str == true_target)
                    {
                        v = true;

                        if (++in == end)
                            err = ios_base::eofbit;
                        else
                            err = ios_base::goodbit;
                    }
                    else if (i == false_str.size() && false_str == false_target)
                    {
                        v = false;

                        if (in == end)
                            err = (ios_base::failbit | ios_base::eofbit);
                        else
                            err = ios_base::failbit;
                    }
                    else
                        err = ios_base::failbit;
                }

                return in;
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, long& v) const
            {
                return get_integral_<int64_t>(in, end, base, err, v);
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, long long& v) const
            {
                return get_integral_<int64_t>(in, end, base, err, v);
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned short& v) const
            {
                return get_integral_<uint64_t>(in, end, base, err, v);
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned int& v) const
            {
                return get_integral_<uint64_t>(in, end, base, err, v);
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned long& v) const
            {
                return get_integral_<uint64_t>(in, end, base, err, v);
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, unsigned long long& v) const
            {
                return get_integral_<uint64_t>(in, end, base, err, v);
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, float& v) const
            {
                // TODO: implement
                return in;
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, double& v) const
            {
                // TODO: implement
                return in;
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, long double& v) const
            {
                // TODO: implement
                return in;
            }

            iter_type do_get(iter_type in, iter_type end, ios_base& base,
                             ios_base::iostate& err, void*& v) const
            {
                // TODO: implement
                return in;
            }

        private:
            template<class BaseType, class T>
            iter_type get_integral_(iter_type in, iter_type end, ios_base& base,
                                    ios_base::iostate& err, T& v) const
            {
                BaseType res{};
                unsigned int num_base{10};

                auto basefield = (base.flags() & ios_base::basefield);
                if (basefield == ios_base::oct)
                    num_base = 8;
                else if (basefield == ios_base::hex)
                    num_base = 16;

                auto size = fill_buffer_integral_(in, end, base);
                if (size > 0)
                {
                    int olderrno{errno};
                    errno = EOK;
                    char *endptr = NULL;

                    if constexpr (is_signed<BaseType>::value)
                        res = ::strtoll(base.buffer_, &endptr, num_base);
                    else
                        res = ::strtoull(base.buffer_, &endptr, num_base);

                    if (errno != EOK || endptr == base.buffer_)
                        err |= ios_base::failbit;

                    errno = olderrno;

                    if (res > static_cast<BaseType>(numeric_limits<T>::max()))
                    {
                        err |= ios_base::failbit;
                        v = numeric_limits<T>::max();
                    }
                    else if (res < static_cast<BaseType>(numeric_limits<T>::min()))
                    {
                        err |= ios_base::failbit;
                        v = numeric_limits<T>::min();
                    }
                    else
                        v = static_cast<T>(res);
                }
                else
                {
                    err |= ios_base::failbit;
                    v = 0;
                }

                return in;
            }

            size_t fill_buffer_integral_(iter_type& in, iter_type end, ios_base& base) const
            {
                if (in == end)
                    return 0;

                auto loc = base.getloc();
                const auto& ct = use_facet<ctype<char_type>>(loc);
                auto hex = ((base.flags() & ios_base::hex) != 0);

                size_t i{};
                if (*in == '+' || *in == '-')
                    base.buffer_[i++] = *in++;

                while (in != end && i < ios_base::buffer_size_ - 1)
                {
                    auto c = *in;
                    if (ct.is(ctype_base::digit, c) || (hex &&
                       ((c >= ct.widen('A') && c <= ct.widen('F')) ||
                        (c >= ct.widen('a') && c <= ct.widen('f')))))
                    {
                        ++in;
                        base.buffer_[i++] = c;
                    }
                    else
                        break;
                }
                base.buffer_[i] = char_type{};

                return i;
            }
    };
}

#endif
