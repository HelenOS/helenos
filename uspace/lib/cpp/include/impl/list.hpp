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

#include <cstdlib>
#include <iterator>
#include <memory>
#include <utility>

namespace std
{
    template<class T, class Allocator = allocator<T>>
    class list;

    namespace aux
    {
        template<class T>
        struct list_node
        {
            T value;
            list_node* next;
            list_node* prev;

            template<class... Args>
            list_node(Args&&... args)
                : value{forward<Args>(args)...},
                  next{this}, prev{this}
            {
                next = this;
                prev = this;
            }

            list_node(const T& val)
                : value{val}, next{this}, prev{this}
            {
                next = this;
                prev = this;
            }

            list_node(T&& val)
                : value{forward<T>(val)}, next{}, prev{}
            {
                next = this;
                prev = this;
            }

            void append(list_node* node)
            {
                node->next = next;
                node->prev = this;
                next->prev = node;
                next = node;
            }

            void prepend(list_node* node)
            {
                node->next = this;
                node->prev = prev;
                prev->next = node;
                prev = node;
            }
        };

        template<class T>
        class list_const_iterator;

        template<class T>
        class list_iterator
        {
            public:
                using reference = typename list<T>::reference;

                list_iterator(list_node<T>* node = nullptr)
                    : current_{node}
                { /* DUMMY BODY */ }

                reference operator*()
                {
                    return current_->value;
                }

                list_iterator& operator++()
                {
                    if (current_)
                        current_ = current_->next;

                    return *this;
                }

                list_iterator operator++(int)
                {
                    auto bckp = current_;

                    if (current_)
                        current_ = current_->next;

                    return list_iterator{bckp};
                }

                list_iterator& operator--()
                {
                    if (current_)
                        current_ = current_->prev;

                    return *this;
                }

                list_iterator operator--(int)
                {
                    auto bckp = current_;

                    if (current_)
                        current_ = current_->prev;

                    return list_iterator{bckp};
                }

                list_node<T>* node()
                {
                    return current_;
                }

            private:
                list_node<T>* current_;
        };

        // TODO: const iterator, iterator must be convertible to const iterator
    }

    /**
     * 23.3.5, class template list:
     */

    template<class T, class Allocator>
    class list
    {
        public:
            using value_type      = T;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using allocator_type  = Allocator;

            using iterator        = aux::list_iterator<value_type>;
            using const_iterator  = aux::list_const_iterator<value_type>;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            using pointer       = typename allocator_traits<allocator_type>::pointer;
            using const_pointer = typename allocator_traits<allocator_type>::const_pointer;

            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            /**
             * 23.3.5.2, construct/copy/destroy:
             */

            list()
                : list{allocator_type{}}
            { /* DUMMY BODY */ }

            explicit list(const allocator_type& alloc)
                : allocator_{alloc}, head_{nullptr}, size_{}
            { /* DUMMY BODY */ }

            explicit list(size_type n, const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, head_{nullptr}, size_{}
            {
                init_(
                    aux::insert_iterator<value_type>{value_type{}},
                    aux::insert_iterator<value_type>{size_}
                );
            }

            list(size_type n, const value_type& val,
                 const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, head_{nullptr}, size_{}
            {
                init_(
                    aux::insert_iterator<value_type>{val},
                    aux::insert_iterator<value_type>{n}
                );
            }

            template<class InputIterator>
            list(InputIterator first, InputIterator last,
                 const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, head_{nullptr}, size_{}
            {
                init_(first, last);
            }

            list(const list& other)
                : list{other, other.allocator_}
            { /* DUMMY BODY */ }

            list(list&& other)
                : allocator_{move(other.allocator_)},
                  head_{move(other.head_)},
                  size_{move(other.size_)}
            {
                other.head_ = nullptr;
            }

            list(const list& other, const allocator_type alloc)
                : allocator_{alloc}, head_{nullptr}, size_{other.size_}
            {
                init_(other.begin(), other.end());
            }

            list(list&& other, const allocator_type& alloc)
                : allocator_{alloc},
                  head_{move(other.head_)},
                  size_{move(other.size_)}
            {
                other.head_ = nullptr;
            }

            list(initializer_list<value_type> init, const allocator_type& alloc)
                : allocator_{alloc}, head_{nullptr}, size_{}
            {
                init_(init.begin(), init.end());
            }

            ~list()
            {
                fini_();
            }

            list& operator=(const list& other)
            {
                fini_();

                allocator_ = other.allocator_;

                init_(other.begin(), other.end());
            }

            list& operator=(list&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                fini_();

                allocator_ = move(other.allocator_);

                init_(
                    make_move_iterator(other.begin()),
                    make_move_iterator(other.end())
                );
            }

            list& operator=(initializer_list<value_type> init)
            {
                fini_();

                init_(init.begin(), init.end());
            }

            template<class InputIterator>
            void assign(InputIterator first, InputIterator last)
            {
                fini_();

                init_(first, last);
            }

            void assign(size_type n, const value_type& val)
            {
                fini_();

                init_(
                    aux::insert_iterator<value_type>{val},
                    aux::insert_iterator<value_type>{n}
                );
            }

            void assign(initializer_list<value_type> init)
            {
                fini_();

                init_(init.begin(), init.end());
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_;
            }

            iterator begin() noexcept
            {
                return iterator{head_};
            }

            const_iterator begin() const noexcept
            {
                return cbegin();
            }

            iterator end() noexcept
            {
                return iterator{};
            }

            const_iterator end() const noexcept
            {
                return cend();
            }

            reverse_iterator rbegin() noexcept
            {
                return make_reverse_iterator(end());
            }

            const_reverse_iterator rbegin() const noexcept
            {
                return make_reverse_iterator(cend());
            }

            reverse_iterator rend() noexcept
            {
                return make_reverse_iterator(begin());
            }

            const_reverse_iterator rend() const noexcept
            {
                return make_reverse_iterator(cbegin());
            }

            const_iterator cbegin() const noexcept
            {
                return const_iterator{head_};
            }

            const_iterator cend() const noexcept
            {
                return const_iterator{};
            }

            const_reverse_iterator crbegin() const noexcept
            {
                return rbegin();
            }

            /**
             * 23.3.5.3, capacity:
             */

            bool empty() const noexcept
            {
                return size_ == 0;
            }

            size_type size() const noexcept
            {
                return size_;
            }

            size_type max_size() const noexcept
            {
                return allocator_traits<allocator_type>::max_size(allocator_);
            }

            void resize(size_type sz)
            {
                // TODO: implement
            }

            void resize(size_type sz, const value_type& val)
            {
                // TODO: implement
            }

            reference front()
            {
                // TODO: bounds checking
                return head_->value;
            }

            const_reference front() const
            {
                // TODO: bounds checking
                return head_->value;
            }

            reference back()
            {
                // TODO: bounds checking
                return head_->prev->value;
            }

            const_reference back() const
            {
                // TODO: bounds checking
                return head_->prev->value;
            }

            /**
             * 23.3.5.4, modifiers:
             * Note: These should have no effect when an exception
             *       is thrown while inserting, but since the only
             *       functions that can throw are the constructors,
             *       creating the node before any modifications to the
             *       list itself will satisfy this requirement.
             */

            template<class... Args>
            void emplace_front(Args&&... args)
            {
                prepend_new_(forward<Args>(args)...);
            }

            void pop_front()
            {
                if (head_)
                {
                    --size_;

                    if (head_->next == head_)
                    {
                        delete head_;
                        head_ = nullptr;
                    }
                    else
                    {
                        auto tmp = head_;
                        head_->prev->next = head_->next;
                        head_->next->prev = head_->prev;
                        head_ = head_->next;

                        delete tmp;
                    }
                }
            }

            template<class... Args>
            void emplace_back(Args&&... args)
            {
                append_new_(forward<Args>(args)...);
            }

            void push_front(const value_type& value)
            {
                prepend_new_(value);
            }

            void push_front(value_type&& value)
            {
                prepend_new_(forward<value_type>(value));
            }

            void push_back(const value_type& value)
            {
                append_new_(value);
            }

            void push_back(value_type&& value)
            {
                append_new_(forward<value_type>(value));
            }

            void pop_back()
            {
                if (head_)
                {
                    --size_;
                    auto target = head_->prev;

                    if (!target)
                    {
                        delete head_;
                        head_ = nullptr;
                    }
                    else
                    {
                        auto tmp = target;
                        target->prev->next = target->next;
                        target->next->prev = target->prev;
                        target = target->next;

                        delete tmp;
                    }
                }
            }

        /* private: */
            allocator_type allocator_;
            aux::list_node<value_type>* head_;
            size_type size_;

            template<class InputIterator>
            void init_(InputIterator first, InputIterator last)
            {
                while (first != last)
                {
                    auto node = append_new_();
                    allocator_.construct(&node->value, *first++);
                }
            }

            void fini_()
            {
                if (!head_)
                    return;

                for (size_type i = size_type{}; i < size_; ++i)
                {
                    head_ = head_->next;
                    delete head_->prev;
                }

                head_ = nullptr;
                size_ = size_type{};
            }

            template<class... Args>
            aux::list_node<value_type>* append_new_(Args&&... args)
            {
                auto node = new aux::list_node<value_type>{forward<Args>(args)...};
                auto last = get_last_();

                if (!last)
                    head_ = node;
                else
                    last->append(node);

                ++size_;

                return node;
            }

            template<class... Args>
            aux::list_node<value_type>* prepend_new_(Args&&... args)
            {
                auto node = new aux::list_node<value_type>{forward<Args>(args)...};

                if (!head_)
                    head_ = node;
                else
                {
                    head_->prepend(node);
                    head_ = head_->prev;
                }

                ++size_;

                return node;
            }

            aux::list_node<value_type>* get_last_()
            {
                if (!head_)
                    return nullptr;

                return head_->prev;
            }

            template<class InputIterator>
            void insert_range_(InputIterator first, InputIterator last,
                               aux::list_node<value_type>* where = nullptr)
            {
                if (first == last)
                    return;

                if (!where)
                    where = get_last_();

                while (first != last)
                {
                    where->append(new aux::list_node<value_type>{*first++});
                    where = where->next;
                }
            }
    };
}

#endif
