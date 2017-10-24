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

#ifndef LIBCPP_ITERATOR
#define LIBCPP_ITERATOR

#include <cstdlib>

namespace std
{
    /**
     * 24.4.3, standard iterator tags:
     */

    struct input_iterator_tag
    { /* DUMMY BODY */ };

    struct output_iterator_tag
    { /* DUMMY BODY */ };

    struct forward_iterator_tag: input_iterator_tag
    { /* DUMMY BODY */ };

    struct bidirectional_iterator_tag: forward_iterator_tag
    { /* DUMMY BODY */ };

    struct random_access_iterator_tag: bidirectional_iterator_tag
    { /* DUMMY BODY */ };

    /**
     * 24.4.1, iterator traits:
     */

    template<class Iterator>
    struct iterator_traits
    {
        using difference_type   = typename Iterator::difference_type;
        using value_type        = typename Iterator::value_type;
        using iterator_category = typename Iterator::iterator_category;
        using reference         = typename Iterator::reference;
        using pointer           = typename Iterator::pointer;
    };

    template<class T>
    struct iterator_traits<T*>
    {
        using difference_type   = ptrdiff_t;
        using value_type        = T;
        using iterator_category = random_access_iterator_tag;
        using reference         = T&;
        using pointer           = T*;
    };

    template<class T>
    struct iterator_traits<const T*>
    {
        using difference_type   = ptrdiff_t;
        using value_type        = T;
        using iterator_category = random_access_iterator_tag;
        using reference         = const T&;
        using pointer           = const T*;
    };

    /**
     * 24.4.2, basic iterator:
     */

    template<
        class Category, class T, class Distance = ptrdiff_t,
        class Pointer = T*, class Reference = T&
    >
    struct iterator
    {
        using difference_type   = Distance;
        using value_type        = T;
        using iterator_category = Category;
        using reference         = Reference;
        using pointer           = Pointer;
    };

    /**
     * 25.5.1, reverse iterator:
     */

    template<class Iterator>
    class reverse_iterator
        : iterator<
            typename iterator_traits<Iterator>::iterator_category,
            typename iterator_traits<Iterator>::value_type,
            typename iterator_traits<Iterator>::difference_type,
            typename iterator_traits<Iterator>::pointer,
            typename iterator_traits<Iterator>::reference
          >
    {
        public:
            using iterator_type   = Iterator;
            using difference_type = typename iterator_traits<Iterator>::difference_type;
            using reference       = typename iterator_traits<Iterator>::reference;
            using pointer         = typename iterator_traits<Iterator>::pointer;

            reverse_iterator()
                : current_{}
            { /* DUMMY BODY */ }

            explicit reverse_iterator(Iterator it)
                : current_{it}
            { /* DUMMY BODY */ }

            template<class U>
            reverse_iterator(const reverse_iterator<U>& other)
                : current_{other.current_}
            { /* DUMMY BODY */ }

            template<class U>
            reverse_iterator& operator=(const reverse_iterator<U>& other)
            {
                current_ = other.base();

                return *this;
            }

            Iterator base() const
            {
                return current_;
            }

            reference operator*() const
            {
                auto tmp = current_;

                return *(--tmp);
            }

            pointer operator->() const
            {
                // TODO: need std::addressof
                return nullptr;
            }

            reverse_iterator& operator++()
            {
                --current_;

                return *this;
            }

            reverse_iterator operator++(int)
            {
                auto tmp = *this;
                --current_;

                return tmp;
            }

            reverse_iterator& operator--()
            {
                ++current_;

                return *this;
            }

            reverse_iterator operator--(int)
            {
                auto tmp = *this;
                ++current_;

                return tmp;
            }

            reverse_iterator operator+(difference_type n) const
            {
                return reverse_iterator{current_ - n};
            }

            reverse_iterator& operator+=(difference_type n)
            {
                current_ -= n;

                return *this;
            }

            reverse_iterator operator-(difference_type n) const
            {
                return reverse_iterator{current_ + n};
            }

            reverse_iterator& operator-=(difference_type n)
            {
                current_ += n;

                return *this;
            }

            // TODO: unspecified operator[difference_type] const;
            auto operator[](difference_type n) const
            {
                return current_[-n - 1];
            }

        protected:
            Iterator current_;
    };

    template<class Iterator1, class Iterator2>
    bool operator==(const reverse_iterator<Iterator1>& lhs,
                    const reverse_iterator<Iterator2>& rhs)
    {
        return lhs.base() == rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator<(const reverse_iterator<Iterator1>& lhs,
                   const reverse_iterator<Iterator2>& rhs)
    {
        // Remember: they are reversed!
        return lhs.base() > rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator!=(const reverse_iterator<Iterator1>& lhs,
                    const reverse_iterator<Iterator2>& rhs)
    {
        return lhs.base() != rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator>(const reverse_iterator<Iterator1>& lhs,
                    const reverse_iterator<Iterator2>& rhs)
    {
        return lhs.base() < rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator>=(const reverse_iterator<Iterator1>& lhs,
                    const reverse_iterator<Iterator2>& rhs)
    {
        return lhs.base() <= rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator<=(const reverse_iterator<Iterator1>& lhs,
                    const reverse_iterator<Iterator2>& rhs)
    {
        return lhs.base() >= rhs.base();
    }

    template<class Iterator1, class Iterator2>
    auto operator-(const reverse_iterator<Iterator1>& lhs,
                    const reverse_iterator<Iterator2>& rhs)
        -> decltype(rhs.base() - lhs.base())
    {
        return rhs.base() - lhs.base();
    }

    template<class Iterator>
    reverse_iterator<Iterator> operator+(
        typename reverse_iterator<Iterator>::difference_type n,
        const reverse_iterator<Iterator>& it
    )
    {
        return reverse_iterator<Iterator>{it.base() - n};
    }

    template<class Iterator>
    reverse_iterator<Iterator> make_reverse_iterator(Iterator it)
    {
        return reverse_iterator<Iterator>(it);
    }

    // TODO: other kind of iterator adaptors!
}

#endif
