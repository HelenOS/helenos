/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ADT_INITIALIZER_LIST
#define LIBCPP_BITS_ADT_INITIALIZER_LIST

#include <cstdlib>

namespace std
{
    /**
     * 18.9, initializer lists:
     */

    template<class T>
    class initializer_list
    {
        public:
            using value_type      = T;
            using const_reference = const T&;
            using reference       = const_reference;
            using size_type       = size_t;
            using const_iterator  = const T*;
            using iterator        = const_iterator;

            constexpr initializer_list() noexcept
                : begin_{nullptr}, size_{0}
            { /* DUMMY BODY */ }

            constexpr size_type size() const noexcept
            {
                return size_;
            }

            constexpr iterator begin() const noexcept
            {
                return begin_;
            }

            constexpr iterator end() const noexcept
            {
                return begin_ + size_;
            }

        private:
            iterator begin_;
            size_type size_;

            constexpr initializer_list(iterator begin, size_type size)
                : begin_{begin}, size_{size}
            { /* DUMMY BODY */ }
    };

    /**
     * 18.9.3, initializer list range access:
     */

    template<class T>
    constexpr auto begin(initializer_list<T> init)
    {
        return init.begin();
    }

    template<class T>
    constexpr auto end(initializer_list<T> init)
    {
        return init.end();
    }
}

#endif
