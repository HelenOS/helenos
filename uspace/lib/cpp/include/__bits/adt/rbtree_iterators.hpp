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

#ifndef LIBCPP_BITS_ADT_RBTREE_ITERATORS
#define LIBCPP_BITS_ADT_RBTREE_ITERATORS

#include <__bits/adt/rbtree_node.hpp>
#include <__bits/iterator_helpers.hpp>
#include <iterator>

namespace std::aux
{
    /**
     * Note: In order for these iterators to be reversible,
     *       the end state of an iterator is represented by a flag
     *       which can be set from true to false in operator--
     *       (i.e. decrementing end iterator) or set from false to
     *       true in operator++ (i.e. incrementing last before end
     *       iterator).
     */

    template<class Value, class Reference, class Pointer, class Size, class Node>
    class rbtree_iterator
    {
        public:
            using value_type      = Value;
            using size_type       = Size;
            using reference       = Reference;
            using pointer         = Pointer;
            using difference_type = ptrdiff_t;

            using iterator_category = bidirectional_iterator_tag;

            using node_type = Node;

            rbtree_iterator(node_type* current = nullptr, bool end = true)
                : current_{current}, end_{end}
            { /* DUMMY BODY */ }

            rbtree_iterator(const rbtree_iterator&) = default;
            rbtree_iterator& operator=(const rbtree_iterator&) = default;

            reference operator*() const
            {
                return current_->value;
            }

            pointer operator->() const
            {
                return &current_->value;
            }

            rbtree_iterator& operator++()
            {
                if (end_)
                    return *this;

                if (current_)
                {
                    auto next = current_->successor();
                    if (next)
                        current_ = next;
                    else
                        end_ = true;
                }

                return *this;
            }

            rbtree_iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);

                return tmp;
            }

            rbtree_iterator& operator--()
            {
                if (end_)
                {
                    end_ = false;

                    return *this;
                }

                if (current_)
                {
                    auto next = current_->predecessor();
                    if (next)
                        current_ = next;
                    else
                        end_ = true;
                }

                return *this;
            }

            rbtree_iterator operator--(int)
            {
                auto tmp = *this;
                --(*this);

                return tmp;
            }

            const node_type* node() const
            {
                return current_;
            }

            node_type* node()
            {
                return current_;
            }

            bool end() const
            {
                return end_;
            }

        private:
            node_type* current_;
            bool end_;
    };

    template<class Val, class Ref, class Ptr, class Sz, class N>
    bool operator==(const rbtree_iterator<Val, Ref, Ptr, Sz, N>& lhs,
                    const rbtree_iterator<Val, Ref, Ptr, Sz, N>& rhs)
    {
        return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
    }

    template<class Val, class Ref, class Ptr, class Sz, class N>
    bool operator!=(const rbtree_iterator<Val, Ref, Ptr, Sz, N>& lhs,
                    const rbtree_iterator<Val, Ref, Ptr, Sz, N>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class ConstReference, class ConstPointer, class Size, class Node>
    class rbtree_const_iterator
    {
        using non_const_iterator_type = rbtree_iterator<
            Value, get_non_const_ref_t<ConstReference>,
            get_non_const_ptr_t<ConstPointer>, Size, Node
        >;

        public:
            using value_type      = Value;
            using size_type       = Size;
            using const_reference = ConstReference;
            using const_pointer   = ConstPointer;
            using difference_type = ptrdiff_t;

            using iterator_category = bidirectional_iterator_tag;

            // For iterator_traits.
            using reference = ConstReference;
            using pointer   = ConstPointer;

            using node_type = Node;

            rbtree_const_iterator(const node_type* current = nullptr, bool end = true)
                : current_{current}, end_{end}
            { /* DUMMY BODY */ }

            rbtree_const_iterator(const rbtree_const_iterator&) = default;
            rbtree_const_iterator& operator=(const rbtree_const_iterator&) = default;

            rbtree_const_iterator(const non_const_iterator_type& other)
                : current_{other.node()}, end_{other.end()}
            { /* DUMMY BODY */ }

            rbtree_const_iterator& operator=(const non_const_iterator_type& other)
            {
                current_ = other.node();
                end_ = other.end();

                return *this;
            }

            const_reference operator*() const
            {
                return current_->value;
            }

            const_pointer operator->() const
            {
                return &current_->value;
            }

            rbtree_const_iterator& operator++()
            {
                if (end_)
                    return *this;

                if (current_)
                {
                    auto next = current_->successor();
                    if (next)
                        current_ = next;
                    else
                        end_ = true;
                }

                return *this;
            }

            rbtree_const_iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);

                return tmp;
            }

            rbtree_const_iterator& operator--()
            {
                if (end_)
                {
                    end_ = false;

                    return *this;
                }

                if (current_)
                {
                    auto next = current_->predecessor();
                    if (next)
                        current_ = next;
                    else
                        end_ = true;
                }

                return *this;
            }

            rbtree_const_iterator operator--(int)
            {
                auto tmp = *this;
                --(*this);

                return tmp;
            }

            const node_type* node() const
            {
                return current_;
            }

            bool end() const
            {
                return end_;
            }

        private:
            const node_type* current_;
            bool end_;
    };

    template<class Val, class CRef, class CPtr, class Sz, class N>
    bool operator==(const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& lhs,
                    const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& rhs)
    {
        return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
    }

    template<class Val, class CRef, class CPtr, class Sz, class N>
    bool operator!=(const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& lhs,
                    const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Val, class Ref, class Ptr, class CRef, class CPtr, class Sz, class N>
    bool operator==(const rbtree_iterator<Val, Ref, Ptr, Sz, N>& lhs,
                    const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& rhs)
    {
        return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
    }

    template<class Val, class Ref, class Ptr, class CRef, class CPtr, class Sz, class N>
    bool operator!=(const rbtree_iterator<Val, Ref, Ptr, Sz, N>& lhs,
                    const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Val, class CRef, class CPtr, class Ref, class Ptr, class Sz, class N>
    bool operator==(const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& lhs,
                    const rbtree_iterator<Val, Ref, Ptr, Sz, N>& rhs)
    {
        return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
    }

    template<class Val, class CRef, class CPtr, class Ref, class Ptr, class Sz, class N>
    bool operator!=(const rbtree_const_iterator<Val, CRef, CPtr, Sz, N>& lhs,
                    const rbtree_iterator<Val, Ref, Ptr, Sz, N>& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif
