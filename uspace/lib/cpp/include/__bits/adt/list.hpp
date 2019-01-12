/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#ifndef LIBCPP_BITS_ADT_LIST
#define LIBCPP_BITS_ADT_LIST

#include <__bits/adt/list_node.hpp>
#include <__bits/insert_iterator.hpp>
#include <cassert>
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
        class list_iterator;

        template<class T>
        class list_const_iterator
        {
            public:
                using value_type      = typename list<T>::value_type;
                using reference       = typename list<T>::const_reference;
                using pointer         = typename list<T>::const_pointer;
                using difference_type = typename list<T>::difference_type;
                using size_type       = typename list<T>::size_type;

                using iterator_category = bidirectional_iterator_tag;

                list_const_iterator(list_node<value_type>* node = nullptr,
                                    list_node<value_type>* head = nullptr,
                                    bool end = true)
                    : current_{node}, head_{head}, end_{end}
                { /* DUMMY BODY */ }

                list_const_iterator(const list_const_iterator&) = default;
                list_const_iterator& operator=(const list_const_iterator&) = default;
                list_const_iterator(list_const_iterator&&) = default;
                list_const_iterator& operator=(list_const_iterator&&) = default;

                list_const_iterator(const list_iterator<T>& other)
                    : current_{other.node()}, head_{other.head()}, end_{other.end()}
                { /* DUMMY BODY */ }

                reference operator*() const
                {
                    return current_->value;
                }

                list_const_iterator& operator++()
                {
                    if (!end_ && current_)
                    {
                        if (current_->next == head_)
                            end_ = true;
                        else
                            current_ = current_->next;
                    }

                    return *this;
                }

                list_const_iterator operator++(int)
                {
                    auto old = *this;
                    ++(*this);

                    return old;
                }

                list_const_iterator& operator--()
                {
                    if (end_)
                        end_ = false;
                    else if (current_)
                    {
                        if (current_ != head_)
                            current_ = current_->prev;
                        else
                            end_ = true;
                    }

                    return *this;
                }

                list_const_iterator operator--(int)
                {
                    auto old = *this;
                    --(*this);

                    return old;
                }

                list_node<value_type>* node()
                {
                    return const_cast<list_node<value_type>*>(current_);
                }

                const list_node<value_type>* node() const
                {
                    return current_;
                }

                list_node<value_type>* head()
                {
                    return const_cast<list_node<value_type>*>(head_);
                }

                const list_node<value_type>* head() const
                {
                    return head_;
                }

                list_const_iterator operator-(difference_type n) const
                {
                    /**
                     * Note: This operator is for internal purposes only,
                     *       so we do not provide inverse operator or shortcut
                     *       -= operator.
                     */
                    auto tmp = current_;

                    for (difference_type i = 0; i < n; ++i)
                        tmp = tmp->prev;

                    return list_const_iterator{tmp};
                }

                bool end() const
                {
                    return end_;
                }

            private:
                const list_node<value_type>* current_;
                const list_node<value_type>* head_;
                bool end_;
        };

        template<class T>
        bool operator==(const list_const_iterator<T>& lhs, const list_const_iterator<T>& rhs)
        {
            return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
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
                using value_type      = typename list<T>::value_type;
                using reference       = typename list<T>::reference;
                using pointer         = typename list<T>::pointer;
                using difference_type = typename list<T>::difference_type;
                using size_type       = typename list<T>::size_type;

                using iterator_category = bidirectional_iterator_tag;

                list_iterator(list_node<value_type>* node = nullptr,
                              list_node<value_type>* head = nullptr,
                              bool end = true)
                    : current_{node}, head_{head}, end_{end}
                { /* DUMMY BODY */ }

                list_iterator(const list_iterator&) = default;
                list_iterator& operator=(const list_iterator&) = default;
                list_iterator(list_iterator&&) = default;
                list_iterator& operator=(list_iterator&&) = default;

                reference operator*() const
                {
                    return current_->value;
                }

                list_iterator& operator++()
                {
                    if (!end_ && current_)
                    {
                        if (current_->next == head_)
                            end_ = true;
                        else
                            current_ = current_->next;
                    }

                    return *this;
                }

                list_iterator operator++(int)
                {
                    auto old = *this;
                    ++(*this);

                    return old;
                }

                list_iterator& operator--()
                {
                    if (end_)
                        end_ = false;
                    else if (current_)
                    {
                        if (current_ != head_)
                            current_ = current_->prev;
                        else
                            end_ = true;
                    }

                    return *this;
                }

                list_iterator operator--(int)
                {
                    auto old = *this;
                    --(*this);

                    return old;
                }

                list_node<value_type>* node()
                {
                    return current_;
                }

                const list_node<value_type>* node() const
                {
                    return current_;
                }

                list_node<value_type>* head()
                {
                    return head_;
                }

                const list_node<value_type>* head() const
                {
                    return head_;
                }

                list_iterator operator-(difference_type n) const
                {
                    /**
                     * Note: This operator is for internal purposes only,
                     *       so we do not provide inverse operator or shortcut
                     *       -= operator.
                     */
                    auto tmp = current_;

                    for (difference_type i = 0; i < n; ++i)
                        tmp = tmp->prev;

                    return list_iterator{tmp};
                }

                bool end() const
                {
                    return end_;
                }

            private:
                list_node<value_type>* current_;
                list_node<value_type>* head_;
                bool end_;
        };

        template<class T>
        bool operator==(const list_iterator<T>& lhs, const list_iterator<T>& rhs)
        {
            return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
        }

        template<class T>
        bool operator!=(const list_iterator<T>& lhs, const list_iterator<T>& rhs)
        {
            return !(lhs == rhs);
        }

        template<class T>
        bool operator==(const list_const_iterator<T>& lhs, const list_iterator<T>& rhs)
        {
            return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
        }

        template<class T>
        bool operator!=(const list_const_iterator<T>& lhs, const list_iterator<T>& rhs)
        {
            return !(lhs == rhs);
        }

        template<class T>
        bool operator==(const list_iterator<T>& lhs, const list_const_iterator<T>& rhs)
        {
            return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
        }

        template<class T>
        bool operator!=(const list_iterator<T>& lhs, const list_const_iterator<T>& rhs)
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
                    aux::insert_iterator<value_type>{size_type{}, value_type{}},
                    aux::insert_iterator<value_type>{size_, value_type{}}
                );
            }

            list(size_type n, const value_type& val,
                 const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, head_{nullptr}, size_{}
            {
                init_(
                    aux::insert_iterator<value_type>{size_type{}, val},
                    aux::insert_iterator<value_type>{n, value_type{}}
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
                other.size_ = size_type{};
            }

            list(const list& other, const allocator_type alloc)
                : allocator_{alloc}, head_{nullptr}, size_{}
            { // Size is set in init_.
                init_(other.begin(), other.end());
            }

            list(list&& other, const allocator_type& alloc)
                : allocator_{alloc},
                  head_{move(other.head_)},
                  size_{move(other.size_)}
            {
                other.head_ = nullptr;
                other.size_ = size_type{};
            }

            list(initializer_list<value_type> init, const allocator_type& alloc = allocator_type{})
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

                return *this;
            }

            list& operator=(list&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                fini_();

                head_ = move(other.head_);
                size_ = move(other.size_);
                allocator_ = move(other.allocator_);

                other.head_ = nullptr;
                other.size_ = size_type{};

                return *this;
            }

            list& operator=(initializer_list<value_type> init)
            {
                fini_();

                init_(init.begin(), init.end());

                return *this;
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
                    aux::insert_iterator<value_type>{size_type{}, val},
                    aux::insert_iterator<value_type>{n, value_type{}}
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
                return iterator{head_, head_, size_ == 0U};
            }

            const_iterator begin() const noexcept
            {
                return cbegin();
            }

            iterator end() noexcept
            {
                return iterator{get_last_(), head_, true};
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
                return const_iterator{head_, head_, size_ == 0U};
            }

            const_iterator cend() const noexcept
            {
                return const_iterator{get_last_(), head_, true};
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
                __unimplemented();
            }

            void resize(size_type sz, const value_type& val)
            {
                // TODO: implement
                __unimplemented();
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

                return iterator{node->prev, head_, false};
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
                    aux::insert_iterator<value_type>{size_type{}, val},
                    aux::insert_iterator<value_type>{n, value_type{}}
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

                return iterator{position.node()->next, head_, false};
            }

            iterator insert(const_iterator position, initializer_list<value_type> init)
            {
                return insert(position, init.begin(), init.end());
            }

            iterator erase(const_iterator position)
            {
                auto node = position.node();

                if (node == head_)
                {
                    if (size_ == 1)
                    {
                        delete head_;
                        head_ = nullptr;
                        size_ = 0;

                        return end();
                    }
                    else
                        head_ = node->next;
                }

                auto next = node->next;

                --size_;

                node->unlink();
                delete node;

                return iterator{next, head_, size_ == 0U};
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                if (first == last)
                    return end();

                auto first_node = first.node();
                auto last_node = last.node()->prev;
                auto prev = first_node->prev;
                auto next = last_node->next;

                first_node->prev = nullptr;
                last_node->next = nullptr;
                prev->next = next;
                next->prev = prev;

                while (first_node)
                {
                    if (first_node == head_)
                    {
                        head_ = next;
                        head_->prev = prev;
                    }

                    auto tmp = first_node;
                    first_node = first_node->next;
                    --size_;

                    delete tmp;
                }

                return iterator{next, head_, size_ == 0U};
            }

            void swap(list& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                std::swap(allocator_, other.allocator_);
                std::swap(head_, other.head_);
                std::swap(size_, other.size_);
            }

            void clear() noexcept
            {
                fini_();
            }

            /**
             * 23.3.5.5, list operations:
             */

            void splice(const_iterator position, list& other)
            {
                if (!head_)
                {
                    swap(other);
                    return;
                }

                auto other_first = other.head_;
                auto other_last = other.get_last_();
                auto node = position.node();
                auto prev = node->prev;

                // Insert a link range.
                prev->next = other_first;
                other_first->prev = prev;
                node->prev = other_last;
                other_last->next = node;

                size_ += other.size_;

                if (node == head_)
                    head_ = other_first;
                other.head_ = nullptr;
                other.size_ = 0;
            }

            void splice(const_iterator position, list&& other)
            {
                splice(position, other);
            }

            void splice(const_iterator position, list& other, const_iterator it)
            {
                if (&other == this)
                    return;

                auto node = position.node();
                auto target = it.node();

                // Unlink from other.
                target->prev->next = target->next;
                target->next->prev = target->prev;

                // Link to us.
                node->prev->next = target;
                target->prev = node->prev;

                node->prev = target;
                target->next = node;

                --other.size_;
                ++size_;

                if (it.node() == other.head_)
                    other.advance_head_();
            }

            void splice(const_iterator position, list&& other, const_iterator it)
            {
                splice(position, other, it);
            }

            void splice(const_iterator position, list& other,
                        const_iterator first, const_iterator last)
            {
                if (&other == this || other.empty())
                    return;

                if (first == other.begin() && last == other.end())
                { // [first, last) is everything in other.
                    splice(position, other);
                    return;
                }

                // [first_node, last_node] is the inserted range.
                aux::list_node<value_type>* first_node{};
                aux::list_node<value_type>* last_node{};

                if (first == other.begin())
                { // The range is a prefix of other.
                    other.head_ = last.node();
                    other.head_->prev = first.node()->prev;
                    first.node()->prev->next = last.node();

                    first_node = first.node();
                    last_node = last.node()->prev;
                }
                else if (last == other.end())
                { // The range is a suffix of other.
                    auto new_last = first.node()->prev;
                    auto old_last = other.head_->prev;
                    other.head_->prev = new_last;
                    new_last->next = other.head_;

                    first_node = first.node();
                    last_node = old_last;
                }
                else
                { // The range is a subrange of other.
                    first_node = first.node();
                    last_node = last.node()->prev;

                    first_node->prev->next = last.node();
                    last.node()->prev = first_node->prev;
                }

                if (!head_)
                { // Assimilate the range.
                    head_ = first_node;
                    first_node->prev = last_node;
                    last_node->next = first_node;
                }
                else
                {
                    auto target = position.node();
                    target->prev->next = first_node;
                    first_node->prev = target->prev;

                    target->prev = last_node;
                    last_node->next = target;
                }

                auto count = distance(
                    iterator{first_node, nullptr, false},
                    iterator{last_node, nullptr, false}
                );
                ++count;

                size_ += count;
                other.size_ -= count;
            }

            void splice(const_iterator position, list&& other,
                        const_iterator first, const_iterator last)
            {
                splice(position, other, first, last);
            }

            void remove(const value_type& val)
            {
                if (!head_)
                    return;

                auto it = begin();
                while (it != end())
                {
                    if (*it == val)
                        it = erase(it);
                    else
                        ++it;
                }
            }

            template<class Predicate>
            void remove_if(Predicate pred)
            {
                if (!head_)
                    return;

                auto it = begin();
                while (it != end())
                {
                    if (pred(*it))
                        it = erase(it);
                    else
                        ++it;
                }
            }

            void unique()
            {
                if (!head_)
                    return;

                auto it = begin();
                ++it;

                while (it != end())
                {
                    if (*it == *(it - 1))
                        it = erase(it);
                    else
                        ++it;
                }
            }

            template<class BinaryPredicate>
            void unique(BinaryPredicate pred)
            {
                if (!head_)
                    return;

                auto it = begin();
                ++it;

                while (it != end())
                {
                    if (pred(*it, *(it - 1)))
                        it = erase(it);
                    else
                        ++it;
                }
            }

            // TODO: make a generic base for algorithms like merge
            //       and quicksort that uses a swapper (the <algorithm>
            //       versions would use std::swap and list versions would
            //       use a swapper that swaps list nodes)

            void merge(list& other)
            {
                // TODO: implement
                __unimplemented();
            }

            void merge(list&& other)
            {
                merge(other);
            }

            template<class Compare>
            void merge(list& other, Compare comp)
            {
                // TODO: implement
                __unimplemented();
            }

            template<class Compare>
            void merge(list&& other, Compare comp)
            {
                merge(other, comp);
            }

            void reverse() noexcept
            {
                // TODO: implement
                __unimplemented();
            }

            void sort()
            {
                // TODO: implement
                __unimplemented();
            }

            template<class Compare>
            void sort(Compare comp)
            {
                // TODO: implement
                __unimplemented();
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

                head_->prev->next = nullptr;
                while (head_)
                {
                    auto tmp = head_;
                    head_ = head_->next;

                    delete tmp;
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

            aux::list_node<value_type>* get_last_() const
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

            void advance_head_()
            {
                if (size_ == 1)
                {
                    head_ = nullptr;
                    size_ = 0;
                }
                else
                { // The head_ is deleted outside.
                    head_->next->prev = head_->prev;
                    head_->prev->next = head_->next;
                    head_ = head_->next;
                }
            }
    };

    template<class T, class Allocator>
    void swap(list<T, Allocator>& lhs, list<T, Allocator>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif
