/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
