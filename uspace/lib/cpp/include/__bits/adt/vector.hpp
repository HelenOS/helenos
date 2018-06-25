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

#ifndef LIBCPP_BITS_ADT_VECTOR
#define LIBCPP_BITS_ADT_VECTOR

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>

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

            explicit vector(const Allocator& alloc)
                : data_{nullptr}, size_{}, capacity_{},
                  allocator_{alloc}
            { /* DUMMY BODY */ }

            explicit vector(size_type n, const Allocator& alloc = Allocator{})
                : data_{}, size_{n}, capacity_{n}, allocator_{alloc}
            {
                data_ = allocator_.allocate(capacity_);

                for (size_type i = 0; i < size_; ++i)
                    allocator_traits<Allocator>::construct(allocator_, data_ + i);
            }

            vector(size_type n, const T& val, const Allocator& alloc = Allocator{})
                : data_{}, size_{n}, capacity_{n}, allocator_{alloc}
            {
                data_ = allocator_.allocate(capacity_);

                for (size_type i = 0; i < size_; ++i)
                    data_[i] = val;
            }

            template<class InputIterator>
            vector(InputIterator first, InputIterator last,
                   const Allocator& alloc = Allocator{})
            {
                // TODO: research constraints and provide multiple
                //       implementations via enable_if
            }

            vector(const vector& other)
                : data_{nullptr}, size_{other.size_}, capacity_{other.capacity_},
                  allocator_{other.allocator_}
            {
                data_ = allocator_.allocate(capacity_);

                for (size_type i = 0; i < size_; ++i)
                    data_[i] = other.data_[i];
            }

            vector(vector&& other) noexcept
                : data_{other.data_}, size_{other.size_}, capacity_{other.capacity_},
                  allocator_{move(other.allocator_)}
            {
                // other is guaranteed to be empty()
                other.size_ = other.capacity_ = 0;
                other.data_ = nullptr;
            }

            vector(const vector& other, const Allocator& alloc)
                : data_{nullptr}, size_{other.size_}, capacity_{other.capacity_},
                  allocator_{alloc}
            {
                data_ = allocator_.allocate(capacity_);

                for (size_type i = 0; i < size_; ++i)
                    data_[i] = other.data_[i];
            }

            vector(initializer_list<T> init, const Allocator& alloc = Allocator{})
                : data_{nullptr}, size_{init.size()}, capacity_{init.size()},
                  allocator_{alloc}
            {
                data_ = allocator_.allocate(capacity_);

                auto it = init.begin();
                for (size_type i = 0; it != init.end(); ++i, ++it)
                {
                    data_[i] = *it;
                }
            }

            ~vector()
            {
                allocator_.deallocate(data_, capacity_);
            }

            vector& operator=(const vector& other)
            {
                vector tmp{other};
                swap(tmp);

                return *this;
            }

            vector& operator=(vector&& other)
                noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
                         allocator_traits<Allocator>::is_always_equal::value)
            {
                if (data_)
                    allocator_.deallocate(data_, capacity_);

                // TODO: test this
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                allocator_ = move(other.allocator_);

                other.data_ = nullptr;
                other.size_ = size_type{};
                other.capacity_ = size_type{};
                other.allocator_ = allocator_type{};
                return *this;
            }

            vector& operator=(initializer_list<T> init)
            {
                vector tmp{init, allocator_};
                swap(tmp);

                return *this;
            }

            template<class InputIterator>
            void assign(InputIterator first, InputIterator last)
            {
                vector tmp{first, last};
                swap(tmp);
            }

            void assign(size_type size, const T& val)
            {
                // Parenthesies required to avoid initializer list
                // construction.
                vector tmp(size, val);
                swap(tmp);
            }

            void assign(initializer_list<T> init)
            {
                vector tmp{init};
                swap(tmp);
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_type{allocator_};
            }

            iterator begin() noexcept
            {
                return &data_[0];
            }

            const_iterator begin() const noexcept
            {
                return &data_[0];
            }

            iterator end() noexcept
            {
                return begin() + size_;
            }

            const_iterator end() const noexcept
            {
                return begin() + size_;
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
                return &data_[0];
            }

            const_iterator cend() const noexcept
            {
                return cbegin() + size_;
            }

            const_reverse_iterator crbegin() const noexcept
            {
                return rbegin();
            }

            const_reverse_iterator crend() const noexcept
            {
                return rend();
            }

            size_type size() const noexcept
            {
                return size_;
            }

            size_type max_size() const noexcept
            {
                return std::allocator_traits<Allocator>::max_size(allocator_);
            }

            void resize(size_type sz)
            {
                resize_with_copy_(size_, capacity_);
            }

            void resize(size_type sz, const value_type& val)
            {
                auto old_size = size_;
                resize_with_copy_(sz, capacity_);

                for (size_type i = old_size - 1; i < size_; ++i)
                    data_[i] = val;
            }

            size_type capacity() const noexcept
            {
                return capacity_;
            }

            bool empty() const noexcept
            {
                return size_ == 0;
            }

            void reserve(size_type new_capacity)
            {
                // TODO: if new_capacity > max_size() throw
                //       length_error (this function shall have no
                //       effect in such case)
                if (new_capacity > capacity_)
                    resize_with_copy_(size_, new_capacity);
            }

            void shrink_to_fit()
            {
                resize_with_copy_(size_, size_);
            }

            reference operator[](size_type idx)
            {
                return data_[idx];
            }

            const_reference operator[](size_type idx) const
            {
                return data_[idx];
            }

            reference at(size_type idx)
            {
                // TODO: bounds checking
                return data_[idx];
            }

            const_reference at(size_type idx) const
            {
                // TODO: bounds checking
                return data_[idx];
            }

            reference front()
            {
                /**
                 * Note: Calling front/back on an empty container
                 *       is undefined, we opted for at-like
                 *       behavior to provide our users with means
                 *       to protect their programs from accidental
                 *       accesses to an empty vector.
                 */
                return at(0);
            }

            const_reference front() const
            {
                return at(0);
            }

            reference back()
            {
                return at(size_ - 1);
            }

            const_reference back() const
            {
                return at(size - 1);
            }

            T* data() noexcept
            {
                return data_;
            }

            const T* data() const noexcept
            {
                return data_;
            }

            template<class... Args>
            reference emplace_back(Args&&... args)
            {
                if (size_ >= capacity_)
                    resize_with_copy_(size_, next_capacity_());

                allocator_traits<Allocator>::construct(allocator_,
                                                       begin() + size_, forward<Args>(args)...);

                return back();
            }

            void push_back(const T& x)
            {
                if (size_ >= capacity_)
                    resize_with_copy_(size_, next_capacity_());
                data_[size_++] = x;
            }

            void push_back(T&& x)
            {
                if (size_ >= capacity_)
                    resize_with_copy_(size_, next_capacity_());
                data_[size_++] = forward<T>(x);
            }

            void pop_back()
            {
                destroy_from_end_until_(end() - 1);
                --size_;
            }

            template<class... Args>
            iterator emplace(const_iterator position, Args&&... args)
            {
                auto pos = const_cast<iterator>(position);

                pos = shift_(pos, 1);
                allocator_.construct(pos, forward<Args>(args)...);

                return pos;
            }

            iterator insert(const_iterator position, const value_type& x)
            {
                auto pos = const_cast<iterator>(position);

                pos = shift_(pos, 1);
                *pos = x;

                return pos;
            }

            iterator insert(const_iterator position, value_type&& x)
            {
                auto pos = const_cast<iterator>(position);

                pos = shift_(pos, 1);
                *pos = forward<value_type>(x);

                return pos;
            }

            iterator insert(const_iterator position, size_type count, const value_type& x)
            {
                auto pos = const_cast<iterator>(position);

                pos = shift_(pos, count);
                auto copy_target = pos;
                for (size_type i = 0; i < count; ++i)
                    *copy_target++ = x;

                return pos;
            }

            template<class InputIterator>
            iterator insert(const_iterator position, InputIterator first,
                            InputIterator last)
            {
                auto pos = const_cast<iterator>(position);
                auto count = static_cast<size_type>(last - first);

                pos = shift_(pos, count);
                copy(first, last, pos);

                return pos;
            }

            iterator insert(const_iterator position, initializer_list<T> init)
            {
                auto pos = const_cast<iterator>(position);

                pos = shift_(pos, init.size());
                copy(init.begin(), init.end(), pos);

                return pos;
            }

            iterator erase(const_iterator position)
            {
                iterator pos = const_cast<iterator>(position);
                copy(position + 1, cend(), pos);
                --size_;

                return pos;
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                iterator pos = const_cast<iterator>(first);
                copy(last, cend(), pos);
                size_ -= static_cast<size_type>(last - first);

                return pos;
            }

            void swap(vector& other)
                noexcept(allocator_traits<Allocator>::propagate_on_container_swap::value ||
                         allocator_traits<Allocator>::is_always_equal::value)
            {
                std::swap(data_, other.data_);
                std::swap(size_, other.size_);
                std::swap(capacity_, other.capacity_);
            }

            void clear() noexcept
            {
                // Note: Capacity remains unchanged.
                destroy_from_end_until_(begin());
                size_ = 0;
            }

        private:
            value_type* data_;
            size_type size_;
            size_type capacity_;
            allocator_type allocator_;

            void resize_without_copy_(size_type capacity)
            {
                if (data_)
                    allocator_.deallocate(data_, capacity_);

                data_ = allocator_.allocate(capacity);
                size_ = 0;
                capacity_ = capacity;
            }

            void resize_with_copy_(size_type size, size_type capacity)
            {
                if (size < size_)
                    destroy_from_end_until_(begin() + size);

                if(capacity_ == 0 || capacity_ < capacity)
                {
                    auto new_data = allocator_.allocate(capacity);

                    auto to_copy = min(size, size_);
                    for (size_type i = 0; i < to_copy; ++i)
                        new_data[i] = move(data_[i]);

                    std::swap(data_, new_data);

                    allocator_.deallocate(new_data, capacity_);
                }

                capacity_ = capacity;
                size_ = size;
            }

            void destroy_from_end_until_(iterator target)
            {
                if (!empty())
                {
                    auto last = end();
                    while(last != target)
                        allocator_traits<Allocator>::destroy(allocator_, --last);
                }
            }

            size_type next_capacity_(size_type hint = 0) const noexcept
            {
                if (hint != 0)
                    return max(capacity_ * 2, hint);
                else
                    return max(capacity_ * 2, size_type{2u});
            }

            iterator shift_(iterator position, size_type count)
            {
                if (size_ + count < capacity_)
                {
                    copy_backward(position, end(), end() + count);
                    size_ += count;

                    return position;
                }
                else
                {
                    auto start_idx = static_cast<size_type>(position - begin());
                    auto end_idx = start_idx + count;
                    auto new_size = size_ + count;

                    // Auxiliary vector for easier swap.
                    vector tmp{};
                    tmp.resize_without_copy_(max(new_size, capacity_));
                    tmp.size_ = new_size;

                    // Copy before insertion index.
                    copy(begin(), begin() + start_idx, tmp.begin());

                    // Copy after insertion index.
                    copy(begin() + start_idx, end(), tmp.begin() + end_idx);

                    swap(tmp);

                    // Position was invalidated!
                    return begin() + start_idx;
                }
            }
    };

    template<class T, class Alloc>
    bool operator==(const vector<T, Alloc>& lhs, const vector<T, Alloc>& rhs)
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

    template<class T, class Alloc>
    bool operator<(const vector<T, Alloc>& lhs, const vector<T, Alloc>& rhs)
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

    template<class T, class Alloc>
    bool operator!=(const vector<T, Alloc>& lhs, const vector<T, Alloc>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class T, class Alloc>
    bool operator>(const vector<T, Alloc>& lhs, const vector<T, Alloc>& rhs)
    {
        return rhs < lhs;
    }

    template<class T, class Alloc>
    bool operator>=(const vector<T, Alloc>& lhs, const vector<T, Alloc>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class T, class Alloc>
    bool operator<=(const vector<T, Alloc>& lhs, const vector<T, Alloc>& rhs)
    {
        return !(rhs < lhs);
    }

    /**
     * 23.3.6.6, specialized algorithms:
     */
    template<class T, class Alloc>
    void swap(vector<T, Alloc>& lhs, vector<T, Alloc>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    /**
     * 23.3.7, class vector<bool>:
     */

    // TODO: implement
}

#endif
