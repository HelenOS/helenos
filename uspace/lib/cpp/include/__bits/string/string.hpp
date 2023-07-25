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

#ifndef LIBCPP_BITS_STRING
#define LIBCPP_BITS_STRING

#include <__bits/string/stringfwd.hpp>
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <memory>
#include <utility>

namespace std
{
    /**
     * 21.2, char_traits:
     */

    template<class Char>
    struct char_traits;

    /**
     * 21.2.3, char_traits specializations:
     */

    template<>
    struct char_traits<char>
    {
        using char_type  = char;
        using int_type   = int;
        using off_type   = streamoff;
        using pos_type   = streampos;
        /* using state_type = mbstate_t; */

        static void assign(char_type& c1, const char_type& c2) noexcept
        {
            c1 = c2;
        }

        static constexpr bool eq(char_type c1, char_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr bool lt(char_type c1, char_type c2) noexcept
        {
            return c1 < c2;
        }

        static int compare(const char_type* s1, const char_type* s2, size_t n)
        {
            return ::strncmp(s1, s2, n);
        }

        static size_t length(const char_type* s)
        {
            return ::strlen(s);
        }

        static const char_type* find(const char_type* s, size_t n, const char_type& c)
        {
            size_t i{};
            while (i++ < n)
            {
                if (s[i] == c)
                    return s + i;
            }

            return nullptr;
        }

        static char_type* move(char_type* s1, const char_type* s2, size_t n)
        {
            return static_cast<char_type*>(memmove(s1, s2, n));
        }

        static char_type* copy(char_type* s1, const char_type* s2, size_t n)
        {
            return static_cast<char_type*>(memcpy(s1, s2, n));
        }

        static char_type* assign(char_type* s, size_t n, char_type c)
        {
            /**
             * Note: Even though memset accepts int as its second argument,
             *       the actual implementation assigns that int to a dereferenced
             *       char pointer.
             */
            return static_cast<char_type*>(memset(s, static_cast<int>(c), n));
        }

        static constexpr int_type not_eof(int_type c) noexcept
        {
            if (!eq_int_type(c, eof()))
                return c;
            else
                return to_int_type('a'); // We just need something that is not eof.
        }

        static constexpr char_type to_char_type(int_type c) noexcept
        {
            return static_cast<char_type>(c);
        }

        static constexpr int_type to_int_type(char_type c) noexcept
        {
            return static_cast<int_type>(c);
        }

        static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr int_type eof() noexcept
        {
            return static_cast<int_type>(EOF);
        }
    };

    template<>
    struct char_traits<char16_t>
    {
        // TODO: implement
        using char_type  = char16_t;
        using int_type   = int16_t;
        using off_type   = streamoff;
        using pos_type   = streampos;
        /* using state_type = mbstate_t; */

        static void assign(char_type& c1, const char_type& c2) noexcept
        {
            c1 = c2;
        }

        static constexpr bool eq(char_type c1, char_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr bool lt(char_type c1, char_type c2) noexcept
        {
            return c1 < c2;
        }

        static int compare(const char_type* s1, const char_type* s2, size_t n)
        {
            // TODO: implement
            __unimplemented();
            return 0;
        }

        static size_t length(const char_type* s)
        {
            // TODO: implement
            __unimplemented();
            return 0;
        }

        static const char_type* find(const char_type* s, size_t n, const char_type& c)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static char_type* move(char_type* s1, const char_type* s2, size_t n)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static char_type* copy(char_type* s1, const char_type* s2, size_t n)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static char_type* assign(char_type* s, size_t n, char_type c)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static constexpr int_type not_eof(int_type c) noexcept
        {
            // TODO: implement
            return int_type{};
        }

        static constexpr char_type to_char_type(int_type c) noexcept
        {
            return static_cast<char_type>(c);
        }

        static constexpr int_type to_int_type(char_type c) noexcept
        {
            return static_cast<int_type>(c);
        }

        static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr int_type eof() noexcept
        {
            return static_cast<int_type>(EOF);
        }
    };

    template<>
    struct char_traits<char32_t>
    {
        // TODO: implement
        using char_type  = char32_t;
        using int_type   = int32_t;
        using off_type   = streamoff;
        using pos_type   = streampos;
        /* using state_type = mbstate_t; */

        static void assign(char_type& c1, const char_type& c2) noexcept
        {
            c1 = c2;
        }

        static constexpr bool eq(char_type c1, char_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr bool lt(char_type c1, char_type c2) noexcept
        {
            return c1 < c2;
        }

        static int compare(const char_type* s1, const char_type* s2, size_t n)
        {
            // TODO: implement
            __unimplemented();
            return 0;
        }

        static size_t length(const char_type* s)
        {
            // TODO: implement
            __unimplemented();
            return 0;
        }

        static const char_type* find(const char_type* s, size_t n, const char_type& c)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static char_type* move(char_type* s1, const char_type* s2, size_t n)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static char_type* copy(char_type* s1, const char_type* s2, size_t n)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static char_type* assign(char_type* s, size_t n, char_type c)
        {
            // TODO: implement
            __unimplemented();
            return nullptr;
        }

        static constexpr int_type not_eof(int_type c) noexcept
        {
            // TODO: implement
            return int_type{};
        }

        static constexpr char_type to_char_type(int_type c) noexcept
        {
            return static_cast<char_type>(c);
        }

        static constexpr int_type to_int_type(char_type c) noexcept
        {
            return static_cast<int_type>(c);
        }

        static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr int_type eof() noexcept
        {
            return static_cast<int_type>(EOF);
        }
    };

    template<>
    struct char_traits<wchar_t>
    {
        using char_type  = wchar_t;
        using int_type   = wint_t;
        using off_type   = streamoff;
        using pos_type   = wstreampos;
        /* using state_type = mbstate_t; */

        static void assign(char_type& c1, const char_type& c2) noexcept
        {
            c1 = c2;
        }

        static constexpr bool eq(char_type c1, char_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr bool lt(char_type c1, char_type c2) noexcept
        {
            return c1 < c2;
        }

        static int compare(const char_type* s1, const char_type* s2, size_t n)
        {
            // TODO: This function does not exits...
            __unimplemented();
            return 0;
        }

        static size_t length(const char_type* s)
        {
            size_t i = 0;
            while (s[i] != 0)
                i++;
            return i;
        }

        static const char_type* find(const char_type* s, size_t n, const char_type& c)
        {
            size_t i{};
            while (i++ < n)
            {
                if (s[i] == c)
                    return s + i;
            }

            return nullptr;
        }

        static char_type* move(char_type* s1, const char_type* s2, size_t n)
        {
            return static_cast<char_type*>(memmove(s1, s2, n * sizeof(wchar_t)));
        }

        static char_type* copy(char_type* s1, const char_type* s2, size_t n)
        {
            return static_cast<char_type*>(memcpy(s1, s2, n * sizeof(wchar_t)));
        }

        static char_type* assign(char_type* s, size_t n, char_type c)
        {
            return static_cast<char_type*>(memset(s, static_cast<int>(c), n * sizeof(wchar_t)));
        }

        static constexpr int_type not_eof(int_type c) noexcept
        {
            if (!eq_int_type(c, eof()))
                return c;
            else
                return to_int_type(L'a'); // We just need something that is not eof.
        }

        static constexpr char_type to_char_type(int_type c) noexcept
        {
            return static_cast<char_type>(c);
        }

        static constexpr int_type to_int_type(char_type c) noexcept
        {
            return static_cast<int_type>(c);
        }

        static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept
        {
            return c1 == c2;
        }

        static constexpr int_type eof() noexcept
        {
            return static_cast<int_type>(EOF);
        }
    };

    /**
     * 21.4, class template basic_string:
     */

    template<class Char, class Traits, class Allocator>
    class basic_string
    {
        public:
            using traits_type     = Traits;
            using value_type      = typename traits_type::char_type;
            using allocator_type  = Allocator;
            using size_type       = typename allocator_traits<allocator_type>::size_type;
            using difference_type = typename allocator_traits<allocator_type>::difference_type;

            using reference       = value_type&;
            using const_reference = const value_type&;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;

            using iterator               = pointer;
            using const_iterator         = const_pointer;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            static constexpr size_type npos = -1;

            /**
             * 21.4.2, construct/copy/destroy:
             * TODO: tagged constructor that moves the char*
             *       and use that with asprintf in to_string
             */

            basic_string() noexcept
                : basic_string(allocator_type{})
            { /* DUMMY BODY */ }

            explicit basic_string(const allocator_type& alloc)
                : data_{}, size_{}, capacity_{}, allocator_{alloc}
            {
                /**
                 * Postconditions:
                 *  data() = non-null copyable value that can have 0 added to it.
                 *  size() = 0
                 *  capacity() = unspecified
                 */
                data_ = allocator_.allocate(default_capacity_);
                capacity_ = default_capacity_;
            }

            basic_string(const basic_string& other)
                : data_{}, size_{other.size_}, capacity_{other.capacity_},
                  allocator_{other.allocator_}
            {
                init_(other.data(), size_);
            }

            basic_string(basic_string&& other)
                : data_{other.data_}, size_{other.size_},
                  capacity_{other.capacity_}, allocator_{move(other.allocator_)}
            {
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }

            basic_string(const basic_string& other, size_type pos, size_type n = npos,
                         const allocator_type& alloc = allocator_type{})
                : data_{}, size_{}, capacity_{}, allocator_{alloc}
            {
                // TODO: if pos < other.size() throw out_of_range.
                auto len = min(n, other.size() - pos);
                init_(other.data() + pos, len);
            }

            basic_string(const value_type* str, size_type n, const allocator_type& alloc = allocator_type{})
                : data_{}, size_{n}, capacity_{n}, allocator_{alloc}
            {
                init_(str, size_);
            }

            basic_string(const value_type* str, const allocator_type& alloc = allocator_type{})
                : data_{}, size_{}, capacity_{}, allocator_{alloc}
            {
                init_(str, traits_type::length(str));
            }

            basic_string(size_type n, value_type c, const allocator_type& alloc = allocator_type{})
                : data_{}, size_{n}, capacity_{n + 1}, allocator_{alloc}
            {
                data_ = allocator_.allocate(capacity_);
                for (size_type i = 0; i < size_; ++i)
                    traits_type::assign(data_[i], c);
                ensure_null_terminator_();
            }

            template<class InputIterator>
            basic_string(InputIterator first, InputIterator last,
                         const allocator_type& alloc = allocator_type{})
                : data_{}, size_{}, capacity_{}, allocator_{alloc}
            {
                if constexpr (is_integral<InputIterator>::value)
                { // Required by the standard.
                    size_ = static_cast<size_type>(first);
                    capacity_ = size_;
                    data_ = allocator_.allocate(capacity_);

                    for (size_type i = 0; i < size_; ++i)
                        traits_type::assign(data_[i], static_cast<value_type>(last));
                    ensure_null_terminator_();
                }
                else
                {
                    auto len = static_cast<size_type>(last - first);
                    init_(first, len);
                }
            }

            basic_string(initializer_list<value_type> init, const allocator_type& alloc = allocator_type{})
                : basic_string{init.begin(), init.size(), alloc}
            { /* DUMMY BODY */ }

            basic_string(const basic_string& other, const allocator_type& alloc)
                : data_{}, size_{other.size_}, capacity_{other.capacity_}, allocator_{alloc}
            {
                init_(other.data(), size_);
            }

            basic_string(basic_string&& other, const allocator_type& alloc)
                : data_{other.data_}, size_{other.size_}, capacity_{other.capacity_}, allocator_{alloc}
            {
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }

            ~basic_string()
            {
                allocator_.deallocate(data_, capacity_);
            }

            basic_string& operator=(const basic_string& other)
            {
                if (this != &other)
                {
                    basic_string tmp{other};
                    swap(tmp);
                }

                return *this;
            }

            basic_string& operator=(basic_string&& other)
                noexcept(allocator_traits<allocator_type>::propagate_on_container_move_assignment::value ||
                         allocator_traits<allocator_type>::is_always_equal::value)
            {
                if (this != &other)
                    swap(other);

                return *this;
            }

            basic_string& operator=(const value_type* other)
            {
                *this = basic_string{other};

                return *this;
            }

            basic_string& operator=(value_type c)
            {
                *this = basic_string{1, c};

                return *this;
            }

            basic_string& operator=(initializer_list<value_type> init)
            {
                *this = basic_string{init};

                return *this;
            }

            /**
             * 21.4.3, iterators:
             */

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

            /**
             * 21.4.4, capacity:
             */

            size_type size() const noexcept
            {
                return size_;
            }

            size_type length() const noexcept
            {
                return size();
            }

            size_type max_size() const noexcept
            {
                return 0x7FFF; // TODO: just temporary
                /* return allocator_traits<allocator_type>::max_size(allocator_); */
            }

            void resize(size_type new_size, value_type c)
            {
                // TODO: if new_size > max_size() throw length_error.
                if (new_size > size_)
                {
                    ensure_free_space_(new_size - size_ + 1);
                    for (size_type i = size_; i < new_size; ++i)
                        traits_type::assign(data_[i], i);
                }

                size_ = new_size;
                ensure_null_terminator_();
            }

            void resize(size_type new_size)
            {
                resize(new_size, value_type{});
            }

            size_type capacity() const noexcept
            {
                return capacity_;
            }

            void reserve(size_type new_capacity = 0)
            {
                // TODO: if new_capacity > max_size() throw
                //       length_error (this function shall have no
                //       effect in such case)
                if (new_capacity > capacity_)
                    resize_with_copy_(size_, new_capacity);
                else if (new_capacity < capacity_)
                    shrink_to_fit(); // Non-binding request, but why not.
            }

            void shrink_to_fit()
            {
                if (size_ != capacity_)
                    resize_with_copy_(size_, capacity_);
            }

            void clear() noexcept
            {
                size_ = 0;
            }

            bool empty() const noexcept
            {
                return size_ == 0;
            }

            /**
             * 21.4.5, element access:
             */

            const_reference operator[](size_type idx) const
            {
                return data_[idx];
            }

            reference operator[](size_type idx)
            {
                return data_[idx];
            }

            const_reference at(size_type idx) const
            {
                // TODO: bounds checking
                return data_[idx];
            }

            reference at(size_type idx)
            {
                // TODO: bounds checking
                return data_[idx];
            }

            const_reference front() const
            {
                return at(0);
            }

            reference front()
            {
                return at(0);
            }

            const_reference back() const
            {
                return at(size_ - 1);
            }

            reference back()
            {
                return at(size_ - 1);
            }

            /**
             * 21.4.6, modifiers:
             */

            basic_string& operator+=(const basic_string& str)
            {
                return append(str);
            }

            basic_string& operator+=(const value_type* str)
            {
                return append(str);
            }

            basic_string& operator+=(value_type c)
            {
                push_back(c);
                return *this;
            }

            basic_string& operator+=(initializer_list<value_type> init)
            {
                return append(init.begin(), init.size());
            }

            basic_string& append(const basic_string& str)
            {
                return append(str.data(), str.size());
            }

            basic_string& append(const basic_string& str, size_type pos,
                                 size_type n = npos)
            {
                if (pos < str.size())
                {
                    auto len = min(n, str.size() - pos);

                    return append(str.data() + pos, len);
                }
                // TODO: Else throw out_of_range.
            }

            basic_string& append(const value_type* str, size_type n)
            {
                // TODO: if (size_ + n > max_size()) throw length_error
                ensure_free_space_(n);
                traits_type::copy(data_ + size(), str, n);
                size_ += n;
                ensure_null_terminator_();

                return *this;
            }

            basic_string& append(const value_type* str)
            {
                return append(str, traits_type::length(str));
            }

            basic_string& append(size_type n, value_type c)
            {
                return append(basic_string(n, c));
            }

            template<class InputIterator>
            basic_string& append(InputIterator first, InputIterator last)
            {
                return append(basic_string(first, last));
            }

            basic_string& append(initializer_list<value_type> init)
            {
                return append(init.begin(), init.size());
            }

            void push_back(value_type c)
            {
                ensure_free_space_(1);
                traits_type::assign(data_[size_++], c);
                ensure_null_terminator_();
            }

            basic_string& assign(const basic_string& str)
            {
                return assign(str, 0, npos);
            }

            basic_string& assign(basic_string&& str)
            {
                swap(str);

                return *this;
            }

            basic_string& assign(const basic_string& str, size_type pos,
                                 size_type n = npos)
            {
                if (pos < str.size())
                {
                    auto len = min(n, str.size() - pos);
                    ensure_free_space_(len);

                    return assign(str.data() + pos, len);
                }
                // TODO: Else throw out_of_range.

                return *this;
            }

            basic_string& assign(const value_type* str, size_type n)
            {
                // TODO: if (n > max_size()) throw length_error.
                resize_without_copy_(n + 1);
                traits_type::copy(begin(), str, n);
                size_ = n;
                ensure_null_terminator_();

                return *this;
            }

            basic_string& assign(const value_type* str)
            {
                return assign(str, traits_type::length(str));
            }

            basic_string& assign(size_type n, value_type c)
            {
                return assign(basic_string(n, c));
            }

            template<class InputIterator>
            basic_string& assign(InputIterator first, InputIterator last)
            {
                return assign(basic_string(first, last));
            }

            basic_string& assign(initializer_list<value_type> init)
            {
                return assign(init.begin(), init.size());
            }

            basic_string& insert(size_type pos, const basic_string& str)
            {
                // TODO: if (pos > str.size()) throw out_of_range.
                return insert(pos, str.data(), str.size());
            }

            basic_string& insert(size_type pos1, const basic_string& str,
                                 size_type pos2, size_type n = npos)
            {
                // TODO: if (pos1 > size() or pos2 > str.size()) throw
                //       out_of_range.
                auto len = min(n, str.size() - pos2);

                return insert(pos1, str.data() + pos2, len);
            }

            basic_string& insert(size_type pos, const value_type* str, size_type n)
            {
                // TODO: throw out_of_range if pos > size()
                // TODO: throw length_error if size() + n > max_size()
                ensure_free_space_(n);

                copy_backward_(begin() + pos, end(), end() + n);
                copy_(str, str + n, begin() + pos);
                size_ += n;

                ensure_null_terminator_();
                return *this;
            }

            basic_string& insert(size_type pos, const value_type* str)
            {
                return insert(pos, str, traits_type::length(str));
            }

            basic_string& insert(size_type pos, size_type n, value_type c)
            {
                return insert(pos, basic_string(n, c));
            }

            iterator insert(const_iterator pos, value_type c)
            {
                auto idx = static_cast<size_type>(pos - begin());

                ensure_free_space_(1);
                copy_backward_(begin() + idx, end(), end() + 1);
                traits_type::assign(data_[idx], c);

                ++size_;
                ensure_null_terminator_();

                return begin() + idx;
            }

            iterator insert(const_iterator pos, size_type n, value_type c)
            {
                if (n == 0)
                    return const_cast<iterator>(pos);

                auto idx = static_cast<size_type>(pos - begin());

                ensure_free_space_(n);
                copy_backward_(begin() + idx, end(), end() + n);

                auto it = begin() + idx;
                for (size_type i = 0; i < n; ++i)
                    traits_type::assign(*it++, c);
                size_ += n;
                ensure_null_terminator_();

                return begin() + idx;
            }

            template<class InputIterator>
            iterator insert(const_iterator pos, InputIterator first,
                            InputIterator last)
            {
                if (first == last)
                    return const_cast<iterator>(pos);

                auto idx = static_cast<size_type>(pos - begin());
                auto str = basic_string{first, last};
                insert(idx, str);

                return begin() + idx;
            }

            iterator insert(const_iterator pos, initializer_list<value_type> init)
            {
                return insert(pos, init.begin(), init.end());
            }

            basic_string& erase(size_type pos = 0, size_type n = npos)
            {
                auto len = min(n, size_ - pos);
                copy_(begin() + pos + n, end(), begin() + pos);
                size_ -= len;
                ensure_null_terminator_();

                return *this;
            }

            iterator erase(const_iterator pos)
            {
                auto idx = static_cast<size_type>(pos - cbegin());
                erase(idx, 1);

                return begin() + idx;
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                auto idx = static_cast<size_type>(first - cbegin());
                auto count = static_cast<size_type>(last - first);
                erase(idx, count);

                return begin() + idx;
            }

            void pop_back()
            {
                --size_;
                ensure_null_terminator_();
            }

            basic_string& replace(size_type pos, size_type n, const basic_string& str)
            {
                // TODO: throw out_of_range if pos > size()
                return replace(pos, n, str.data(), str.size());
            }

            basic_string& replace(size_type pos1, size_type n1, const basic_string& str,
                                  size_type pos2, size_type n2 = npos)
            {
                // TODO: throw out_of_range if pos1 > size() or pos2 > str.size()
                auto len = min(n2, str.size() - pos2);
                return replace(pos1, n1, str.data() + pos2, len);
            }

            basic_string& replace(size_type pos, size_type n1, const value_type* str,
                                  size_type n2)
            {
                // TODO: throw out_of_range if pos > size()
                // TODO: if size() - len > max_size() - n2 throw length_error
                auto len = min(n1, size_ - pos);

                basic_string tmp{};
                tmp.resize_without_copy_(size_ - len + n2);

                // Prefix.
                copy_(begin(), begin() + pos, tmp.begin());

                // Substitution.
                traits_type::copy(tmp.begin() + pos, str, n2);

                // Suffix.
                copy_(begin() + pos + len, end(), tmp.begin() + pos + n2);

                tmp.size_ = size_ - len + n2;
                swap(tmp);
                return *this;
            }

            basic_string& replace(size_type pos, size_type n, const value_type* str)
            {
                return replace(pos, n, str, traits_type::length(str));
            }

            basic_string& replace(size_type pos, size_type n1, size_type n2,
                                  value_type c)
            {
                return replace(pos, n1, basic_string(n2, c));
            }

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  const basic_string& str)
            {
                return replace(i1 - begin(), i2 - i1, str);
            }

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  const value_type* str, size_type n)
            {
                return replace(i1 - begin(), i2 - i1, str, n);
            }

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  const value_type* str)
            {
                return replace(i1 - begin(), i2 - i1, str, traits_type::length(str));
            }

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  size_type n, value_type c)
            {
                return replace(i1 - begin(), i2 - i1, basic_string(n, c));
            }

            template<class InputIterator>
            basic_string& replace(const_iterator i1, const_iterator i2,
                                  InputIterator j1, InputIterator j2)
            {
                return replace(i1 - begin(), i2 - i1, basic_string(j1, j2));
            }

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  initializer_list<value_type> init)
            {
                return replace(i1 - begin(), i2 - i1, init.begin(), init.size());
            }

            size_type copy(value_type* str, size_type n, size_type pos = 0) const
            {
                auto len = min(n , size_ - pos);
                for (size_type i = 0; i < len; ++i)
                    traits_type::assign(str[i], data_[pos + i]);

                return len;
            }

            void swap(basic_string& other)
                noexcept(allocator_traits<allocator_type>::propagate_on_container_swap::value ||
                         allocator_traits<allocator_type>::is_always_equal::value)
            {
                std::swap(data_, other.data_);
                std::swap(size_, other.size_);
                std::swap(capacity_, other.capacity_);
            }

            /**
             * 21.4.7, string operations:
             */

            const value_type* c_str() const noexcept
            {
                return data_;
            }

            const value_type* data() const noexcept
            {
                return data_;
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_type{allocator_};
            }

            /**
             * Note: The following find functions have 4 versions each:
             *       (1) takes basic_string
             *       (2) takes c string and length
             *       (3) takes c string
             *       (4) takes value_type
             *       According to the C++14 standard, only (1) is marked as
             *       noexcept and the other three return the first one with
             *       a newly allocated strings (and thus cannot be noexcept).
             *       However, allocating a new string results in memory
             *       allocation and copying of the source and thus we have
             *       decided to follow C++17 signatures of these functions
             *       (i.e. all of them being marked as noexcept) and use
             *       (2) for the actual implementation (and avoiding any
             *       allocations or copying in the process and also providing
             *       stronger guarantees to the user).
             */

            size_type find(const basic_string& str, size_type pos = 0) const noexcept
            {
                return find(str.c_str(), pos, str.size());
            }

            size_type find(const value_type* str, size_type pos, size_type len) const noexcept
            {
                if (empty() || len == 0 || len + pos > size())
                    return npos;

                size_type idx{pos};

                while (idx + len < size_)
                {
                    if (substr_starts_at_(idx, str, len))
                        return idx;
                    ++idx;
                }

                return npos;
            }

            size_type find(const value_type* str, size_type pos = 0) const noexcept
            {
                return find(str, pos, traits_type::length(str));
            }

            size_type find(value_type c, size_type pos = 0) const noexcept
            {
                if (empty())
                    return npos;

                for (size_type i = pos; i < size_; ++i)
                {
                    if (traits_type::eq(c, data_[i]))
                        return i;
                }

                return npos;
            }

            size_type rfind(const basic_string& str, size_type pos = npos) const noexcept
            {
                return rfind(str.c_str(), pos, str.size());
            }

            size_type rfind(const value_type* str, size_type pos, size_type len) const noexcept
            {
                if (empty() || len == 0 || len + pos > size())
                    return npos;

                size_type idx{min(pos, size_ - 1) + 1};

                while (idx > 0)
                {
                    if (substr_starts_at_(idx - 1, str, len))
                        return idx - 1;
                    --idx;
                }

                return npos;
            }

            size_type rfind(const value_type* str, size_type pos = npos) const noexcept
            {
                return rfind(str, pos, traits_type::length(str));
            }

            size_type rfind(value_type c, size_type pos = npos) const noexcept
            {
                if (empty())
                    return npos;

                for (size_type i = min(pos + 1, size_ - 1) + 1; i > 0; --i)
                {
                    if (traits_type::eq(c, data_[i - 1]))
                        return i - 1;
                }

                return npos;
            }

            size_type find_first_of(const basic_string& str, size_type pos = 0) const noexcept
            {
                return find_first_of(str.c_str(), pos, str.size());
            }

            size_type find_first_of(const value_type* str, size_type pos, size_type len) const noexcept
            {
                if (empty() || len == 0 || pos >= size())
                    return npos;

                size_type idx{pos};

                while (idx < size_)
                {
                    if (is_any_of_(idx, str, len))
                        return idx;
                    ++idx;
                }

                return npos;
            }

            size_type find_first_of(const value_type* str, size_type pos = 0) const noexcept
            {
                return find_first_of(str, pos, traits_type::length(str));
            }

            size_type find_first_of(value_type c, size_type pos = 0) const noexcept
            {
                return find(c, pos);
            }

            size_type find_last_of(const basic_string& str, size_type pos = npos) const noexcept
            {
                return find_last_of(str.c_str(), pos, str.size());
            }

            size_type find_last_of(const value_type* str, size_type pos, size_type len) const noexcept
            {
                if (empty() || len == 0)
                    return npos;

                for (size_type i = min(pos, size_ - 1) + 1; i > 0; --i)
                {
                    if (is_any_of_(i - 1, str, len))
                        return i - 1;
                }

                return npos;
            }

            size_type find_last_of(const value_type* str, size_type pos = npos) const noexcept
            {
                return find_last_of(str, pos, traits_type::length(str));
            }

            size_type find_last_of(value_type c, size_type pos = npos) const noexcept
            {
                return rfind(c, pos);
            }

            size_type find_first_not_of(const basic_string& str, size_type pos = 0) const noexcept
            {
                return find_first_not_of(str.c_str(), pos, str.size());
            }

            size_type find_first_not_of(const value_type* str, size_type pos, size_type len) const noexcept
            {
                if (empty() || pos >= size())
                    return npos;

                size_type idx{pos};

                while (idx < size_)
                {
                    if (!is_any_of_(idx, str, len))
                        return idx;
                    ++idx;
                }

                return npos;
            }

            size_type find_first_not_of(const value_type* str, size_type pos = 0) const noexcept
            {
                return find_first_not_of(str, pos, traits_type::length(str));
            }

            size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept
            {
                if (empty())
                    return npos;

                for (size_type i = pos; i < size_; ++i)
                {
                    if (!traits_type::eq(c, data_[i]))
                        return i;
                }

                return npos;
            }

            size_type find_last_not_of(const basic_string& str, size_type pos = npos) const noexcept
            {
                return find_last_not_of(str.c_str(), pos, str.size());
            }

            size_type find_last_not_of(const value_type* str, size_type pos, size_type len) const noexcept
            {
                if (empty())
                    return npos;

                for (size_type i = min(pos, size_ - 1) + 1; i > 0; --i)
                {
                    if (!is_any_of_(i - 1, str, len))
                        return i - 1;
                }

                return npos;
            }

            size_type find_last_not_of(const value_type* str, size_type pos = npos) const noexcept
            {
                return find_last_not_of(str, pos, traits_type::length(str));
            }

            size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept
            {
                if (empty())
                    return npos;

                pos = min(pos, size_ - 1);

                for (size_type i = min(pos, size_ - 1) + 1; i > 1; --i)
                {
                    if (!traits_type::eq(c, data_[i - 1]))
                        return i - 1;
                }

                return npos;
            }

            basic_string substr(size_type pos = 0, size_type n = npos) const
            {
                // TODO: throw out_of_range if pos > size().
                auto len = min(n, size_ - pos);
                return basic_string{data() + pos, len};
            }

            int compare(const basic_string& other) const noexcept
            {
                auto len = min(size(), other.size());
                auto comp = traits_type::compare(data_, other.data(), len);

                if (comp != 0)
                    return comp;
                else if (size() == other.size())
                    return 0;
                else if (size() > other.size())
                    return 1;
                else
                    return -1;
            }

            int compare(size_type pos, size_type n, const basic_string& other) const
            {
                return basic_string{*this, pos, n}.compare(other);
            }

            int compare(size_type pos1, size_type n1, const basic_string& other,
                        size_type pos2, size_type n2 = npos) const
            {
                return basic_string{*this, pos1, n1}.compare(basic_string{other, pos2, n2});
            }

            int compare(const value_type* other) const
            {
                return compare(basic_string(other));
            }

            int compare(size_type pos, size_type n, const value_type* other) const
            {
                return basic_string{*this, pos, n}.compare(basic_string{other});
            }

            int compare(size_type pos, size_type n1,
                        const value_type* other, size_type n2) const
            {
                return basic_string{*this, pos, n1}.compare(basic_string{other, n2});
            }

        private:
            value_type* data_;
            size_type size_;
            size_type capacity_;
            allocator_type allocator_;

            template<class C, class T, class A>
            friend class basic_stringbuf;

            /**
             * Arbitrary value, standard just requires
             * data() to have some capacity.
             * (Well, we could've done data_ = &data_ and
             * set capacity to 0, but that would be too cryptic.)
             */
            static constexpr size_type default_capacity_{4};

            void init_(const value_type* str, size_type size)
            {
                if (data_)
                    allocator_.deallocate(data_, capacity_);

                size_ = size;
                capacity_ = size + 1;

                data_ = allocator_.allocate(capacity_);
                traits_type::copy(data_, str, size);
                ensure_null_terminator_();
            }

            size_type next_capacity_(size_type hint = 0) const noexcept
            {
                if (hint != 0)
                    return max(capacity_ * 2, hint);
                else
                    return max(capacity_ * 2, size_type{2u});
            }

            void ensure_free_space_(size_type n)
            {
                /**
                 * Note: We cannot use reserve like we
                 *       did in vector, because in string
                 *       reserve can cause shrinking.
                 */
                if (size_ + 1 + n > capacity_)
                    resize_with_copy_(size_, max(size_ + 1 + n, next_capacity_()));
            }

            void resize_without_copy_(size_type capacity)
            {
                if (data_)
                    allocator_.deallocate(data_, capacity_);

                data_ = allocator_.allocate(capacity);
                size_ = 0;
                capacity_ = capacity;
                ensure_null_terminator_();
            }

            void resize_with_copy_(size_type size, size_type capacity)
            {
                if(capacity_ == 0 || capacity_ < capacity)
                {
                    auto new_data = allocator_.allocate(capacity);

                    auto to_copy = min(size, size_);
                    traits_type::move(new_data, data_, to_copy);

                    std::swap(data_, new_data);

                    allocator_.deallocate(new_data, capacity_);
                }

                capacity_ = capacity;
                size_ = size;
                ensure_null_terminator_();
            }

            template<class Iterator1, class Iterator2>
            Iterator2 copy_(Iterator1 first, Iterator1 last,
                            Iterator2 result)
            {
                while (first != last)
                    traits_type::assign(*result++, *first++);

                return result;
            }

            template<class Iterator1, class Iterator2>
            Iterator2 copy_backward_(Iterator1 first, Iterator1 last,
                                     Iterator2 result)
            {
                while (last != first)
                    traits_type::assign(*--result, *--last);

                return result;
            }

            void ensure_null_terminator_()
            {
                value_type c{};
                traits_type::assign(data_[size_], c);
            }

            bool is_any_of_(size_type idx, const value_type* str, size_type len) const
            {
                for (size_type i = 0; i < len; ++i)
                {
                    if (traits_type::eq(data_[idx], str[i]))
                        return true;
                }

                return false;
            }

            bool substr_starts_at_(size_type idx, const value_type* str, size_type len) const
            {
                size_type i{};
                for (i = 0; i < len; ++i)
                {
                    if (!traits_type::eq(data_[idx + i], str[i]))
                        break;
                }

                return i == len;
            }
    };

    using string    = basic_string<char>;
    using u16string = basic_string<char16_t>;
    using u32string = basic_string<char32_t>;
    using wstring   = basic_string<wchar_t>;

    /**
     * 21.4.8, basic_string non-member functions:
     */

    /**
     * 21.4.8.1, operator+:
     */

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(const basic_string<Char, Traits, Allocator>& lhs,
              const basic_string<Char, Traits, Allocator>& rhs)
    {
        return basic_string<Char, Traits, Allocator>{lhs}.append(rhs);
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(basic_string<Char, Traits, Allocator>&& lhs,
              const basic_string<Char, Traits, Allocator>& rhs)
    {
        return move(lhs.append(rhs));
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(const basic_string<Char, Traits, Allocator>& lhs,
              basic_string<Char, Traits, Allocator>&& rhs)
    {
        return move(rhs.insert(0, lhs));
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(basic_string<Char, Traits, Allocator>&& lhs,
              basic_string<Char, Traits, Allocator>&& rhs)
    {
        return move(lhs.append(rhs));
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(const Char* lhs,
              const basic_string<Char, Traits, Allocator>& rhs)
    {
        return basic_string<Char, Traits, Allocator>{lhs} + rhs;
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(const Char* lhs,
              basic_string<Char, Traits, Allocator>&& rhs)
    {
        return move(rhs.insert(0, lhs));
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(Char lhs,
              const basic_string<Char, Traits, Allocator>& rhs)
    {
        return basic_string<Char, Traits, Allocator>{1, lhs}.append(rhs);
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(Char lhs,
              basic_string<Char, Traits, Allocator>&& rhs)
    {
        return move(rhs.insert(0, 1, lhs));
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(const basic_string<Char, Traits, Allocator>& lhs,
              const Char* rhs)
    {
        return lhs + basic_string<Char, Traits, Allocator>{rhs};
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(basic_string<Char, Traits, Allocator>&& lhs,
              const Char* rhs)
    {
        return move(lhs.append(rhs));
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(const basic_string<Char, Traits, Allocator>& lhs,
              Char rhs)
    {
        return lhs + basic_string<Char, Traits, Allocator>{1, rhs};
    }

    template<class Char, class Traits, class Allocator>
    basic_string<Char, Traits, Allocator>
    operator+(basic_string<Char, Traits, Allocator>&& lhs,
              Char rhs)
    {
        return move(lhs.append(1, rhs));
    }

    /**
     * 21.4.8.2, operator==:
     */

    template<class Char, class Traits, class Allocator>
    bool operator==(const basic_string<Char, Traits, Allocator>& lhs,
                    const basic_string<Char, Traits, Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) == 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator==(const Char* lhs,
                    const basic_string<Char, Traits, Allocator>& rhs)
    {
        return rhs == lhs;
    }

    template<class Char, class Traits, class Allocator>
    bool operator==(const basic_string<Char, Traits, Allocator>& lhs,
                    const Char* rhs)
    {
        return lhs.compare(rhs) == 0;
    }

    /**
     * 21.4.8.3, operator!=:
     */

    template<class Char, class Traits, class Allocator>
    bool operator!=(const basic_string<Char, Traits, Allocator>& lhs,
                    const basic_string<Char, Traits, Allocator>& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    template<class Char, class Traits, class Allocator>
    bool operator!=(const Char* lhs,
                    const basic_string<Char, Traits, Allocator>& rhs)
    {
        return rhs != lhs;
    }

    template<class Char, class Traits, class Allocator>
    bool operator!=(const basic_string<Char, Traits, Allocator>& lhs,
                    const Char* rhs)
    {
        return lhs.compare(rhs) != 0;
    }

    /**
     * 21.4.8.4, operator<:
     */

    template<class Char, class Traits, class Allocator>
    bool operator<(const basic_string<Char, Traits, Allocator>& lhs,
                   const basic_string<Char, Traits, Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) < 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator<(const Char* lhs,
                   const basic_string<Char, Traits, Allocator>& rhs)
    {
        return rhs.compare(lhs) > 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator<(const basic_string<Char, Traits, Allocator>& lhs,
                   const Char* rhs)
    {
        return lhs.compare(rhs) < 0;
    }

    /**
     * 21.4.8.5, operator>:
     */

    template<class Char, class Traits, class Allocator>
    bool operator>(const basic_string<Char, Traits, Allocator>& lhs,
                   const basic_string<Char, Traits, Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) > 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator>(const Char* lhs,
                   const basic_string<Char, Traits, Allocator>& rhs)
    {
        return rhs.compare(lhs) < 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator>(const basic_string<Char, Traits, Allocator>& lhs,
                   const Char* rhs)
    {
        return lhs.compare(rhs) > 0;
    }

    /**
     * 21.4.8.6, operator<=:
     */

    template<class Char, class Traits, class Allocator>
    bool operator<=(const basic_string<Char, Traits, Allocator>& lhs,
                    const basic_string<Char, Traits, Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) <= 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator<=(const Char* lhs,
                    const basic_string<Char, Traits, Allocator>& rhs)
    {
        return rhs.compare(lhs) >= 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator<=(const basic_string<Char, Traits, Allocator>& lhs,
                    const Char* rhs)
    {
        return lhs.compare(rhs) <= 0;
    }

    /**
     * 21.4.8.7, operator>=:
     */

    template<class Char, class Traits, class Allocator>
    bool operator>=(const basic_string<Char, Traits, Allocator>& lhs,
                    const basic_string<Char, Traits, Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) >= 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator>=(const Char* lhs,
                    const basic_string<Char, Traits, Allocator>& rhs)
    {
        return rhs.compare(lhs) <= 0;
    }

    template<class Char, class Traits, class Allocator>
    bool operator>=(const basic_string<Char, Traits, Allocator>& lhs,
                    const Char* rhs)
    {
        return lhs.compare(rhs) >= 0;
    }

    /**
     * 21.4.8.8, swap:
     */

    template<class Char, class Traits, class Allocator>
    void swap(basic_string<Char, Traits, Allocator>& lhs,
              basic_string<Char, Traits, Allocator>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    /**
     * 21.5, numeric conversions:
     */

    int stoi(const string& str, size_t* idx = nullptr, int base = 10);
    long stol(const string& str, size_t* idx = nullptr, int base = 10);
    unsigned long stoul(const string& str, size_t* idx = nullptr, int base = 10);
    long long stoll(const string& str, size_t* idx = nullptr, int base = 10);
    unsigned long long stoull(const string& str, size_t* idx = nullptr, int base = 10);

    float stof(const string& str, size_t* idx = nullptr);
    double stod(const string& str, size_t* idx = nullptr);
    long double stold(const string& str, size_t* idx = nullptr);

    string to_string(int val);
    string to_string(unsigned val);
    string to_string(long val);
    string to_string(unsigned long val);
    string to_string(long long val);
    string to_string(unsigned long long val);
    string to_string(float val);
    string to_string(double val);
    string to_string(long double val);

    int stoi(const wstring& str, size_t* idx = nullptr, int base = 10);
    long stol(const wstring& str, size_t* idx = nullptr, int base = 10);
    unsigned long stoul(const wstring& str, size_t* idx = nullptr, int base = 10);
    long long stoll(const wstring& str, size_t* idx = nullptr, int base = 10);
    unsigned long long stoull(const wstring& str, size_t* idx = nullptr, int base = 10);

    float stof(const wstring& str, size_t* idx = nullptr);
    double stod(const wstring& str, size_t* idx = nullptr);
    long double stold(const wstring& str, size_t* idx = nullptr);

    wstring to_wstring(int val);
    wstring to_wstring(unsigned val);
    wstring to_wstring(long val);
    wstring to_wstring(unsigned long val);
    wstring to_wstring(long long val);
    wstring to_wstring(unsigned long long val);
    wstring to_wstring(float val);
    wstring to_wstring(double val);
    wstring to_wstring(long double val);

    /**
     * 21.6, hash support:
     */

    template<class>
    struct hash;

    template<>
    struct hash<string>
    {
        size_t operator()(const string& str) const noexcept
        {
            size_t res{};

            /**
             * Note: No need for fancy algorithms here,
             *       std::hash is used for indexing, not
             *       cryptography.
             */
            for (const auto& c: str)
                res = res * 5 + (res >> 3) + static_cast<size_t>(c);

            return res;
        }

        using argument_type = string;
        using result_type   = size_t;
    };

    template<>
    struct hash<wstring>
    {
        size_t operator()(const wstring& str) const noexcept
        {
            // TODO: implement
            __unimplemented();
            return size_t{};
        }

        using argument_type = wstring;
        using result_type   = size_t;
    };

    // TODO: add those other 2 string types

    /**
     * 21.7, suffix for basic_string literals:
     */

    /**
     * Note: According to the standard, literal suffixes that do not
     *       start with an underscore are reserved for future standardization,
     *       but since we are implementing the standard, we're going to ignore it.
     *       This should work (according to their documentation) work for clang,
     *       but that should be tested.
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"
inline namespace literals {
inline namespace string_literals
{
    string operator "" s(const char* str, size_t len);
    u16string operator "" s(const char16_t* str, size_t len);
    u32string operator "" s(const char32_t* str, size_t len);

    /* Problem: wchar_t == int in HelenOS, but standard forbids it.
    wstring operator "" s(const wchar_t* str, size_t len);
    */
}}
#pragma GCC diagnostic pop
}

#endif
