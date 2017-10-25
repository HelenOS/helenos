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

#ifndef LIBCPP_STRING
#define LIBCPP_STRING

#include <initializer_list>
#include <iosfwd>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

        static void assign(char_type& c1, char_type& c2) noexcept
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
            return std::str_lcmp(s1, s2, n);
        }

        static size_t length(const char_type* s)
        {
            return std::str_size(s);
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
    { /* TODO: implement */ };

    template<>
    struct char_traits<char32_t>
    { /* TODO: implement */ };

    template<>
    struct char_traits<wchar_t>
    {
        using char_type  = wchar_t;
        using int_type   = wint_t;
        using off_type   = streamoff;
        using pos_type   = wstreampos;
        /* using state_type = mbstate_t; */

        static void assign(char_type& c1, char_type& c2) noexcept
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
            return std::wstr_lcmp(s1, s2, n);
        }

        static size_t length(const char_type* s)
        {
            return std::wstr_size(s);
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

    template<class Char, class Traits = char_traits<Char>,
             class Allocator = allocator<Char>>
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
            using pointer         = allocator_traits<allocator_type>::pointer;
            using const_pointer   = allocator_traits<allocator_type>::const_pointer;

            using iterator               = pointer;
            using const_iterator         = const_pointer;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            static constexpr size_type npos = -1;

            /**
             * 21.4.2, construct/copy/destroy:
             */
            basic_string() noexcept
                : basic_string(allocator_type{})
            { /* DUMMY BODY */ }

            explicit basic_string(const allocator_type& alloc);

            basic_string(const basic_string& other);

            basic_string(basic_string&& other);

            basic_string(const basic_string& other, size_type pos, size_type n = npos,
                         const allocator_type& alloc = allocator_type{});

            basic_string(const value_type*, size_type n, const allocator_type& alloc = allocator{});

            basic_string(const value_type*, const allocator_type& alloc = allocator{});

            basic_string(size_type n, value_type c, const allocator_type& alloc = allocator{});

            template<class InputIterator>
            basic_string(InputIterator first, InputIterator last,
                         const allocator_type& alloc = allocator{});

            basic_string(initializer_list<value_type> init, const allocator_type& alloc = allocator{});

            basic_string(const basic_string& other, const allocator_type& alloc);

            basic_string(basic_string&& other, const allocator_type& alloc);

            ~basic_string();

            basic_string& operator=(const basic_string& other);

            basic_string& operator=(basic_string&& other)
                noexcept(allocator_traits<allocator_type>::propagate_on_container_move_assignment::value ||
                         allocator_traits<allocator_type>::is_always_equal::value);

            basic_string& operator=(const value_type* other);

            basic_string& operator=(value_type c);

            basic_string& operator=(initializer_list<value_type>);

            /**
             * 21.4.3, iterators:
             */

            iterator begin() noexcept;

            const_iterator begin() const noexcept;

            iterator end() noexcept;

            const_iterator end() const noexcept;

            reverse_iterator rbegin() noexcept
            {
                return make_reverse_iterator(begin());
            }

            const_reverse_iterator rbegin() const noexcept
            {
                return make_reverse_iterator(cbegin());
            }

            reverse_iterator rend() noexcept
            {
                return make_reverse_iterator(end());
            }

            const_reverse_iterator rend() const noexcept
            {
                return make_reverse_iterator(cend());
            }

            const_iterator cbegin() const noexcept;

            const_iterator cend() const noexcept;

            const_reverse_iterator crbegin() const noexcept
            {
                return make_reverse_iterator(cbegin());
            }

            const_reverse_iterator crend() const noexcept
            {
                return make_reverse_iterator(cend());
            }

            /**
             * 21.4.4, capacity:
             */

            size_type size() const noexcept;

            size_type length() const noexcept;

            size_type max_size() const noexcept;

            void resize(size_type n, value_type c);

            void resize(size_type n);

            size_type capacity() const noexcept;

            void reserve(size_type res_arg = 0);

            void shrink_to_fit();

            void clear() noexcept;

            bool empty() const noexcept;

            /**
             * 21.4.5, element access:
             */

            const_reference operator[](size_type idx) const;

            reference operator[](size_type idx);

            const_reference at(size_type idx) const;

            reference at(size_type idx);

            const_reference front() const;

            reference front();

            const_reference back() const;

            reference back();

            /**
             * 21.4.6, modifiers:
             */

            basic_string& operator+=(const basic_string& str);

            basic_string& operator+=(const value_type* str);

            basic_string& operator+=(value_type c);

            basic_string& operator+=(initializer_list<value_type> init);

            basic_string& append(const basic_string& str);

            basic_string& append(const basic_string& str, size_type pos
                                 size_type n = npos);

            basic_string& append(const value_type* str, size_type n);

            basic_string& append(const value_type* str);

            basic_string& append(size_type n, value_type c);

            template<class InputIterator>
            basic_string& append(InputIterator first, InputIterator last);

            basic_string& append(initializer_list<value_type> init);

            void push_back(value_type c);

            basic_string& assign(const basic_string& str);

            basic_string& assign(basic_string&& str);

            basic_string& assign(const basic_string& str, size_type pos,
                                 size_type n = npos);

            basic_string& assign(const value_type* str, size_type n);

            basic_string& assign(const value_type* str);

            basic_string& assign(size_type n, value_type c);

            template<class InputIterator>
            basic_string& assign(InputIterator first, InputIterator last);

            basic_string& assign(initializer_list<value_type> init);

            basic_string& insert(size_type pos, const basic_string& str);

            basic_string& insert(size_type pos1, const basic_string& str,
                                 size_type pos2, size_type n = npos);

            basic_string& insert(size_type pos, const value_type* str, size_type n);

            basic_string& insert(size_type pos, const value_type* str);

            basic_string& insert(size_type pos, size_type n, value_type c);

            iterator insert(const_iterator pos, value_type c);

            iterator insert(const_iterator pos, size_type n, value_type c);

            template<class InputIterator>
            iterator insert(const_iterator pos, InputIterator first,
                            InputIterator last);

            iterator insert(const_iterator pos, initializer_list<value_type>);

            basic_string& erase(size_type pos = 0; size_type n = npos);

            iterator erase(const_iterator pos);

            iterator erase(const_iterator pos, const_iterator last);

            void pop_back();

            basic_string& replace(size_type pos, size_type n, const basic_string& str);

            basic_string& replace(size_type pos1, size_type n1, const basic_string& str
                                  size_type pos2, size_type n2);

            basic_string& replace(size_type pos, size_type n1, const value_type* str,
                                  size_type n2);

            basic_string& replace(size_type pos, size_type n, const value_type* str);

            basic_string& replace(size_type pos, size_type n1, size_type n2,
                                  value_type c);

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  const basic_string& str);

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  const value_type* str, size_type n);

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  const value_type* str);

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  value_type c);

            template<class InputIterator>
            basic_string& replace(const_iterator i1, const_iterator i2,
                                  InputIterator j1, InputIterator j2);

            basic_string& replace(const_iterator i1, const_iterator i2,
                                  initializer_list<value_type> init);

            size_type copy(value_type* str, size_type n, size_type pos = 0) const;

            void swap(basic_string& other)
                noexcept(allocator_traits<allocator_type>::propagate_on_container_swap::value ||
                         allocator_traits<allocator_type>::is_always_equal);

            /**
             * 21.4.7, string operations:
             */

            const value_type* c_str() const noexcept;

            const value_type* data() const noexcept;

            allocator_type get_allocator() const noexcept;

            size_type find(const basic_string& str, size_type pos = 0) const noexcept;

            size_type find(const value_type* str, size_type pos, size_type n) const;

            size_type find(const value_type* str, size_type pos = 0) const;

            size_type find(value_type c, size_type pos = 0) const;

            size_type rfind(const basic_string& str, size_type pos = npos) const noexcept;

            size_type rfind(const value_type* str, size_type pos, size_type n) const;

            size_type rfind(const value_type* str, size_type pos = npos) const;

            size_type rfind(value_type c, size_type pos = npos) const;

            size_type find_first_of(const basic_string& str, size_type pos = 0) const noexcept;

            size_type find_first_of(const value_type* str, size_type pos, size_type n) const;

            size_type find_first_of(const value_type* str, size_type pos = 0) const;

            size_type find_first_of(value_type c, size_type pos = 0) const;

            size_type find_last_of(const basic_string& str, size_type pos = npos) const noexcept;

            size_type find_last_of(const value_type* str, size_type pos, size_type n) const;

            size_type find_last_of(const value_type* str, size_type pos = npos) const;

            size_type find_last_of(value_type c, size_type pos = npos) const;

            size_type find_first_not_of(const basic_string& str, size_type pos = 0) const noexcept;

            size_type find_first_not_of(const value_type* str, size_type pos, size_type n) const;

            size_type find_first_not_of(const value_type* str, size_type pos = 0) const;

            size_type find_first_not_of(value_type c, size_type pos = 0) const;

            size_type find_last_not_of(const basic_string& str, size_type pos = npos) const noexcept;

            size_type find_last_not_of(const value_type* str, size_type pos, size_type n) const;

            size_type find_last_not_of(const value_type* str, size_type pos = npos) const;

            size_type find_last_not_of(value_type c, size_type pos = npos) const;

            basic_string substr(size_type pos = 0, size_type n = npos) const;

            int compare(const basic_string& other) const noexcept;

            int compare(size_type pos, size_type n, const basic_string& other) const;

            int compare(size_type pos1, size_type n1, const basic_string& other,
                        size_type pos2, size_type n2 = npos) const;

            int compare(const value_type* other) const;

            int compare(size_type pos, size_type n, const value_type* other) const;

            int compare(size_type pos1, size_type n1,
                        const value_type* other, size_type n2) const;
    };
}

#endif
