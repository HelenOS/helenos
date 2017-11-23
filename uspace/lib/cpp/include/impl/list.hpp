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

#ifndef LIBCPP_LIST
#define LIBCPP_LIST

#include <iterator>
#include <memory>
#include <utility>

namespace cpp
{
    namespace aux
    {
        template<class T>
        struct list_node
        {
            T value;
            list_node* next;
            list_node* prev;
        };
    }

    /**
     * 23.3.5, class template list:
     */

    template<class T, class Allocator = allocator<T>>
    class list
    {
        public:
            using value_type      = T;
            using reference       = value_type&;
            using const_reference = value_type&;
            using allocator_type  = Allocator;

            using iterator        = void;
            using const_iterator  = void;
            using size_type       = void;
            using difference_type = void;

            using pointer       = typename allocator_traits<allocator_type>::pointer;
            using const_pointer = typename allocator_traits<allocator_type>::const_pointer;

            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::const_reverse_iterator<iterator>;

            /**
             * 23.3.5.2, construct/copy/destroy:
             */

            list()
                : list{allocator_type{}}
            { /* DUMMY BODY */ }

            explicit list(const allocator_type& alloc)
                : allocator_{alloc}, head_{nullptr}
            { /* DUMMY BODY */ }

            explicit list(size_type n, const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, head_{nullptr}
            {
                for (size_type i = 0; i < n; ++i)
                {
                    auto node = append_new_();
                    allocator_.construct(&node->value);
                }
            }

            list(size_type n, const value_type& val,
                 const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, head_{nullptr}
            {
                for (size_type i = 0; i < n; ++i)
                {
                    auto node = append_new_();
                    allocator_.construct(&node->value, val);
                }
            }

            template<class InputIterator>
            list(InputIterator first, InputIterator last,
                 const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, head_{nullptr}
            {
                while (first != last)
                {
                    auto node = append_new_();
                    allocator_.construct(&node->value, *first++);
                }
            }

            list(const list& other)
                : allocator_{other.allocator_}, head_{nullptr}
            {
                auto tmp = other.head_;

                while (tmp->next != other.head_)
                {
                    auto node = append_new_();
                    allocator_.construct(&node->value, tmp->value);
                }
            }

            list(list&& other)
            {
                // TODO:
            }

        private:
            allocator_type allocator_;
            list_node<T>* head_;

            list_node<T>* append_new_()
            {
                auto node = new aux::list_node<T>{};
                auto last = get_last_();

                if (!last)
                    head_ = node;
                else
                {
                    last->next = node;
                    node->prev = last;

                    node->next = head_;
                    head_->prev = node;
                }

                return node;
            }

            list_node<T>* get_last_()
            {
                if (!head_)
                    return nullptr;

                auto node = head_;

                while (node->next != head_)
                    node = node->next;
            }
    };
}

#endif
