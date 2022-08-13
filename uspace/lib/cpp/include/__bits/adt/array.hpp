/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ADT_ARRAY
#define LIBCPP_BITS_ADT_ARRAY

#include <iterator>
#include <utility>

namespace std
{
    /**
     * 23.3.2, class template array:
     */

    template<class T, size_t N>
    struct array
    {
        using value_type             = T;
        using reference              = T&;
        using const_reference        = const T&;
        using size_type              = size_t;
        using difference_type        = ptrdiff_t;
        using pointer                = T*;
        using const_pointer          = const T*;
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /**
         * Note: In the case of N == 0 the standard mandates that
         *       begin() == end() which is achieved by setting the size
         *       to 1. Return value of data() is unspecified and front()
         *       and back() cause undefined behavior.
         */
        value_type elems[N ? N : 1];

        void fill(const T& x)
        {
            fill_n(begin(), N, x);
        }

        void swap(array& other)
                // TODO: Does not find declval atm :/
                /* noexcept(noexcept(swap(declval<T&>(), declval<T&>()))) */
        {
            swap_ranges(begin(), end(), other.begin());
        }

        iterator begin() noexcept
        {
            return &elems[0];
        }

        iterator end() noexcept
        {
            return &elems[0] + N;
        }

        reverse_iterator rbegin() noexcept
        {
            return make_reverse_iterator(end());
        }

        reverse_iterator rend() noexcept
        {
            return make_reverse_iterator(begin());
        }

        const_iterator cbegin() const noexcept
        {
            return &elems[0];
        }

        const_iterator cend() const noexcept
        {
            return &elems[0] + N;
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return make_reverse_iterator(end());
        }

        const_reverse_iterator crend() const noexcept
        {
            return make_reverse_iterator(begin());
        }

        reference operator[](size_type idx)
        {
            return elems[idx];
        }

        constexpr const_reference operator[](size_type idx) const
        {
            return elems[idx];
        }

        reference at(size_type idx)
        {
            // TODO: Bounds checking.
            return elems[idx];
        }

        constexpr const_reference at(size_type idx) const
        {
            // TODO: Bounds checking.
            return elems[idx];
        }

        reference front()
        {
            return elems[0];
        }

        constexpr const_reference front() const
        {
            return elems[0];
        }

        reference back()
        {
            return elems[N - 1];
        }

        constexpr const_reference back() const
        {
            return elems[N - 1];
        }

        pointer data() noexcept
        {
            return &elems[0];
        }

        const_pointer data() const noexcept
        {
            return &elems[0];
        }

        size_type size() const noexcept
        {
            return N;
        }
    };

    template<class T, size_t N>
    void swap(array<T, N>& lhs, array<T, N>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    /**
     * 23.3.2.9, tuple interface for class template array:
     */

    template<class>
    struct tuple_size;

    template<class T, size_t N>
    struct tuple_size<array<T, N>>
        : integral_constant<size_t, N>
    { /* DUMMY BODY */ };

    template<size_t, class>
    struct tuple_element;

    template<size_t I, class T, size_t N>
    struct tuple_element<I, array<T, N>>
        : aux::type_is<T>
    { /* DUMMY BODY */ };

    template<size_t I, class T, size_t N>
    constexpr T& get(array<T, N>& arr) noexcept
    {
        static_assert(I < N, "index out of bounds");

        return arr[I];
    }

    template<size_t I, class T, size_t N>
    constexpr T&& get(array<T, N>&& arr) noexcept
    {
        return move(get<I>(arr));
    }

    template<size_t I, class T, size_t N>
    constexpr const T& get(const array<T, N>& arr) noexcept
    {
        static_assert(I < N, "index out of bounds");

        return arr[I];
    }
}

#endif
