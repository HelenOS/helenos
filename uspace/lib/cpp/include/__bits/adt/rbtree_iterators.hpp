/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
