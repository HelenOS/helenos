/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ADT_STACK
#define LIBCPP_BITS_ADT_STACK

#include <deque>
#include <initializer_list>
#include <utility>

namespace std
{
    /**
     * 23.5.6.2, stack:
     */

    template<class T, class Container = deque<T>>
    class stack
    {
        public:
            using container_type  = Container;
            using value_type      = typename container_type::value_type;
            using reference       = typename container_type::reference;
            using const_reference = typename container_type::const_reference;
            using size_type       = typename container_type::size_type;

            explicit stack(container_type& cont)
                : c{cont}
            { /* DUMMY BODY */ }

            explicit stack(container_type&& cont = container_type{})
                : c{move(cont)}
            { /* DUMMY BODY */ }

            /**
             * TODO: The allocator constructor should use enable_if
             *       as a last parameter that checks if uses_allocator
             *       from <memory> holds.
             */
            template<class Alloc>
            explicit stack(Alloc& alloc)
                : c{alloc}
            { /* DUMMY BODY */ }

            template<class Alloc>
            stack(const container_type& cont, const Alloc& alloc)
                : c{cont, alloc}
            { /* DUMMY BODY */ }

            template<class Alloc>
            stack(container_type&& cont, const Alloc& alloc)
                : c{move(cont), alloc}
            { /* DUMMY BODY */ }

            template<class Alloc>
            stack(const stack& other, const Alloc& alloc)
                : c{other.c, alloc}
            { /* DUMMY BODY */ }

            template<class Alloc>
            stack(stack&& other, const Alloc& alloc)
                : c{move(other.c), alloc}
            { /* DUMMY BODY */ }

            bool empty()
            {
                return c.empty();
            }

            size_type size()
            {
                return c.size();
            }

            reference top()
            {
                return c.back();
            }

            const_reference top() const
            {
                return c.back();
            }

            void push(const value_type& val)
            {
                c.push_back(val);
            }

            void push(value_type&& val)
            {
                c.push_back(move(val));
            }

            template<class... Args>
            void emplace(Args&&... args)
            {
                c.emplace_back(forward<Args>(args)...);
            }

            void pop()
            {
                c.pop_back();
            }

            void swap(stack& other) // We cannot use c in the noexcept :/
                noexcept(noexcept(declval<container_type>().swap(declval<container_type&>())))
            {
                std::swap(c, other.c);
            }

        protected:
            container_type c;
    };

    /**
     * 23.6.5.5, stack operators:
     */

    template<class T, class Container>
    bool operator==(const stack<T, Container>& lhs,
                    const stack<T, Container>& rhs)
    {
        return lhs.c == rhs.c;
    }

    template<class T, class Container>
    bool operator!=(const stack<T, Container>& lhs,
                    const stack<T, Container>& rhs)
    {
        return lhs.c != rhs.c;
    }

    template<class T, class Container>
    bool operator<(const stack<T, Container>& lhs,
                   const stack<T, Container>& rhs)
    {
        return lhs.c < rhs.c;
    }

    template<class T, class Container>
    bool operator<=(const stack<T, Container>& lhs,
                    const stack<T, Container>& rhs)
    {
        return lhs.c <= rhs.c;
    }

    template<class T, class Container>
    bool operator>(const stack<T, Container>& lhs,
                   const stack<T, Container>& rhs)
    {
        return lhs.c > rhs.c;
    }

    template<class T, class Container>
    bool operator>=(const stack<T, Container>& lhs,
                    const stack<T, Container>& rhs)
    {
        return lhs.c >= rhs.c;
    }

    /**
     * 23.6.5.6, stack specialized algorithms:
     */

    template<class T, class Container>
    void swap(stack<T, Container>& lhs, stack<T, Container>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif
