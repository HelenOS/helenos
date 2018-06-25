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

#ifndef LIBCPP_BITS_ITERATOR
#define LIBCPP_BITS_ITERATOR

#include <__bits/memory/addressof.hpp>
#include <cstdlib>
#include <initializer_list>
#include <iosfwd>
#include <type_traits>
#include <utility>

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
     * 24.4.4, iterator operations
     */

    template<class InputIterator, class Distance>
    void advance(InputIterator& it, Distance n)
    {
        for (Distance i = Distance{}; i < n; ++i)
            ++it;
    }

    template<class InputIterator>
    typename iterator_traits<InputIterator>::difference_type
    distance(InputIterator first, InputIterator last)
    {
        using cat_t = typename iterator_traits<InputIterator>::iterator_category;
        using diff_t = typename iterator_traits<InputIterator>::difference_type;

        if constexpr (is_same_v<cat_t, random_access_iterator_tag>)
            return last - first;
        else
        {
            diff_t diff{};
            while (first != last)
            {
                ++diff;
                ++first;
            }

            return diff;
        }
    }

    template<class ForwardIterator>
    ForwardIterator
    next(ForwardIterator it, typename iterator_traits<ForwardIterator>::difference_type n = 1)
    {
        advance(it, n);

        return it;
    }

    template<class BidirectionalIterator>
    BidirectionalIterator
    prev(BidirectionalIterator it,
         typename iterator_traits<BidirectionalIterator>::difference_type n = 1)
    {
        advance(it, -n);

        return it;
    }

    /**
     * 24.5.1, reverse iterator:
     */

    template<class Iterator>
    class reverse_iterator
        : public iterator<
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
                return addressof(operator*());
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
            using type = remove_reference_t<typename iterator_traits<Iterator>::reference>&&;
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

    /**
     * 24.6, stream iterators:
     */

    /**
     * 24.6.1, class template istream_iterator:
     */

    template<class T, class Char = char, class Traits = char_traits<Char>,
             class Distance = ptrdiff_t>
    class istream_iterator
        : public iterator<input_iterator_tag, T, Distance, const T*, const T&>
    {
        public:
            using char_type    = Char;
            using traits_type  = Traits;
            using istream_type = basic_istream<char_type, traits_type>;

            // TODO: if T is literal, this should be constexpr
            istream_iterator()
                : is_{nullptr}, value_{}
            { /* DUMMY BODY */ }

            istream_iterator(istream_type& is)
                : is_{&is}, value_{}
            { /* DUMMY BODY */ }

            istream_iterator(const istream_iterator&) = default;

            ~istream_iterator() = default;

            const T& operator*() const
            {
                return value_;
            }

            const T* operator->() const
            {
                return &(operator*());
            }

            istream_iterator& operator++()
            {
                if (is_)
                    (*is_) >> value_;

                return *this;
            }

            istream_iterator operator++(int)
            {
                auto tmp{*this};

                if (is_)
                    (*is_) >> value_;

                return tmp;
            }

        private:
            basic_istream<char_type, traits_type>* is_;

            T value_;

            friend bool operator==<>(const istream_iterator&,
                                     const istream_iterator&);

            friend bool operator!=<>(const istream_iterator&,
                                     const istream_iterator&);
    };

    template<class T, class Char, class Traits, class Distance>
    bool operator==(const istream_iterator<T, Char, Traits, Distance>& lhs,
                    const istream_iterator<T, Char, Traits, Distance>& rhs)
    {
        return lhs.is_ == rhs.is_;
    }

    template<class T, class Char, class Traits, class Distance>
    bool operator!=(const istream_iterator<T, Char, Traits, Distance>& lhs,
                    const istream_iterator<T, Char, Traits, Distance>& rhs)
    {
        return !(lhs == rhs);
    }

    /**
     * 24.6.2, class template ostream_iterator:
     */

    template<class T, class Char = char, class Traits = char_traits<Char>>
    class ostream_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        public:
            using char_type    = Char;
            using traits_type  = Traits;
            using ostream_type = basic_ostream<char_type, traits_type>;

            ostream_iterator(ostream_type& os)
                : os_{&os}, delim_{nullptr}
            { /* DUMMY BODY */ }

            ostream_iterator(ostream_type& os, const char_type* delim)
                : os_{&os}, delim_{delim}
            { /* DUMMY BODY */ }

            ostream_iterator(const ostream_iterator&) = default;

            ~ostream_iterator() = default;

            ostream_iterator& operator=(const T& value)
            {
                os_ << value;
                if (delim_)
                    os_ << delim_;

                return *this;
            }

            ostream_iterator& operator*() const
            {
                return *this;
            }

            ostream_iterator& operator++()
            {
                return *this;
            }

            ostream_iterator& operator++(int)
            {
                return *this;
            }

        private:
            basic_ostream<char_type, traits_type>* os_;

            const char_type* delim_;
    };

    /**
     * 24.6.3, class template istreambuf_iterator:
     */

    template<class Char, class Traits>
    class istreambuf_iterator
        : public iterator<input_iterator_tag, Char, typename Traits::off_type, Char*, Char>
    {
        public:
            using char_type      = Char;
            using traits_type    = Traits;
            using int_type       = typename traits_type::int_type;
            using streambuf_type = basic_streambuf<char_type, traits_type>;
            using istream_type   = basic_istream<char_type, traits_type>;

            class proxy_type
            {
                public:
                    proxy_type(int_type c, streambuf_type* sbuf)
                        : char_{c}, sbuf_{sbuf}
                    { /* DUMMY BODY */ }

                    char_type operator*()
                    {
                        return traits_type::to_char_type(char_);
                    }

                private:
                    int_type char_;

                    streambuf_type* sbuf_;
            };

            constexpr istreambuf_iterator() noexcept
                : sbuf_{nullptr}
            { /* DUMMY BODY */ }

            istreambuf_iterator(const istreambuf_iterator&) noexcept = default;

            ~istreambuf_iterator() = default;

            istreambuf_iterator(istream_type& is) noexcept
                : sbuf_{is.rdbuf()}
            { /* DUMMY BODY */ }

            istreambuf_iterator(streambuf_type* sbuf) noexcept
                : sbuf_{sbuf}
            { /* DUMMY BODY */ }

            istreambuf_iterator(const proxy_type& proxy) noexcept
                : sbuf_{proxy.sbuf_}
            { /* DUMMY BODY */ }

            char_type operator*() /* const */ // TODO: Should be const :/
            {
                if (sbuf_)
                {
                    auto res = sbuf_->sgetc();
                    if (res == traits_type::eof())
                        sbuf_ = nullptr;

                    return res;
                }
                else
                    return traits_type::eof();
            }

            istreambuf_iterator& operator++()
            {
                if (sbuf_)
                    sbuf_->sbumpc();

                return *this;
            }

            proxy_type operator++(int)
            {
                if (sbuf_)
                    return proxy_type{sbuf_->sbumpc(), sbuf_};
                else
                    return proxy_type{traits_type::eof(), nullptr};
            }

            bool equal(const istreambuf_iterator& rhs) const
            {
                if ((sbuf_ == nullptr && rhs.sbuf_ == nullptr) ||
                    (sbuf_ != nullptr && rhs.sbuf_ != nullptr))
                    return true;
                else
                    return false;
            }

        private:
            streambuf_type* sbuf_;
    };

    template<class Char, class Traits>
    bool operator==(const istreambuf_iterator<Char, Traits>& lhs,
                    const istreambuf_iterator<Char, Traits>& rhs)
    {
        return lhs.equal(rhs);
    }

    template<class Char, class Traits>
    bool operator!=(const istreambuf_iterator<Char, Traits>& lhs,
                    const istreambuf_iterator<Char, Traits>& rhs)
    {
        return !lhs.equal(rhs);
    }

    /**
     * 24.6.4, class template ostreambuf_iterator:
     */

    template<class Char, class Traits>
    class ostreambuf_iterator
        : public iterator<output_iterator_tag, void, void, void, void>
    {
        public:
            using char_type      = Char;
            using traits_type    = Traits;
            using streambuf_type = basic_streambuf<char_type, traits_type>;
            using ostream_type   = basic_ostream<char_type, traits_type>;

            ostreambuf_iterator(ostream_type& os) noexcept
                : sbuf_{os.rdbuf()}
            { /* DUMMY BODY */ }

            ostreambuf_iterator(streambuf_type* sbuf) noexcept
                : sbuf_{sbuf}
            { /* DUMMY BODY */ }

            ostreambuf_iterator& operator=(char_type c)
            {
                if (!failed() && sbuf_->sputc(c) == traits_type::eof())
                    failed_ = true;

                return *this;
            }

            ostreambuf_iterator& operator*()
            {
                return *this;
            }

            ostreambuf_iterator& operator++()
            {
                return *this;
            }

            ostreambuf_iterator& operator++(int)
            {
                return *this;
            }

            bool failed() const noexcept
            {
                return failed_;
            }

        private:
            streambuf_type* sbuf_;

            bool failed_{false};
    };

    /**
     * 24.7, range access:
     */

    template<class Container>
    auto begin(Container& c) -> decltype(c.begin())
    {
        return c.begin();
    }

    template<class Container>
    auto begin(const Container& c) -> decltype(c.begin())
    {
        return c.begin();
    }

    template<class Container>
    auto end(Container& c) -> decltype(c.end())
    {
        return c.end();
    }

    template<class Container>
    auto end(const Container& c) -> decltype(c.end())
    {
        return c.end();
    }

    template<class T, size_t N>
    constexpr T* begin(T (&array)[N]) noexcept
    {
        return array;
    }

    template<class T, size_t N>
    constexpr T* end(T (&array)[N]) noexcept
    {
        return array + N;
    }

    template<class Container>
    constexpr auto cbegin(const Container& c) noexcept(noexcept(std::begin(c)))
        -> decltype(std::begin(c))
    {
        return std::begin(c);
    }

    template<class Container>
    constexpr auto cend(const Container& c) noexcept(noexcept(std::end(c)))
        -> decltype(std::end(c))
    {
        return std::end(c);
    }

    template<class Container>
    auto rbegin(Container& c) -> decltype(c.rbegin())
    {
        return c.rbegin();
    }

    template<class Container>
    auto rbegin(const Container& c) -> decltype(c.rbegin())
    {
        return c.rbegin();
    }

    template<class Container>
    auto rend(Container& c) -> decltype(c.rend())
    {
        return c.rend();
    }

    template<class Container>
    auto rend(const Container& c) -> decltype(c.rend())
    {
        return c.rend();
    }

    template<class T, size_t N>
    reverse_iterator<T*> rbegin(T (&array)[N])
    {
        return reverse_iterator<T*>{array + N};
    }

    template<class T, size_t N>
    reverse_iterator<T*> rend(T (&array)[N])
    {
        return reverse_iterator<T*>{array};
    }

    template<class T>
    reverse_iterator<const T*> rbegin(initializer_list<T> init)
    {
        return reverse_iterator<const T*>{init.end()};
    }

    template<class T>
    reverse_iterator<const T*> rend(initializer_list<T> init)
    {
        return reverse_iterator<const T*>{init.begin()};
    }

    template<class Container>
    auto crbegin(const Container& c) -> decltype(std::rbegin(c))
    {
        return std::rbegin(c);
    }

    template<class Container>
    auto crend(const Container& c) -> decltype(std::rend(c))
    {
        return std::rend(c);
    }

    /**
     * 24.8, container access:
     */

    template<class Container>
    constexpr auto size(const Container& c) -> decltype(c.size())
    {
        return c.size();
    }

    template<class T, size_t N>
    constexpr size_t size(T (&array)[N]) noexcept
    {
        return N;
    }

    template<class Container>
    constexpr auto empty(const Container& c) -> decltype(c.empty())
    {
        return c.empty();
    }

    template<class T, size_t N>
    constexpr bool empty(T (&array)[N]) noexcept
    {
        return false;
    }

    template<class T>
    constexpr bool empty(initializer_list<T> init) noexcept
    {
        return init.size() == 0;
    }

    template<class Container>
    constexpr auto data(Container& c) -> decltype(c.data())
    {
        return c.data();
    }

    template<class Container>
    constexpr auto data(const Container& c) -> decltype(c.data())
    {
        return c.data();
    }

    template<class T, size_t N>
    constexpr T* data(T (&array)[N]) noexcept
    {
        return array;
    }

    template<class T>
    constexpr const T* data(initializer_list<T> init) noexcept
    {
        return init.begin();
    }
}

#endif
