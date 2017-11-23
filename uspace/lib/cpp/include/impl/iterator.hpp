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
#include <memory>
#include <type_traits>

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
     * 24.5.1, reverse iterator:
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

    /**
     * 24.5.2, insert iterators:
     */

    /**
     * 24.5.2.1, back insert iterator:
     */

    template<class Container>
    class back_insert_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        public:
            using container_type = Container;

            explicit back_insert_iterator(Container& cont)
                : container{std::addressof(cont)}
            { /* DUMMY BODY */ }

            back_insert_iterator& operator=(const typename container_type::value_type& value)
            {
                container->push_back(value);
                return *this;
            }

            back_insert_iterator& operator=(typename container_type::value_type&& value)
            {
                container->push_back(move(value));
                return *this;
            }

            back_insert_iterator& operator*()
            {
                return *this;
            }

            back_insert_iterator& operator++()
            {
                return *this;
            }

            back_insert_iterator operator++(int)
            {
                return *this;
            }

        protected:
            Container* container;
    };

    template<class Container>
    back_insert_iterator<Container> back_inserter(Container& cont)
    {
        return back_insert_iterator<Container>(cont);
    }

    /**
     * 24.5.2.3, front insert iterator:
     */

    template<class Container>
    class front_insert_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        public:
            using container_type = Container;

            explicit front_insert_iterator(Container& cont)
                : container{std::addressof(cont)}
            { /* DUMMY BODY */ }

            front_insert_iterator& operator=(const typename container_type::value_type& value)
            {
                container->push_front(value);
                return *this;
            }

            front_insert_iterator& operator=(typename container_type::value_type&& value)
            {
                container->push_front(move(value));
                return *this;
            }

            front_insert_iterator& operator*()
            {
                return *this;
            }

            front_insert_iterator& operator++()
            {
                return *this;
            }

            front_insert_iterator operator++(int)
            {
                return *this;
            }

        protected:
            Container* container;
    };

    template<class Container>
    front_insert_iterator<Container> front_inserter(Container& cont)
    {
        return front_insert_iterator<Container>(cont);
    }

    /**
     * 24.5.2.5, front insert iterator:
     */

    template<class Container>
    class insert_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        public:
            using container_type = Container;

            explicit insert_iterator(Container& cont, typename Container::iterator i)
                : container{std::addressof(cont)}, iter{i}
            { /* DUMMY BODY */ }

            insert_iterator& operator=(const typename container_type::value_type& value)
            {
                iter = container.insert(iter, value);
                ++iter;

                return *this;
            }

            insert_iterator& operator=(typename container_type::value_type&& value)
            {
                iter = container.insert(iter, move(value));
                ++iter;

                return *this;
            }

            insert_iterator& operator*()
            {
                return *this;
            }

            insert_iterator& operator++()
            {
                return *this;
            }

            insert_iterator operator++(int)
            {
                return *this;
            }

        protected:
            Container* container;
            typename Container::iterator iter;
    };

    template<class Container>
    insert_iterator<Container> inserter(Container& cont, typename Container::iterator i)
    {
        return insert_iterator<Container>(cont, i);
    }

    /**
     * 24.5.3.1, move iterator:
     */

    namespace aux
    {
        template<class Iterator, class = void>
        struct move_it_get_reference
        {
            using type = typename iterator_traits<Iterator>::reference;
        };

        template<class Iterator>
        struct move_it_get_reference<
            Iterator, enable_if_t<
                is_reference<typename iterator_traits<Iterator>::reference>::value,
                void
            >
        >
        {
            using type = remove_reference_t<typename iterator_traits<Iterator>::reference>;
        };
    }

    template<class Iterator>
    class move_iterator
    {
        public:
            using iterator_type     = Iterator;
            using pointer           = iterator_type;
            using difference_type   = typename iterator_traits<iterator_type>::difference_type;
            using value_type        = typename iterator_traits<iterator_type>::value_type;
            using iterator_category = typename iterator_traits<iterator_type>::iterator_category;
            using reference         = typename aux::move_it_get_reference<iterator_type>::type;

            move_iterator()
                : current_{}
            { /* DUMMY BODY */ }

            explicit move_iterator(iterator_type i)
                : current_{i}
            { /* DUMMY BODY */ }

            // TODO: both require is_convertible
            template<class U>
            move_iterator(const move_iterator<U>& other)
                : current_{other.current_}
            { /* DUMMY BODY */ }

            template<class U>
            move_iterator& operator=(const move_iterator<U>& other)
            {
                current_ = other.current_;

                return *this;
            }

            iterator_type base() const
            {
                return current_;
            }

            reference operator*() const
            {
                return static_cast<reference>(*current_);
            }

            pointer operator->() const
            {
                return current_;
            }

            move_iterator& operator++()
            {
                ++current_;

                return *this;
            }

            move_iterator operator++(int)
            {
                auto tmp = *this;
                ++current_;

                return tmp;
            }

            move_iterator& operator--()
            {
                --current_;

                return *this;
            }

            move_iterator operator--(int)
            {
                auto tmp = *this;
                --current_;

                return tmp;
            }

            move_iterator operator+(difference_type n) const
            {
                return move_iterator(current_ + n);
            }

            move_iterator& operator+=(difference_type n)
            {
                current_ += n;

                return *this;
            }

            move_iterator operator-(difference_type n) const
            {
                return move_iterator(current_ - n);
            }

            move_iterator& operator-=(difference_type n)
            {
                current_ -= n;

                return *this;
            }

            auto operator[](difference_type idx) const
            {
                return move(current_[idx]);
            }

        private:
            iterator_type current_;
    };

    template<class Iterator1, class Iterator2>
    bool operator==(const move_iterator<Iterator1>& lhs,
                    const move_iterator<Iterator2>& rhs)
    {
        return lhs.base() == rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator!=(const move_iterator<Iterator1>& lhs,
                    const move_iterator<Iterator2>& rhs)
    {
        return lhs.base() != rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator<(const move_iterator<Iterator1>& lhs,
                   const move_iterator<Iterator2>& rhs)
    {
        return lhs.base() < rhs.base();
    }

    template<class Iterator1, class Iterator2>
    bool operator<=(const move_iterator<Iterator1>& lhs,
                    const move_iterator<Iterator2>& rhs)
    {
        return !(rhs < lhs);
    }

    template<class Iterator1, class Iterator2>
    bool operator>(const move_iterator<Iterator1>& lhs,
                   const move_iterator<Iterator2>& rhs)
    {
        return rhs < lhs;
    }

    template<class Iterator1, class Iterator2>
    bool operator>=(const move_iterator<Iterator1>& lhs,
                    const move_iterator<Iterator2>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class Iterator1, class Iterator2>
    auto operator-(const move_iterator<Iterator1>& lhs,
                   const move_iterator<Iterator2>& rhs)
        -> decltype(rhs.base() - lhs.base())
    {
        return lhs.base() - rhs.base();
    }

    template<class Iterator>
    move_iterator<Iterator> operator+(
        typename move_iterator<Iterator>::difference_type n,
        const move_iterator<Iterator>& it
    )
    {
        return it + n;
    }

    template<class Iterator>
    move_iterator<Iterator> make_move_iterator(Iterator it)
    {
        return move_iterator<Iterator>(it);
    }
}

#endif
