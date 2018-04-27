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

#ifndef LIBCPP_QUEUE
#define LIBCPP_QUEUE

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

        protected:
            container_type c;

        public: // The noexcept part of swap's declaration needs c to be declared.
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

    template<
        class T, class Container = vector<T>,
        class Compare = less<typename Container::value_type>
    >
    class priority_queue;

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
