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

#ifndef LIBCPP_BITS_NUMERIC
#define LIBCPP_BITS_NUMERIC

#include <utility>

namespace std
{

    /**
     * 26.7.2, accumulate:
     */

    template<class InputIterator, class T>
    T accumulate(InputIterator first, InputIterator last, T init)
    {
        auto acc{init};
        while (first != last)
            acc += *first++;

        return acc;
    }

    template<class InputIterator, class T, class BinaryOperation>
    T accumulate(InputIterator first, InputIterator last, T init,
                 BinaryOperation op)
    {
        auto acc{init};
        while (first != last)
            acc = op(acc, *first++);

        return acc;
    }

    /**
     * 26.7.3, inner product:
     */

    template<class InputIterator1, class InputIterator2, class T>
    T inner_product(InputIterator1 first1, InputIterator1 last1,
                    InputIterator2 first2, T init)
    {
        auto res{init};
        while (first1 != last1)
            res += (*first1++) * (*first2++);

        return res;
    }

    template<class InputIterator1, class InputIterator2, class T,
             class BinaryOperation1, class BinaryOperation2>
    T inner_product(InputIterator1 first1, InputIterator1 last1,
                    InputIterator2 first2, T init,
                    BinaryOperation1 op1, BinaryOperation2 op2)
    {
        auto res{init};
        while (first1 != last1)
            res = op1(res, op2(*first1++, *first2++));

        return res;
    }

    /**
     * 26.7.4, partial sum:
     */

    template<class InputIterator, class OutputIterator>
    OutputIterator partial_sum(InputIterator first, InputIterator last,
                               OutputIterator result)
    {
        if (first == last)
            return result;

        auto acc{*first++};
        *result++ = acc;

        while (first != last)
            *result++ = acc = acc + *first++;

        return result;
    }

    template<class InputIterator, class OutputIterator, class BinaryOperation>
    OutputIterator partial_sum(InputIterator first, InputIterator last,
                               OutputIterator result, BinaryOperation op)
    {
        if (first == last)
            return result;

        auto acc{*first++};
        *result++ = acc;

        while (first != last)
            *result++ = acc = op(acc, *first++);

        return result;
    }

    /**
     * 26.7.5, adjacent difference:
     */

    template<class InputIterator, class OutputIterator>
    OutputIterator adjacent_difference(InputIterator first, InputIterator last,
                                       OutputIterator result)
    {
        if (first == last)
            return result;

        auto acc{*first++};
        *result++ = acc;

        while (first != last)
        {
            auto val = *first++;
            *result++ = val - acc;
            acc = move(val);
        }

        return result;
    }

    template<class InputIterator, class OutputIterator, class BinaryOperation>
    OutputIterator adjacent_difference(InputIterator first, InputIterator last,
                                       OutputIterator result, BinaryOperation op)
    {
        if (first == last)
            return result;

        auto acc{*first++};
        *result++ = acc;

        while (first != last)
        {
            auto val = *first++;
            *result++ = op(val, acc);
            acc = move(val);
        }

        return result;
    }

    /**
     * 26.7.6, iota:
     */

    template<class ForwardIterator, class T>
    void iota(ForwardIterator first, ForwardIterator last, T value)
    {
        while (first != last)
            *first++ = value++;
    }
}

#endif
