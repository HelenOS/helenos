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

#ifndef LIBCPP_BITS_ADT_QUEUE
#define LIBCPP_BITS_ADT_QUEUE

#include <algorithm>
#include <deque>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace std
{
    /**
     * 23.6.3, class template queue:
     */

    template<class T, class Container = deque<T>>
    class queue
    {
        public:
            using value_type      = typename Container::value_type;
            using reference       = typename Container::reference;
            using const_reference = typename Container::const_reference;
            using size_type       = typename Container::size_type;
            using container_type  = Container;

        protected:
            container_type c;

        public:
            explicit queue(const container_type& cc)
                : c{cc}
            { /* DUMMY BODY */ }

            explicit queue(container_type&& cc = container_type{})
                : c{move(cc)}
            { /* DUMMY BODY */ }

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            explicit queue(const Alloc& alloc)
                : c{alloc}
            { /* DUMMY BODY */}

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            queue(const container_type& cc, const Alloc& alloc)
                : c{cc, alloc}
            { /* DUMMY BODY */}

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            queue(container_type&& cc, const Alloc& alloc)
                : c{move(cc), alloc}
            { /* DUMMY BODY */}

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            queue(const queue& other, const Alloc& alloc)
                : c{other.c, alloc}
            { /* DUMMY BODY */}

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            queue(queue&& other, const Alloc& alloc)
                : c{move(other.c), alloc}
            { /* DUMMY BODY */}

            bool empty() const
            {
                return c.empty();
            }

            size_type size() const
            {
                return c.size();
            }

            reference front()
            {
                return c.front();
            }

            const_reference front() const
            {
                return c.front();
            }

            reference back()
            {
                return c.back();
            }

            const_reference back() const
            {
                return c.back();
            }

            void push(const value_type& val)
            {
                c.push_back(val);
            }

            void push(value_type&& val)
            {
                c.push_back(forward<value_type>(val));
            }

            template<class... Args>
            void emplace(Args&&... args)
            {
                c.emplace_back(forward<Args>(args)...);
            }

            void pop()
            {
                c.pop_front();
            }

            void swap(queue& other)
                noexcept(noexcept(swap(c, other.c)))
            {
                std::swap(c, other.c);
            }

        private:
            template<class U, class C>
            friend bool operator==(const queue<U, C>&, const queue<U, C>&);

            template<class U, class C>
            friend bool operator<(const queue<U, C>&, const queue<U, C>&);

            template<class U, class C>
            friend bool operator!=(const queue<U, C>&, const queue<U, C>&);

            template<class U, class C>
            friend bool operator>(const queue<U, C>&, const queue<U, C>&);

            template<class U, class C>
            friend bool operator>=(const queue<U, C>&, const queue<U, C>&);

            template<class U, class C>
            friend bool operator<=(const queue<U, C>&, const queue<U, C>&);
    };

    template<class T, class Container, class Alloc>
    struct uses_allocator<queue<T, Container>, Alloc>
        : uses_allocator<Container, Alloc>
    { /* DUMMY BODY */ };

    /**
     * 23.6.4, class template priority_queue:
     */

    template<
        class T, class Container = vector<T>,
        class Compare = less<typename Container::value_type>
    >
    class priority_queue
    {
        public:
            using value_type      = typename Container::value_type;
            using reference       = typename Container::reference;
            using const_reference = typename Container::const_reference;
            using size_type       = typename Container::size_type;
            using container_type  = Container;

        protected:
            using compare_type = Compare;

            compare_type comp;
            container_type c;

        public:
            priority_queue(const compare_type& cmp, const container_type& cc)
                : comp{cmp}, c{cc}
            {
                make_heap(c.begin(), c.end(), comp);
            }

            explicit priority_queue(const compare_type& cmp = compare_type{},
                                    container_type&& cc = container_type{})
                : comp{cmp}, c{move(cc)}
            {
                make_heap(c.begin(), c.end(), comp);
            }

            template<class InputIterator>
            priority_queue(InputIterator first, InputIterator last,
                           const compare_type& cmp,
                           const container_type& cc)
                : comp{cmp}, c{cc}
            {
                c.insert(c.end(), first, last);
                make_heap(c.begin(), c.end(), comp);
            }

            template<class InputIterator>
            priority_queue(InputIterator first, InputIterator last,
                           const compare_type& cmp = compare_type{},
                           container_type&& cc = container_type{})
                : comp{cmp}, c{move(cc)}
            {
                c.insert(c.end(), first, last);
                make_heap(c.begin(), c.end(), comp);
            }

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            explicit priority_queue(const Alloc& alloc)
                : comp{}, c{alloc}
            { /* DUMMY BODY */ }

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            priority_queue(const compare_type& cmp, const Alloc& alloc)
                : comp{cmp}, c{alloc}
            { /* DUMMY BODY */ }

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            priority_queue(const compare_type& cmp, const container_type& cc,
                           const Alloc& alloc)
                : comp{cmp}, c{cc, alloc}
            { /* DUMMY BODY */ }

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            priority_queue(const compare_type& cmp, container_type&& cc,
                           const Alloc& alloc)
                : comp{cmp}, c{move(cc), alloc}
            { /* DUMMY BODY */ }

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            priority_queue(const priority_queue& other, const Alloc& alloc)
                : comp{other.comp}, c{other.c, alloc}
            { /* DUMMY BODY */ }

            template<
                class Alloc,
                class = enable_if_t<uses_allocator<container_type, Alloc>::value, void>
            >
            priority_queue(priority_queue&& other, const Alloc& alloc)
                : comp{move(other.comp)}, c{move(other.c), alloc}
            { /* DUMMY BODY */ }

            bool empty() const
            {
                return c.empty();
            }

            size_type size() const
            {
                return c.size();
            }

            const_reference top() const
            {
                return c.front();
            }

            void push(const value_type& val)
            {
                c.push_back(val);
                push_heap(c.begin(), c.end(), comp);
            }

            void push(value_type&& val)
            {
                c.push_back(forward<value_type>(val));
                push_heap(c.begin(), c.end(), comp);
            }

            template<class... Args>
            void emplace(Args&&... args)
            {
                c.emplace_back(forward<Args>(args)...);
                push_heap(c.begin(), c.end(), comp);
            }

            void pop()
            {
                pop_heap(c.begin(), c.end(), comp);
                c.pop_back();
            }

            void swap(priority_queue& other)
                noexcept(noexcept(swap(c, other.c)) && noexcept(swap(comp, other.comp)))
            {
                std::swap(c, other.c);
                std::swap(comp, other.comp);
            }
    };

    template<class T, class Container, class Compare, class Alloc>
    struct uses_allocator<priority_queue<T, Container, Compare>, Alloc>
        : uses_allocator<Container, Alloc>
    { /* DUMMY BODY */ };

    template<class T, class Container>
    bool operator==(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c == rhs.c;
    }

    template<class T, class Container>
    bool operator<(const queue<T, Container>& lhs,
                   const queue<T, Container>& rhs)
    {
        return lhs.c < rhs.c;
    }

    template<class T, class Container>
    bool operator!=(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c != rhs.c;
    }

    template<class T, class Container>
    bool operator>(const queue<T, Container>& lhs,
                   const queue<T, Container>& rhs)
    {
        return lhs.c > rhs.c;
    }

    template<class T, class Container>
    bool operator>=(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c >= rhs.c;
    }

    template<class T, class Container>
    bool operator<=(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c <= rhs.c;
    }

    template<class T, class Container>
    void swap(queue<T, Container>& lhs, queue<T, Container>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class T, class Container, class Compare>
    void swap(priority_queue<T, Container, Compare>& lhs,
              priority_queue<T, Container, Compare>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif
