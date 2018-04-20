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
                  next{}, prev{}
            {
                next = this;
                prev = this;
            }

            list_node(const T& val)
                : value{val}, next{}, prev{}
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
        class list_const_iterator
        {
            public:
                using value_type = typename list<T>::value_type;
                using reference  = typename list<T>::const_reference;

                using iterator_category = forward_iterator_tag;

                list_const_iterator(list_node<value_type>* node = nullptr,
                                    list_node<value_type>* head = nullptr)
                    : current_{node}, head_{head}
                { /* DUMMY BODY */ }

                list_const_iterator(const list_const_iterator&) = default;
                list_const_iterator& operator=(const list_const_iterator&) = default;
                list_const_iterator(list_const_iterator&&) = default;
                list_const_iterator& operator=(list_const_iterator&&) = default;

                reference operator*() const
                {
                    return current_->value;
                }

                list_const_iterator& operator++()
                {
                    if (current_)
                    {
                        if (current_->next == head_)
                            current_ = nullptr;
                        else
                            current_ = current_->next;
                    }

                    return *this;
                }

                list_const_iterator operator++(int)
                {
                    auto bckp = current_;

                    if (current_)
                    {
                        if (current_->next == head_)
                            current_ = nullptr;
                        else
                            current_ = current_->next;
                    }

                    return list_const_iterator{bckp};
                }

                list_node<value_type>* node()
                {
                    return current_;
                }

                const list_node<value_type>* node() const
                {
                    return current_;
                }

            private:
                list_node<value_type>* current_;
                list_node<value_type>* head_;
        };

        template<class T>
        bool operator==(const list_const_iterator<T>& lhs, const list_const_iterator<T>& rhs)
        {
            return lhs.node() == rhs.node();
        }

        template<class T>
        bool operator!=(const list_const_iterator<T>& lhs, const list_const_iterator<T>& rhs)
        {
            return !(lhs == rhs);
        }

        template<class T>
        class list_iterator
        {
            public:
                using value_type = typename list<T>::value_type;
                using reference  = typename list<T>::reference;

                using iterator_category = forward_iterator_tag;

                list_iterator(list_node<value_type>* node = nullptr,
                              list_node<value_type>* head = nullptr)
                    : current_{node}, head_{head}
                { /* DUMMY BODY */ }

                list_iterator(const list_iterator&) = default;
                list_iterator& operator=(const list_iterator&) = default;
                list_iterator(list_iterator&&) = default;
                list_iterator& operator=(list_iterator&&) = default;

                reference operator*()
                {
                    return current_->value;
                }

                list_iterator& operator++()
                {
                    if (current_)
                    {
                        if (current_->next == head_)
                            current_ = nullptr;
                        else
                            current_ = current_->next;
                    }

                    return *this;
                }

                list_iterator operator++(int)
                {
                    auto bckp = current_;

                    if (current_)
                    {
                        if (current_->next == head_)
                            current_ = nullptr;
                        else
                            current_ = current_->next;
                    }

                    return list_iterator{bckp};
                }

                list_node<value_type>* node()
                {
                    return current_;
                }

                const list_node<value_type>* node() const
                {
                    return current_;
                }

                operator list_const_iterator<T>() const
                {
                    return list_const_iterator<T>{current_};
                }

            private:
                list_node<value_type>* current_;
                list_node<value_type>* head_;
        };

        template<class T>
        bool operator==(const list_iterator<T>& lhs, const list_iterator<T>& rhs)
        {
            return lhs.node() == rhs.node();
        }

        template<class T>
        bool operator!=(const list_iterator<T>& lhs, const list_iterator<T>& rhs)
        {
            return !(lhs == rhs);
        }
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
                return iterator{head_, head_};
            }

            const_iterator begin() const noexcept
            {
                return cbegin();
            }

            iterator end() noexcept
            {
                return iterator{nullptr, head_};
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
                return const_iterator{head_, head_};
            }

            const_iterator cend() const noexcept
            {
                return const_iterator{nullptr, head_};
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

            template<class... Args>
            iterator emplace(const_iterator position, Args&&... args)
            {
                auto node = position.node();
                node->prepend(new aux::list_node<value_type>{forward<Args>(args)...});
                ++size_;

                if (node == head_)
                    head_ = head_->prev;

                return iterator{node->prev};
            }

            iterator insert(const_iterator position, const value_type& val)
            {
                return emplace(position, val);
            }

            iterator insert(const_iterator position, value_type&& val)
            {
                return emplace(position, forward<value_type>(val));
            }

            iterator insert(const_iterator position, size_type n, const value_type& val)
            {
                return insert(
                    position,
                    aux::insert_iterator<value_type>{0u, val},
                    aux::insert_iterator<value_type>{n}
                );
            }

            template<class InputIterator>
            iterator insert(const_iterator position, InputIterator first, InputIterator last)
            {
                auto node = position.node()->prev;

                while (first != last)
                {
                    node->append(new aux::list_node<value_type>{*first++});
                    node = node->next;
                    ++size_;
                }

                return iterator{position.node()->next};
            }

            iterator insert(const_iterator position, initializer_list<value_type> init)
            {
                return insert(position, init.begin(), init.end());
            }

            iterator erase(const_iterator position)
            {
                auto node = position.node();
                --size_;

                if (node != get_last_())
                {
                    auto next = node->next;
                    auto prev = node->prev;

                    next->prev = prev;
                    prev->next = next;

                    delete node;

                    return iterator{next};
                }
                else
                {
                    auto prev = node->prev;
                    head_->prev = prev;
                    prev->next = head_;

                    delete node;

                    return end();
                }
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                if (first == last)
                    return end();

                auto first_node = first.node();
                auto last_node = last.node();
                auto prev = first_node->prev;
                auto next = last_node->next;

                prev->append(next);

                while (first_node != next)
                {
                    auto tmp = first_node;
                    first_node = first_node->next;
                    --size_;

                    delete tmp;
                }

                return iterator{next};
            }

            void swap(list& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                std::swap(allocator_, other.allocator_);
                std::swap(head_, other.head_);
                std::swap(size_, other.size_);
            }

            void clear()
            {
                fini_();
            }

        private:
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
