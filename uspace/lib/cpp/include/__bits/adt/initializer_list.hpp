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

#ifndef LIBCPP_BITS_ADT_INITIALIZER_LIST
#define LIBCPP_BITS_ADT_INITIALIZER_LIST

#include <cstdlib>

namespace std
{
    /**
     * 18.9, initializer lists:
     */

    template<class T>
    class initializer_list
    {
        public:
            using value_type      = T;
            using const_reference = const T&;
            using reference       = const_reference;
            using size_type       = size_t;
            using const_iterator  = const T*;
            using iterator        = const_iterator;

            constexpr initializer_list() noexcept
                : begin_{nullptr}, size_{0}
            { /* DUMMY BODY */ }

            constexpr size_type size() const noexcept
            {
                return size_;
            }

            constexpr iterator begin() const noexcept
            {
                return begin_;
            }

            constexpr iterator end() const noexcept
            {
                return begin_ + size_;
            }

        private:
            iterator begin_;
            size_type size_;

            constexpr initializer_list(iterator begin, size_type size)
                : begin_{begin}, size_{size}
            { /* DUMMY BODY */ }
    };

    /**
     * 18.9.3, initializer list range access:
     */

    template<class T>
    constexpr auto begin(initializer_list<T> init)
    {
        return init.begin();
    }

    template<class T>
    constexpr auto end(initializer_list<T> init)
    {
        return init.end();
    }
}

#endif
