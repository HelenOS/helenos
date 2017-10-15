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

#ifndef LIBCPP_VECTOR
#define LIBCPP_VECTOR

#include <initializer_list>
#include <iterator>
#include <memory>

namespace std
{
    /**
     * 23.3.6, vector:
     */

    template<class T, class Allocator = allocator<T>>
    class vector
    {
        public:
            using value_type             = T;
            using reference              = value_type&;
            using const_reference        = const value_type&;
            using allocator_type         = Allocator;
            using size_type              = size_t;
            using difference_type        = ptrdiff_t;
            using pointer                = typename allocator_traits<Allocator>::pointer;
            using const_pointer          = typename allocator_traits<Allocator>::pointer;
            using iterator               = pointer;
            using const_iterator         = const_pointer;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            vector() noexcept
                : vector{Allocator{}}
            { /* DUMMY BODY */ }

            explicit vector(const Allocator&);

            explicit vector(size_type n, const Allocator& alloc = Allocator{});

            vector(size_type n, const T& val, const Allocator& alloc = Allocator{});

            template<class InputIterator>
            vector(InputIterator first, InputIterator last,
                   const Allocator& alloc = Allocator{});

            vector(const vector& other);

            vector(vector&& other) noexcept;

            vector(const vector& other, const Allocator& alloc);

            vector(initializer_list<T> init, const Allocator& alloc = Allocator{});

            ~vector();

            vector& operator=(const vector& other);

            vector& operator=(vector&& other)
                noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
                         allocator_traits<Allocator>::is_always_equal::value)
            {
                return *this;
            }

            vector& operator=(initializer_list<T> init);

            template<class InputIterator>
            void assign(InputIterator first, InputIterator last);

            void assign(size_type n, const T& val);

            void assign(initializer_list<T> init);

            allocator_type get_allocator() const noexcept;
    };
}

#endif
