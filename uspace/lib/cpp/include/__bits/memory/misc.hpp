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

#ifndef LIBCPP_BITS_MEMORY_MISC
#define LIBCPP_BITS_MEMORY_MISC

#include <cstdlib>
#include <iterator>
#include <new>
#include <utility>

namespace std
{
    /**
     * 20.7.10, raw storage iterator:
     */

    template<class OutputIterator, class T>
    class raw_storage_iterator: public iterator<output_iterator_tag, void, void, void, void>
    {
        public:
            explicit raw_storage_iterator(OutputIterator it)
                : it_{it}
            { /* DUMMY BODY */ }

            raw_storage_iterator& operator*()
            {
                return *this;
            }

            raw_storage_iterator& operator=(const T& element)
            {
                new(it_) T{element};

                return *this;
            }

            raw_storage_iterator& operator++()
            {
                ++it_;

                return *this;
            }

            raw_storage_iterator operator++(int)
            {
                return raw_storage_iterator{it_++};
            }

        private:
            OutputIterator it_;
    };

    /**
     * 20.7.11, temporary buffers:
     */

    template<class T>
    pair<T*, ptrdiff_t> get_temporary_buffer(ptrdiff_t n) noexcept
    {
        T* res{};

        while (n > 0)
        {
            res = (T*)malloc(n * sizeof(T));

            if (res)
                return make_pair(res, n);

            --n;
        }

        return make_pair(nullptr, ptrdiff_t{});
    }

    template<class T>
    void return_temporary_buffer(T* ptr)
    {
        free(ptr);
    }

    /**
     * 20.7.12, specialized algorithms:
     */

    template<class Iterator>
    struct iterator_traits;

    template<class InputIterator, class ForwardIterator>
    ForwardIterator unitialized_copy(
        InputIterator first, InputIterator last,
        ForwardIterator result
    )
    {
        for (; first != last; ++first, ++result)
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(*first);

        return result;
    }

    template<class InputIterator, class Size, class ForwardIterator>
    ForwardIterator unitialized_copy_n(
        InputIterator first, Size n, ForwardIterator result
    )
    {
        for (; n > 0; ++first, --n, ++result)
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(*first);

        return result;
    }

    template<class ForwardIterator, class T>
    void unitialized_fill(
        ForwardIterator first, ForwardIterator last, const T& x
    )
    {
        for (; first != last; ++first)
            ::new (static_cast<void*>(&*first)) typename iterator_traits<ForwardIterator>::value_type(x);
    }

    template<class ForwardIterator, class Size, class T>
    ForwardIterator unitialized_fill_n(
        ForwardIterator first, Size n, const T& x
    )
    {
        for (; n > 0; ++first, --n)
            ::new (static_cast<void*>(&*first)) typename iterator_traits<ForwardIterator>::value_type(x);

        return first;
    }
}

#endif
