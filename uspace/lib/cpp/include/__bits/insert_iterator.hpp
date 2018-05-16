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

#ifndef LIBCPP_BITS_INSERT_ITERATOR
#define LIBCPP_BITS_INSERT_ITERATOR

namespace std
{
    struct forward_iterator_tag;
}

namespace std::aux
{
    /**
     * Note: Commonly the data structures in this library
     *       contain a set of insert member functions. These
     *       are often both iterator based and copy based.
     *       This iterator can change the copy version into
     *       an iterator version by creating a count based copy
     *       iterator.
     * Usage:
     *      first == insert_iterator{value}
     *      last  == insert_iterator{count}
     *
     *      So the following code:
     *          while (first != last)
     *              *data_++ = *first++;
     *
     *      Will insert a copy of value into the data_
     *      iterator exactly count times.
     * TODO: Apply this to existing containers?
     */
    template<class T>
    class insert_iterator
    {
        public:
            using difference_type   = unsigned long long;
            using value_type        = T;
            using iterator_category = input_iterator_tag;
            using reference         = value_type&;
            using pointer           = value_type*;

            explicit insert_iterator(difference_type count, const value_type& val = value_type{})
                : value_{val}, count_{count}
            { /* DUMMY BODY */ }

            insert_iterator(const insert_iterator&) = default;
            insert_iterator& operator=(const insert_iterator&) = default;

            insert_iterator(insert_iterator&&) = default;
            insert_iterator& operator=(insert_iterator&&) = default;

            const value_type& operator*() const
            {
                return value_;
            }

            const value_type* operator->() const
            {
                return &value_;
            }

            insert_iterator& operator++()
            {
                ++count_;

                return *this;
            }

            insert_iterator operator++(int)
            {
                ++count_;

                return insert_iterator{count_ - 1, value_};
            }

            bool operator==(const insert_iterator& other)
            {
                return count_ == other.count_;
            }

            bool operator!=(const insert_iterator& other)
            {
                return count_ != other.count_;
            }

        private:
            value_type value_;
            difference_type count_;
    };
}

#endif
