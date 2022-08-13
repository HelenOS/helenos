/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
