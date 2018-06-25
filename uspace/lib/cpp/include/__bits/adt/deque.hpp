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

#ifndef LIBCPP_BITS_ADT_DEQUE
#define LIBCPP_BITS_ADT_DEQUE

#include <__bits/insert_iterator.hpp>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>

namespace std
{
    template<class T, class Allocator = allocator<T>>
    class deque;

    namespace aux
    {
        /**
         * Note: We decided that these iterators contain a
         *       reference to the container and an index, which
         *       allows us to use the already implemented operator[]
         *       on deque and also allows us to conform to the requirement
         *       of the standard that functions such as push_back
         *       invalidate the .end() iterator.
         */

        template<class T, class Allocator>
        class deque_iterator;

        template<class T, class Allocator>
        class deque_const_iterator
        {
            public:
                using size_type         = typename deque<T, Allocator>::size_type;
                using value_type        = typename deque<T, Allocator>::value_type;
                using reference         = typename deque<T, Allocator>::const_reference;
                using difference_type   = size_type;
                using pointer           = const value_type*;
                using iterator_category = random_access_iterator_tag;

                deque_const_iterator(const deque<T, Allocator>& deq, size_type idx)
                    : deq_{deq}, idx_{idx}
                { /* DUMMY BODY */ }

                deque_const_iterator(const deque_const_iterator& other)
                    : deq_{other.deq_}, idx_{other.idx_}
                { /* DUMMY BODY */ }

                deque_const_iterator& operator=(const deque_const_iterator& other)
                {
                    deq_ = other.deq_;
                    idx_ = other.idx_;

                    return *this;
                }

                reference operator*() const
                {
                    return deq_[idx_];
                }

                pointer operator->() const
                {
                    return addressof(deq_[idx_]);
                }

                deque_const_iterator& operator++()
                {
                    ++idx_;

                    return *this;
                }

                deque_const_iterator operator++(int)
                {
                    return deque_const_iterator{deq_, idx_++};
                }

                deque_const_iterator& operator--()
                {
                    --idx_;

                    return *this;
                }

                deque_const_iterator operator--(int)
                {
                    return deque_const_iterator{deq_, idx_--};
                }

                deque_const_iterator operator+(difference_type n)
                {
                    return deque_const_iterator{deq_, idx_ + n};
                }

                deque_const_iterator& operator+=(difference_type n)
                {
                    idx_ += n;

                    return *this;
                }

                deque_const_iterator operator-(difference_type n)
                {
                    return deque_const_iterator{deq_, idx_ - n};
                }

                deque_const_iterator& operator-=(difference_type n)
                {
                    idx_ -= n;

                    return *this;
                }

                reference operator[](difference_type n) const
                {
                    return deq_[idx_ + n];
                }

                difference_type operator-(const deque_const_iterator& rhs)
                {
                    return idx_ - rhs.idx_;
                }

                size_type idx() const
                {
                    return idx_;
                }

                operator deque_iterator<T, Allocator>()
                {
                    return deque_iterator{
                        const_cast<deque<T, Allocator>&>(deq_), idx_
                    };
                }

            private:
                const deque<T, Allocator>& deq_;
                size_type idx_;
        };

        template<class T, class Allocator>
        bool operator==(const deque_const_iterator<T, Allocator>& lhs,
                        const deque_const_iterator<T, Allocator>& rhs)
        {
            return lhs.idx() == rhs.idx();
        }

        template<class T, class Allocator>
        bool operator!=(const deque_const_iterator<T, Allocator>& lhs,
                        const deque_const_iterator<T, Allocator>& rhs)
        {
            return !(lhs == rhs);
        }

        template<class T, class Allocator>
        class deque_iterator
        {
            public:
                using size_type         = typename deque<T, Allocator>::size_type;
                using value_type        = typename deque<T, Allocator>::value_type;
                using reference         = typename deque<T, Allocator>::reference;
                using difference_type   = size_type;
                using pointer           = value_type*;
                using iterator_category = random_access_iterator_tag;

                deque_iterator(deque<T, Allocator>& deq, size_type idx)
                    : deq_{deq}, idx_{idx}
                { /* DUMMY BODY */ }

                deque_iterator(const deque_iterator& other)
                    : deq_{other.deq_}, idx_{other.idx_}
                { /* DUMMY BODY */ }

                deque_iterator(const deque_const_iterator<T, Allocator>& other)
                    : deq_{other.deq_}, idx_{other.idx_}
                { /* DUMMY BODY */ }

                deque_iterator& operator=(const deque_iterator& other)
                {
                    deq_ = other.deq_;
                    idx_ = other.idx_;

                    return *this;
                }

                deque_iterator& operator=(const deque_const_iterator<T, Allocator>& other)
                {
                    deq_ = other.deq_;
                    idx_ = other.idx_;

                    return *this;
                }

                reference operator*()
                {
                    return deq_[idx_];
                }

                pointer operator->()
                {
                    return addressof(deq_[idx_]);
                }

                deque_iterator& operator++()
                {
                    ++idx_;

                    return *this;
                }

                deque_iterator operator++(int)
                {
                    return deque_iterator{deq_, idx_++};
                }

                deque_iterator& operator--()
                {
                    --idx_;

                    return *this;
                }

                deque_iterator operator--(int)
                {
                    return deque_iterator{deq_, idx_--};
                }

                deque_iterator operator+(difference_type n)
                {
                    return deque_iterator{deq_, idx_ + n};
                }

                deque_iterator& operator+=(difference_type n)
                {
                    idx_ += n;

                    return *this;
                }

                deque_iterator operator-(difference_type n)
                {
                    return deque_iterator{deq_, idx_ - n};
                }

                deque_iterator& operator-=(difference_type n)
                {
                    idx_ -= n;

                    return *this;
                }

                reference operator[](difference_type n) const
                {
                    return deq_[idx_ + n];
                }

                difference_type operator-(const deque_iterator& rhs)
                {
                    return idx_ - rhs.idx_;
                }

                size_type idx() const
                {
                    return idx_;
                }

                operator deque_const_iterator<T, Allocator>()
                {
                    return deque_const_iterator{deq_, idx_};
                }

            private:
                deque<T, Allocator>& deq_;
                size_type idx_;
        };

        template<class T, class Allocator>
        bool operator==(const deque_iterator<T, Allocator>& lhs,
                        const deque_iterator<T, Allocator>& rhs)
        {
            return lhs.idx() == rhs.idx();
        }

        template<class T, class Allocator>
        bool operator!=(const deque_iterator<T, Allocator>& lhs,
                        const deque_iterator<T, Allocator>& rhs)
        {
            return !(lhs == rhs);
        }
    }

    /**
     * 23.3.3 deque:
     */

    template<class T, class Allocator>
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
                : allocator_{alloc}, front_bucket_idx_{bucket_size_},
                  back_bucket_idx_{0}, front_bucket_{default_front_},
                  back_bucket_{default_back_}, bucket_count_{default_bucket_count_},
                  bucket_capacity_{default_bucket_capacity_}, size_{}, data_{}
            {
                init_();
            }

            explicit deque(size_type n, const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, front_bucket_idx_{bucket_size_}, back_bucket_idx_{},
                  front_bucket_{}, back_bucket_{}, bucket_count_{},
                  bucket_capacity_{}, size_{n}, data_{}
            {
                prepare_for_size_(n);
                init_();

                for (size_type i = 0; i < size_; ++i)
                    allocator_.construct(&(*this)[i]);
                back_bucket_idx_ = size_ % bucket_size_;
            }

            deque(size_type n, const value_type& value, const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, front_bucket_idx_{bucket_size_}, back_bucket_idx_{},
                  front_bucket_{}, back_bucket_{}, bucket_count_{},
                  bucket_capacity_{}, size_{n}, data_{}
            {
                prepare_for_size_(n);
                init_();

                for (size_type i = 0; i < size_; ++i)
                    (*this)[i] = value;
                back_bucket_idx_ = size_ % bucket_size_;
            }

            template<class InputIterator>
            deque(InputIterator first, InputIterator last,
                  const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, front_bucket_idx_{bucket_size_},
                  back_bucket_idx_{}, front_bucket_{}, back_bucket_{},
                  bucket_count_{}, bucket_capacity_{}, size_{},
                  data_{}
            {
                copy_from_range_(first, last);
            }

            deque(const deque& other)
                : deque{other.begin(), other.end(), other.allocator_}
            { /* DUMMY BODY */ }

            deque(deque&& other)
                : allocator_{move(other.allocator_)},
                  front_bucket_idx_{other.front_bucket_idx_},
                  back_bucket_idx_{other.back_bucket_idx_},
                  front_bucket_{other.front_bucket_},
                  back_bucket_{other.back_bucket_},
                  bucket_count_{other.bucket_count_},
                  bucket_capacity_{other.bucket_capacity_},
                  size_{other.size_}, data_{other.data_}
            {
                other.data_ = nullptr;
                other.clear();
            }

            deque(const deque& other, const allocator_type& alloc)
                : deque{other.begin(), other.end(), alloc}
            { /* DUMMY BODY */ }

            deque(deque&& other, const allocator_type& alloc)
                : allocator_{alloc},
                  front_bucket_idx_{other.front_bucket_idx_},
                  back_bucket_idx_{other.front_bucket_idx_},
                  front_bucket_{other.front_bucket_},
                  back_bucket_{other.back_bucket_},
                  bucket_count_{other.bucket_count_},
                  bucket_capacity_{other.bucket_capacity_},
                  size_{other.size_}, data_{other.data_}
            {
                other.data_ = nullptr;
                other.clear();
            }

            deque(initializer_list<T> init, const allocator_type& alloc = allocator_type{})
                : allocator_{alloc}, front_bucket_idx_{bucket_size_},
                  back_bucket_idx_{}, front_bucket_{}, back_bucket_{},
                  bucket_count_{}, bucket_capacity_{}, size_{},
                  data_{}
            {
                copy_from_range_(init.begin(), init.end());
            }

            ~deque()
            {
                fini_();
            }

            deque& operator=(const deque& other)
            {
                if (data_)
                    fini_();

                copy_from_range_(other.begin(), other.end());

                return *this;
            }

            deque& operator=(deque&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                swap(other);
                other.clear();

                return *this;
            }

            deque& operator=(initializer_list<T> init)
            {
                if (data_)
                    fini_();

                copy_from_range_(init.begin(), init.end());

                return *this;
            }

            template<class InputIterator>
            void assign(InputIterator first, InputIterator last)
            {
                copy_from_range_(first, last);
            }

            void assign(size_type n, const T& value)
            {
                prepare_for_size_(n);
                init_();
                size_ = n;

                auto it = begin();
                for (size_type i = size_type{}; i < n; ++i)
                    *it++ = value;
            }

            void assign(initializer_list<T> init)
            {
                copy_from_range_(init.begin(), init.end());
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_;
            }

            iterator begin() noexcept
            {
                return aux::deque_iterator{*this, 0};
            }

            const_iterator begin() const noexcept
            {
                return aux::deque_const_iterator{*this, 0};
            }

            iterator end() noexcept
            {
                return aux::deque_iterator{*this, size_};
            }

            const_iterator end() const noexcept
            {
                return aux::deque_const_iterator{*this, size_};
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
                return aux::deque_const_iterator{*this, 0};
            }

            const_iterator cend() const noexcept
            {
                return aux::deque_const_iterator{*this, size_};
            }

            const_reverse_iterator crbegin() const noexcept
            {
                return make_reverse_iterator(cend());
            }

            const_reverse_iterator crend() const noexcept
            {
                return make_reverse_iterator(cbegin());
            }

            /**
             * 23.3.3.3, capacity:
             */

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
                if (sz <= size_)
                {
                    auto count = size_ - sz;
                    for (size_type i = 0; i < count; ++i)
                        pop_back();
                }
                else
                {
                    value_type value{};
                    auto count = sz - size_;
                    for (size_type i = 0; i < count; ++i)
                        push_back(value);
                }
            }

            void resize(size_type sz, const value_type& value)
            {
                if (sz <= size_)
                {
                    auto count = size_ - sz;
                    for (size_type i = 0; i < count; ++i)
                        pop_back();
                }
                else
                {
                    auto count = sz - size_;
                    for (size_type i = 0; i < count; ++i)
                        push_back(value);
                }
            }

            void shrink_to_fit()
            {
                /**
                 * We lazily allocate buckets and eagerily deallocate them.
                 * We cannot go into smaller pieces as buckets have fixed size.
                 * Because of this, shrink_to_fit has no effect (as permitted
                 * by the standard).
                 */
            }

            bool empty() const noexcept
            {
                return size_ == size_type{};
            }

            reference operator[](size_type idx)
            {
                return data_[get_bucket_index_(idx)][get_element_index_(idx)];
            }

            const_reference operator[](size_type idx) const
            {
                return data_[get_bucket_index_(idx)][get_element_index_(idx)];
            }

            reference at(size_type idx)
            {
                // TODO: bounds checking
                return operator[](idx);
            }

            const_reference at(size_type idx) const
            {
                // TODO: bounds checking
                return operator[](idx);
            }

            reference front()
            {
                return *begin();
            }

            const_reference front() const
            {
                return *cbegin();
            }

            reference back()
            {
                auto tmp = end();

                return *(--tmp);
            }

            const_reference back() const
            {
                auto tmp = cend();

                return *(--tmp);
            }

            /**
             * 23.3.3.4, modifiers:
             */

            template<class... Args>
            void emplace_front(Args&&... args)
            {
                if (front_bucket_idx_ == 0)
                    add_new_bucket_front_();

                allocator_traits<allocator_type>::construct(
                    allocator_,
                    &data_[front_bucket_][--front_bucket_idx_],
                    forward<Args>(args)...
                );

                ++size_;
            }

            template<class... Args>
            void emplace_back(Args&&... args)
            {
                allocator_traits<allocator_type>::construct(
                    allocator_,
                    &data_[back_bucket_][back_bucket_idx_++],
                    forward<Args>(args)...
                );

                ++size_;

                if (back_bucket_idx_ >= bucket_size_)
                    add_new_bucket_back_();
            }

            template<class... Args>
            iterator emplace(const_iterator position, Args&&... args)
            {
                auto idx = position.idx();
                shift_right_(idx, 1);

                allocator_traits<allocator_type>::construct(
                    allocator_,
                    &data_[get_bucket_index_(idx)][get_element_index_(idx)],
                    forward<Args>(args)...
                );
                ++size_;

                return iterator{*this, idx};
            }

            void push_front(const value_type& value)
            {
                if (front_bucket_idx_ == 0)
                    add_new_bucket_front_();

                data_[front_bucket_][--front_bucket_idx_] = value;
                ++size_;
            }

            void push_front(value_type&& value)
            {
                if (front_bucket_idx_ == 0)
                    add_new_bucket_front_();

                data_[front_bucket_][--front_bucket_idx_] = forward<value_type>(value);
                ++size_;
            }

            void push_back(const value_type& value)
            {
                data_[back_bucket_][back_bucket_idx_++] = value;
                ++size_;

                if (back_bucket_idx_ >= bucket_size_)
                    add_new_bucket_back_();
            }

            void push_back(value_type&& value)
            {
                data_[back_bucket_][back_bucket_idx_++] = forward<value_type>(value);
                ++size_;

                if (back_bucket_idx_ >= bucket_size_)
                    add_new_bucket_back_();
            }

            iterator insert(const_iterator position, const value_type& value)
            {
                auto idx = position.idx();
                shift_right_(idx, 1);

                /**
                 * Note: One may notice that when working with the deque
                 *       iterator, we use its index without any checks.
                 *       This is because a valid iterator will always have
                 *       a valid index as functions like pop_back or erase
                 *       invalidate iterators.
                 */
                data_[get_bucket_index_(idx)][get_element_index_(idx)] = value;
                ++size_;

                return iterator{*this, idx};
            }

            iterator insert(const_iterator position, value_type&& value)
            {
                auto idx = position.idx();
                shift_right_(idx, 1);

                data_[get_bucket_index_(idx)][get_element_index_(idx)] = forward<value_type>(value);
                ++size_;

                return iterator{*this, idx};
            }

            iterator insert(const_iterator position, size_type n, const value_type& value)
            {
                return insert(
                    position,
                    aux::insert_iterator<int>{0u, value},
                    aux::insert_iterator<int>{n}
                );
            }

            template<class InputIterator>
            iterator insert(const_iterator position, InputIterator first, InputIterator last)
            {
                auto idx = position.idx();
                auto count = distance(first, last);

                if (idx < size_ / 2)
                {
                    shift_left_(idx, count);

                    iterator it{*this, idx - 1};
                    while (first != last)
                        *it++ = *first++;
                }
                else
                {
                    shift_right_(idx, count);

                    iterator it{*this, idx};
                    while (first != last)
                        *it++ = *first++;
                }

                size_ += count;

                return iterator{*this, idx};
            }

            iterator insert(const_iterator position, initializer_list<value_type> init)
            {
                return insert(position, init.begin(), init.end());
            }

            void pop_back()
            {
                if (empty())
                    return;

                if (back_bucket_idx_ == 0)
                { // Means we gotta pop data_[back_bucket_ - 1][bucket_size_ - 1]!
                    if (data_[back_bucket_])
                        allocator_.deallocate(data_[back_bucket_], bucket_size_);

                    --back_bucket_;
                    back_bucket_idx_ = bucket_size_ - 1;
                }
                else
                    --back_bucket_idx_;

                allocator_.destroy(&data_[back_bucket_][back_bucket_idx_]);
                --size_;
            }

            void pop_front()
            {
                if (empty())
                    return;

                if (front_bucket_idx_ >= bucket_size_)
                { // Means we gotta pop data_[front_bucket_ + 1][0]!
                    if (data_[front_bucket_])
                        allocator_.deallocate(data_[front_bucket_], bucket_size_);

                    ++front_bucket_;
                    front_bucket_idx_ = 1;

                    allocator_.destroy(&data_[front_bucket_][0]);
                }
                else
                {
                    allocator_.destroy(&data_[front_bucket_][front_bucket_idx_]);

                    ++front_bucket_idx_;
                }

                --size_;
            }

            iterator erase(const_iterator position)
            {
                auto idx = position.idx();
                copy(
                    iterator{*this, idx + 1},
                    end(),
                    iterator{*this, idx}
                );

                /**
                 * Note: We need to not only decrement size,
                 *       but also take care of any issues caused
                 *       by decrement bucket indices, which pop_back
                 *       does for us.
                 */
                pop_back();

                return iterator{*this, idx};
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                if (first == last)
                    return first;

                auto first_idx = first.idx();
                auto last_idx = last.idx();
                auto count = distance(first, last);

                copy(
                    iterator{*this, last_idx},
                    end(),
                    iterator{*this, first_idx}
                );

                for (size_type i = 0; i < count; ++i)
                    pop_back();

                return iterator{*this, first_idx};
            }

            void swap(deque& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value)
            {
                std::swap(allocator_, other.allocator_);
                std::swap(front_bucket_idx_, other.front_bucket_idx_);
                std::swap(back_bucket_idx_, other.back_bucket_idx_);
                std::swap(front_bucket_, other.front_bucket_);
                std::swap(back_bucket_, other.back_bucket_);
                std::swap(bucket_count_, other.bucket_count_);
                std::swap(bucket_capacity_, other.bucket_capacity_);
                std::swap(size_, other.size_);
                std::swap(data_, other.data_);
            }

            void clear() noexcept
            {
                if (data_)
                    fini_();

                front_bucket_ = default_front_;
                back_bucket_ = default_back_;
                bucket_count_ = default_bucket_count_;
                bucket_capacity_ = default_bucket_capacity_;
                size_ = size_type{};

                init_();
            }

        private:
            allocator_type allocator_;

            /**
             * Note: In our implementation, front_bucket_idx_
             *       points at the first element and back_bucket_idx_
             *       points at the one past last element. Because of this,
             *       some operations may be done in inverse order
             *       depending on the side they are applied to.
             */
            size_type front_bucket_idx_;
            size_type back_bucket_idx_;
            size_type front_bucket_;
            size_type back_bucket_;

            static constexpr size_type bucket_size_{16};
            static constexpr size_type default_bucket_count_{2};
            static constexpr size_type default_bucket_capacity_{4};
            static constexpr size_type default_front_{1};
            static constexpr size_type default_back_{2};

            size_type bucket_count_;
            size_type bucket_capacity_;
            size_type size_{};

            value_type** data_;

            void init_()
            {
                data_ = new value_type*[bucket_capacity_];

                for (size_type i = front_bucket_; i <= back_bucket_; ++i)
                    data_[i] = allocator_.allocate(bucket_size_);
            }

            void prepare_for_size_(size_type size)
            {
                if (data_)
                    fini_();

                bucket_count_ = bucket_capacity_ = size / bucket_size_ + 2;

                front_bucket_ = 0;
                back_bucket_ = bucket_capacity_ - 1;

                front_bucket_idx_ = bucket_size_;
                back_bucket_idx_ = size % bucket_size_;
            }

            template<class Iterator>
            void copy_from_range_(Iterator first, Iterator last)
            {
                size_ = distance(first, last);
                prepare_for_size_(size_);
                init_();

                auto it = begin();
                while (first != last)
                    *it++ = *first++;
            }

            void ensure_space_front_(size_type idx, size_type count)
            {
                auto free_space = bucket_size_ - elements_in_front_bucket_();
                if (front_bucket_idx_ == 0)
                    free_space = 0;

                if (count <= free_space)
                {
                    front_bucket_idx_ -= count;
                    return;
                }

                count -= free_space;

                auto buckets_needed = count / bucket_size_;
                if (count % bucket_size_ != 0)
                    ++buckets_needed;

                for (size_type i = size_type{}; i < buckets_needed; ++i)
                    add_new_bucket_front_();

                front_bucket_idx_ = bucket_size_ - (count % bucket_size_);
            }

            void ensure_space_back_(size_type idx, size_type count)
            {
                auto free_space = bucket_size_ - back_bucket_idx_;
                if (count < free_space)
                    return;

                count -= free_space;
                auto buckets_needed = count / bucket_size_;
                if (count % bucket_size_ != 0)
                    ++buckets_needed;

                for (size_type i = size_type{}; i < buckets_needed; ++i)
                    add_new_bucket_back_();

                back_bucket_idx_ = (back_bucket_idx_ + count) % bucket_size_;
            }

            void shift_right_(size_type idx, size_type count)
            {
                ensure_space_back_(idx, count);

                iterator it{*this, idx};
                copy_backward(it, end(), end() + count - 1);

            }

            void shift_left_(size_type idx, size_type count)
            {
                ensure_space_front_(idx, count);

                copy(
                    iterator{*this, count},
                    iterator{*this, idx + count - 1},
                    iterator{*this, 0}
                );
            }

            void fini_()
            {
                for (size_type i = front_bucket_; i <= back_bucket_; ++i)
                    allocator_.deallocate(data_[i], bucket_size_);

                delete[] data_;
                data_ = nullptr;
            }

            bool has_bucket_space_back_() const
            {
                return back_bucket_ < bucket_capacity_ - 1;
            }

            bool has_bucket_space_front_() const
            {
                return front_bucket_ > 0;
            }

            void add_new_bucket_back_()
            {
                if (!has_bucket_space_back_())
                    expand_();

                ++back_bucket_;
                ++bucket_count_;
                data_[back_bucket_] = allocator_.allocate(bucket_size_);
                back_bucket_idx_ = size_type{};
            }

            void add_new_bucket_front_()
            {
                if (!has_bucket_space_front_())
                    expand_();

                --front_bucket_;
                ++bucket_count_;
                data_[front_bucket_] = allocator_.allocate(bucket_size_);
                front_bucket_idx_ = bucket_size_;
            }

            void expand_()
            {
                bucket_capacity_ *= 2;
                value_type** new_data = new value_type*[bucket_capacity_];

                /**
                 * Note: This currently expands both sides whenever one reaches
                 *       its limit. Might be better to expand only one (or both when
                 *       the other is near its limit)?
                 */
                size_type new_front = bucket_capacity_ / 4;
                size_type new_back = new_front + bucket_count_ - 1;

                for (size_type i = new_front, j = front_bucket_; i <= new_back; ++i, ++j)
                    new_data[i] = move(data_[j]);
                std::swap(data_, new_data);

                delete[] new_data;
                front_bucket_ = new_front;
                back_bucket_ = new_back;
            }

            size_type get_bucket_index_(size_type idx) const
            {
                if (idx < elements_in_front_bucket_())
                    return front_bucket_;

                idx -= elements_in_front_bucket_();
                return idx / bucket_size_ + front_bucket_ + 1;
            }

            size_type get_element_index_(size_type idx) const
            {
                if (idx < elements_in_front_bucket_())
                    return bucket_size_ - elements_in_front_bucket_() + idx;

                idx -= elements_in_front_bucket_();
                return idx % bucket_size_;
            }

            size_type elements_in_front_bucket_() const
            {
                return bucket_size_ - front_bucket_idx_;
            }
    };

    template<class T, class Allocator>
    bool operator==(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;

        for (decltype(lhs.size()) i = 0; i < lhs.size(); ++i)
        {
            if (lhs[i] != rhs[i])
                return false;
        }

        return true;
    }

    template<class T, class Allocator>
    bool operator<(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        auto min_size = min(lhs.size(), rhs.size());
        for (decltype(lhs.size()) i = 0; i < min_size; ++i)
        {
            if (lhs[i] >= rhs[i])
                return false;
        }

        if (lhs.size() == rhs.size())
            return true;
        else
            return lhs.size() < rhs.size();
    }

    template<class T, class Allocator>
    bool operator!=(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class T, class Allocator>
    bool operator>(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        return rhs < lhs;
    }

    template<class T, class Allocator>
    bool operator<=(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        return !(rhs < lhs);
    }

    template<class T, class Allocator>
    bool operator>=(const deque<T, Allocator>& lhs, const deque<T, Allocator>& rhs)
    {
        return !(lhs < rhs);
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
