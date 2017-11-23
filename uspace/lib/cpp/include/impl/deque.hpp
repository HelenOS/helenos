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

#ifndef LIBCPP_DEQUE
#define LIBCPP_DEQUE

#include <initializer_list>
#include <memory>

namespace std
{

    namespace aux
    {
        template<class T, class Allocator>
        class deque_iterator
        {
            // TODO: implement
        };

        template<class T, class Allocator>
        class deque_const_iterator
        {
            // TODO: implement
        };
    }

    /**
     * 23.3.3 deque:
     */

    template<class T, class Allocator = allocator<T>>
    class deque
    {
        public:
            using allocator_type  = Allocator;
            using value_type      = T;
            using reference       = value_type&;
            using const_reference = const value_type&;

            using iterator               = aux::deque_iterator<T, Allocator>;
            using const_iterator         = aux::deque_const_iterator<T, Allocator>;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            using size_type       = typename allocator_traits<allocator_type>::size_type;
            using difference_type = typename allocator_traits<allocator_type>::difference_type;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;

            /**
             * 23.3.3.2, construct/copy/destroy:
             */

            deque()
                : deque{allocator_type{}}
            { /* DUMMY BODY */ }

            explicit deque(const allocator_type& alloc)
            {
                // TODO: implement
            }

            explicit deque(size_type n, const allocator_type& alloc = allocator_type{})
            {
                // TODO: implement
            }

            deque(size_type n, const value_type& value, const allocator_type& alloc = allocator_type{})
            {
                // TODO: implement
            }

            template<class InputIterator>
            deque(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type{})
            {
                // TODO: implement
            }

            deque(const deque& other)
            {
                // TODO: implement
            }

            deque(deque&& other)
            {
                // TODO: implement
            }

            deque(const deque& other, const allocator_type& alloc)
            {
                // TODO: implement
            }

            deque(deque&& other, const allocator_type& alloc)
            {
                // TODO: implement
            }

            deque(initializer_list<T> init, const allocator_type& alloc = allocator_type{})
            {
                // TODO: implement
            }

            ~deque()
            {
                // TODO: implement
            }

            deque& operator=(const deque& other)
            {
                // TODO: implement
            }

            deque& operator=(deque&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                // TODO: implement
            }

            deque& operator=(initializer_list<T> init)
            {
                // TODO: implement
            }

            template<class InputIterator>
            void assign(InputIterator first, InputIterator last)
            {
                // TODO: implement
            }

            void assign(size_type n, const T& value)
            {
                // TODO: implement
            }

            void assign(initializer_list<T> init)
            {
                // TODO: implement
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_;
            }

            iterator begin() noexcept
            {
                // TODO: implement
            }

            const_iterator begin() const noexcept
            {
                // TODO: implement
            }

            iterator end() noexcept
            {
                // TODO: implement
            }

            const_iterator end() const noexcept
            {
                // TODO: implement
            }

            reverse_iterator rbegin() noexcept
            {
                // TODO: implement
            }

            const_reverse_iterator rbegin() const noexcept
            {
                // TODO: implement
            }

            reverse_iterator rend() noexcept
            {
                // TODO: implement
            }

            const_reverse_iterator rend() const noexcept
            {
                // TODO: implement
            }

            const_iterator cbegin() const noexcept
            {
                // TODO: implement
            }

            const_iterator cend() const noexcept
            {
                // TODO: implement
            }

            const_reverse_iterator crbegin() const noexcept
            {
                // TODO: implement
            }

            const_reverse_iterator crend() const noexcept
            {
                // TODO: implement
            }

            /**
             * 23.3.3.3, capacity:
             */

            size_type size() const noexcept
            {
                // TODO: implement
            }

            size_type max_size() const noexcept
            {
                // TODO: implement
            }

            void resize(size_type sz)
            {
                // TODO: implement
            }

            void resize(size_type sz, const value_type& value)
            {
                // TODO: implement
            }

            void shrink_to_fit()
            {
                // TODO: implement
            }

            bool empty() const noexcept
            {
                // TODO: implement
            }

            reference operator[](size_type idx)
            {
                // TODO: implement
            }

            const_reference operator[](size_type idx)
            {
                // TODO: implement
            }

            reference at(size_type idx)
            {
                // TODO: implement
            }

            const_reference at(size_type idx)
            {
                // TODO: implement
            }

            reference front()
            {
                // TODO: implement
            }

            const_reference front() const
            {
                // TODO: implement
            }

            reference back()
            {
                // TODO: implement
            }

            const_reference back() const
            {
                // TODO: implement
            }

            /**
             * 23.3.3.4, modifiers:
             */

            template<class... Args>
            void emplace_front(Args&&... args)
            {
                // TODO: implement
            }

            template<class... Args>
            void emplace_back(Args&&... args)
            {
                // TODO: implement
            }

            template<class... Args>
            iterator emplace(const_iterator position, Args&&... args)
            {
                // TODO: implement
            }

            void push_front(const value_type& value)
            {
                // TODO: implement
            }

            void push_front(value_type&& value)
            {
                // TODO: implement
            }

            void push_back(const value_type& value)
            {
                // TODO: implement
            }

            void push_back(value_type&& value)
            {
                // TODO: implement
            }

            iterator insert(const_iterator position, const value_type& value)
            {
                // TODO: implement
            }

            iterator insert(const_iterator position, value_type&& value)
            {
                // TODO: implement
            }

            iterator insert(const_iterator position, size_type n, const value_type& value)
            {
                // TODO: implement
            }

            template<class InputIterator>
            iterator insert(const_iterator position, InputIterator first, InputIterator last)
            {
                // TODO: implement
            }

            iterator insert(const_iterator position, initializer_list<value_type> init)
            {
                // TODO: implement
            }

            void pop_back()
            {
                // TODO: implement
            }

            void pop_front()
            {
                // TODO: implement
            }

            iterator erase(const_iterator position)
            {
                // TODO: implement
            }

            iterator eraser(cosnt_iteterator first, const_iterator last)
            {
                // TODO: implement
            }

            void swap(deque& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                // TODO: implement
            }

            void clear() noexcept
            {
                // TODO: implement
            }

        private:
            allocator_type allocator_;

            size_type front_bucket_idx_;
            size_type back_bucket_idx_;

            static constexpr size_type bucket_size_{16};

            size_type bucket_count_;
            size_type bucket_capacity_;

            value_type** data_;
    };

    template<class T, class Allocator>
    bool operator==(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        // TODO: implement
    }

    template<class T, class Allocator>
    bool operator<(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        // TODO: implement
    }

    template<class T, class Allocator>
    bool operator=!(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        // TODO: implement
    }

    template<class T, class Allocator>
    bool operator>(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        // TODO: implement
    }

    template<class T, class Allocator>
    bool operator<=(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        // TODO: implement
    }

    template<class T, class Allocator>
    bool operator>=(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        // TODO: implement
    }

    /**
     * 23.3.3.5, deque specialized algorithms:
     */

    template<class T, class Allocator>
    void swap(deque<T, Allocator>& lhs, deque<T, Allocator>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif
