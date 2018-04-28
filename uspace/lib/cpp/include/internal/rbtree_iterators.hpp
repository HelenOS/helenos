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

#ifndef LIBCPP_INTERNAL_RBTREE_ITERATORS
#define LIBCPP_INTERNAL_RBTREE_ITERATORS

#include <internal/rbtree_node.hpp>
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

    template<class Value, class ConstReference, class ConstPointer, class Size>
    class rbtree_const_iterator
    {
        public:
            using value_type      = Value;
            using size_type       = Size;
            using const_reference = ConstReference;
            using const_pointer   = ConstPointer;
            using difference_type = ptrdiff_t;

            using iterator_category = bidirectional_iterator_tag;

            rbtree_const_iterator(const rbtree_node<value_type>* current = nullptr, bool end = true)
                : current_{current}, end_{end}
            { /* DUMMY BODY */ }

            rbtree_const_iterator(const rbtree_const_iterator&) = default;
            rbtree_const_iterator& operator=(const rbtree_const_iterator&) = default;

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
                if (current_)
                {
                    auto bckp = current_;
                    if (current_->right)
                        current_ = current_->right->find_smallest();
                    else
                    {
                        while (!current_->is_left_child())
                        {
                            current_ = current_->parent;

                            if (!current_->parent)
                            {
                                /**
                                 * We've gone back to root without
                                 * being a left child, which means we
                                 * were the last node.
                                 */
                                end_ = true;
                                current_ = bckp;

                                return *this;
                            }
                        }

                        /**
                         * Now we are a left child,
                         * so the next node we have to visit
                         * is our parent.
                         */
                        current_ = current_->parent;
                    }
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
                    try_undo_end_();

                    return *this;
                }

                if (current_)
                {
                    if (current_->left)
                        current_ = current_->left->find_largest();
                    else if (current_->parent)
                    {
                        while (current_->is_left_child())
                            current_ = current_->parent;

                        /**
                         * We know parent exists here
                         * because we went up from the
                         * left and stopped being left
                         * child (if at any point we happened
                         * to become root then this branch
                         * wouldn't happen).
                         */
                        current_ = current_->parent;
                    }
                    else // We are root without a left child.
                        current_ = nullptr;
                }

                return *this;
            }

            rbtree_const_iterator operator--(int)
            {
                auto tmp = *this;
                --(*this);

                return tmp;
            }

            const rbtree_node<value_type>* node() const
            {
                return current_;
            }

            bool end() const
            {
                return end_;
            }

        private:
            const rbtree_node<value_type>* current_;
            bool end_;

            void try_undo_end_()
            {
                if (!current_)
                    return;

                /**
                 * We can do this if we are past end().
                 * This means we are the largest.
                 */
                if (current_->find_largest() == current_)
                    end_ = false;
            }
    };

    template<class Val, class CRef, class CPtr, class Sz>
    bool operator==(const rbtree_const_iterator<Val, CRef, CPtr, Sz>& lhs,
                    const rbtree_const_iterator<Val, CRef, CPtr, Sz>& rhs)
    {
        return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
    }

    template<class Val, class CRef, class CPtr, class Sz>
    bool operator!=(const rbtree_const_iterator<Val, CRef, CPtr, Sz>& lhs,
                    const rbtree_const_iterator<Val, CRef, CPtr, Sz>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class Reference, class Pointer, class Size>
    class rbtree_iterator
    {
        public:
            using value_type      = Value;
            using size_type       = Size;
            using reference       = Reference;
            using pointer         = Pointer;
            using difference_type = ptrdiff_t;

            using iterator_category = bidirectional_iterator_tag;

            rbtree_iterator(rbtree_node<value_type>* current = nullptr, bool end = true)
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
                if (current_)
                {
                    auto bckp = current_;
                    if (current_->right)
                        current_ = current_->right->find_smallest();
                    else
                    {
                        while (!current_->is_left_child())
                        {
                            current_ = current_->parent;

                            if (!current_ || !current_->parent)
                            {
                                /**
                                 * We've gone back to root without
                                 * being a left child, which means we
                                 * were the last node.
                                 */
                                end_ = true;
                                current_ = bckp;

                                return *this;
                            }
                        }

                        /**
                         * Now we are a left child,
                         * so the next node we have to visit
                         * is our parent.
                         */
                        current_ = current_->parent;
                    }
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
                    try_undo_end_();

                    return *this;
                }

                if (current_)
                {
                    if (current_->left)
                        current_ = current_->left->find_largest();
                    else if (current_->parent)
                    {
                        while (current_->is_left_child())
                            current_ = current_->parent;

                        /**
                         * We know parent exists here
                         * because we went up from the
                         * left and stopped being left
                         * child (if at any point we happened
                         * to become root then this branch
                         * wouldn't happen).
                         */
                        current_ = current_->parent;
                    }
                    else // We are root without a left child.
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

            const rbtree_node<value_type>* node() const
            {
                return current_;
            }

            rbtree_node<value_type>* node()
            {
                return current_;
            }

            bool end() const
            {
                return end_;
            }

        private:
            rbtree_node<value_type>* current_;
            bool end_;

            void try_undo_end_()
            {
                if (!current_)
                    return;

                /**
                 * We can do this if we are past end().
                 * This means we are the largest.
                 */
                if (current_->find_largest() == current_)
                    end_ = false;
            }
    };

    template<class Val, class Ref, class Ptr, class Sz>
    bool operator==(const rbtree_iterator<Val, Ref, Ptr, Sz>& lhs,
                    const rbtree_iterator<Val, Ref, Ptr, Sz>& rhs)
    {
        return (lhs.node() == rhs.node()) && (lhs.end() == rhs.end());
    }

    template<class Val, class Ref, class Ptr, class Sz>
    bool operator!=(const rbtree_iterator<Val, Ref, Ptr, Sz>& lhs,
                    const rbtree_iterator<Val, Ref, Ptr, Sz>& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif
