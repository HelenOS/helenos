/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ADT_HASH_TABLE_ITERATORS
#define LIBCPP_BITS_ADT_HASH_TABLE_ITERATORS

#include <__bits/adt/list_node.hpp>
#include <__bits/adt/hash_table_bucket.hpp>
#include <__bits/iterator_helpers.hpp>
#include <iterator>

namespace std::aux
{
    template<class Value, class Reference, class Pointer, class Size>
    class hash_table_iterator
    {
        public:
            using value_type      = Value;
            using size_type       = Size;
            using reference       = Reference;
            using pointer         = Pointer;
            using difference_type = ptrdiff_t;

            using iterator_category = forward_iterator_tag;

            hash_table_iterator(hash_table_bucket<value_type, size_type>* table = nullptr,
                                size_type idx = size_type{}, size_type max_idx = size_type{},
                                list_node<value_type>* current = nullptr)
                : table_{table}, idx_{idx}, max_idx_{max_idx}, current_{current}
            { /* DUMMY BODY */ }

            hash_table_iterator(const hash_table_iterator&) = default;
            hash_table_iterator& operator=(const hash_table_iterator&) = default;

            reference operator*()
            {
                return current_->value;
            }

            pointer operator->()
            {
                return &current_->value;
            }

            hash_table_iterator& operator++()
            {
                current_ = current_->next;
                if (current_ == table_[idx_].head)
                {
                    if (idx_ < max_idx_)
                    {
                        while (!table_[++idx_].head && idx_ < max_idx_)
                        { /* DUMMY BODY */ }

                        if (idx_ < max_idx_)
                            current_ = table_[idx_].head;
                        else
                            current_ = nullptr;
                    }
                    else
                        current_ = nullptr;
                }

                return *this;
            }

            hash_table_iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);

                return tmp;
            }

            list_node<value_type>* node()
            {
                return current_;
            }

            const list_node<value_type>* node() const
            {
                return current_;
            }

            size_type idx() const
            {
                return idx_;
            }

        private:
            hash_table_bucket<value_type, size_type>* table_;
            size_type idx_;
            size_type max_idx_;
            list_node<value_type>* current_;

            template<class V, class CR, class CP, class S>
            friend class hash_table_const_iterator;
    };

    template<class Value, class Ref, class Ptr, class Size>
    bool operator==(const hash_table_iterator<Value, Ref, Ptr, Size>& lhs,
                    const hash_table_iterator<Value, Ref, Ptr, Size>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class Ref, class Ptr, class Size>
    bool operator!=(const hash_table_iterator<Value, Ref, Ptr, Size>& lhs,
                    const hash_table_iterator<Value, Ref, Ptr, Size>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class ConstReference, class ConstPointer, class Size>
    class hash_table_const_iterator
    {
        using non_const_iterator_type = hash_table_iterator<
            Value, get_non_const_ref_t<ConstReference>,
            get_non_const_ptr_t<ConstPointer>, Size
        >;

        public:
            using value_type      = Value;
            using size_type       = Size;
            using const_reference = ConstReference;
            using const_pointer   = ConstPointer;
            using difference_type = ptrdiff_t;

            using iterator_category = forward_iterator_tag;

            hash_table_const_iterator(const hash_table_bucket<value_type, size_type>* table = nullptr,
                                      size_type idx = size_type{}, size_type max_idx = size_type{},
                                      const list_node<value_type>* current = nullptr)
                : table_{table}, idx_{idx}, max_idx_{max_idx}, current_{current}
            { /* DUMMY BODY */ }

            hash_table_const_iterator(const hash_table_const_iterator&) = default;
            hash_table_const_iterator& operator=(const hash_table_const_iterator&) = default;

            hash_table_const_iterator(const non_const_iterator_type& other)
                : table_{other.table_}, idx_{other.idx_}, max_idx_{other.max_idx_},
                  current_{other.current_}
            { /* DUMMY BODY */ }

            hash_table_const_iterator& operator=(const non_const_iterator_type& other)
            {
                table_ = other.table_;
                idx_ = other.idx_;
                max_idx_ = other.max_idx_;
                current_ = other.current_;

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

            hash_table_const_iterator& operator++()
            {
                current_ = current_->next;
                if (current_ == table_[idx_].head)
                {
                    if (idx_ < max_idx_)
                    {
                        while (!table_[++idx_].head && idx_ < max_idx_)
                        { /* DUMMY BODY */ }

                        if (idx_ < max_idx_)
                            current_ = table_[idx_].head;
                        else
                            current_ = nullptr;
                    }
                    else
                        current_ = nullptr;
                }

                return *this;
            }

            hash_table_const_iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);

                return tmp;
            }

            list_node<value_type>* node()
            {
                return const_cast<list_node<value_type>*>(current_);
            }

            const list_node<value_type>* node() const
            {
                return current_;
            }

            size_type idx() const
            {
                return idx_;
            }

        private:
            const hash_table_bucket<value_type, size_type>* table_;
            size_type idx_;
            size_type max_idx_;
            const list_node<value_type>* current_;
    };

    template<class Value, class CRef, class CPtr, class Size>
    bool operator==(const hash_table_const_iterator<Value, CRef, CPtr, Size>& lhs,
                    const hash_table_const_iterator<Value, CRef, CPtr, Size>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class CRef, class CPtr, class Size>
    bool operator!=(const hash_table_const_iterator<Value, CRef, CPtr, Size>& lhs,
                    const hash_table_const_iterator<Value, CRef, CPtr, Size>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class Ref, class Ptr, class CRef, class CPtr, class Size>
    bool operator==(const hash_table_iterator<Value, Ref, Ptr, Size>& lhs,
                    const hash_table_const_iterator<Value, CRef, CPtr, Size>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class Ref, class Ptr, class CRef, class CPtr, class Size>
    bool operator!=(const hash_table_iterator<Value, Ref, Ptr, Size>& lhs,
                    const hash_table_const_iterator<Value, CRef, CPtr, Size>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class CRef, class CPtr, class Ref, class Ptr, class Size>
    bool operator==(const hash_table_const_iterator<Value, CRef, CPtr, Size>& lhs,
                    const hash_table_iterator<Value, Ref, Ptr, Size>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class CRef, class CPtr, class Ref, class Ptr, class Size>
    bool operator!=(const hash_table_const_iterator<Value, CRef, CPtr, Size>& lhs,
                    const hash_table_iterator<Value, Ref, Ptr, Size>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class Reference, class Pointer>
    class hash_table_local_iterator
    {
        public:
            using value_type      = Value;
            using reference       = Reference;
            using pointer         = Pointer;
            using difference_type = ptrdiff_t;

            using iterator_category = forward_iterator_tag;

            hash_table_local_iterator(list_node<value_type>* head = nullptr,
                                      list_node<value_type>* current = nullptr)
                : head_{head}, current_{current}
            { /* DUMMY BODY */ }

            hash_table_local_iterator(const hash_table_local_iterator&) = default;
            hash_table_local_iterator& operator=(const hash_table_local_iterator&) = default;

            reference operator*()
            {
                return current_->value;
            }

            pointer operator->()
            {
                return &current_->value;
            }

            hash_table_local_iterator& operator++()
            {
                current_ = current_->next;
                if (current_ == head_)
                    current_ = nullptr;

                return *this;
            }

            hash_table_local_iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);

                return tmp;
            }

            list_node<value_type>* node()
            {
                return current_;
            }

            const list_node<value_type>* node() const
            {
                return current_;
            }

        private:
            list_node<value_type>* head_;
            list_node<value_type>* current_;

            template<class V, class CR, class CP>
            friend class hash_table_const_local_iterator;
    };

    template<class Value, class Ref, class Ptr>
    bool operator==(const hash_table_local_iterator<Value, Ref, Ptr>& lhs,
                    const hash_table_local_iterator<Value, Ref, Ptr>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class Ref, class Ptr>
    bool operator!=(const hash_table_local_iterator<Value, Ref, Ptr>& lhs,
                    const hash_table_local_iterator<Value, Ref, Ptr>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class ConstReference, class ConstPointer>
    class hash_table_const_local_iterator
    {
        using non_const_iterator_type = hash_table_local_iterator<
            Value, get_non_const_ref_t<ConstReference>,
            get_non_const_ptr_t<ConstPointer>
        >;

        public:
            using value_type      = Value;
            using const_reference = ConstReference;
            using const_pointer   = ConstPointer;
            using difference_type = ptrdiff_t;

            using iterator_category = forward_iterator_tag;

            // TODO: requirement for forward iterator is default constructibility, fix others!
            hash_table_const_local_iterator(const list_node<value_type>* head = nullptr,
                                            const list_node<value_type>* current = nullptr)
                : head_{head}, current_{current}
            { /* DUMMY BODY */ }

            hash_table_const_local_iterator(const hash_table_const_local_iterator&) = default;
            hash_table_const_local_iterator& operator=(const hash_table_const_local_iterator&) = default;

            hash_table_const_local_iterator(const non_const_iterator_type& other)
                : head_{other.head_}, current_{other.current_}
            { /* DUMMY BODY */ }

            hash_table_const_local_iterator& operator=(const non_const_iterator_type& other)
            {
                head_ = other.head_;
                current_ = other.current_;

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

            hash_table_const_local_iterator& operator++()
            {
                current_ = current_->next;
                if (current_ == head_)
                    current_ = nullptr;

                return *this;
            }

            hash_table_const_local_iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);

                return tmp;
            }


            list_node<value_type>* node()
            {
                return const_cast<list_node<value_type>*>(current_);
            }

            const list_node<value_type>* node() const
            {
                return current_;
            }

        private:
            const list_node<value_type>* head_;
            const list_node<value_type>* current_;
    };

    template<class Value, class CRef, class CPtr>
    bool operator==(const hash_table_const_local_iterator<Value, CRef, CPtr>& lhs,
                    const hash_table_const_local_iterator<Value, CRef, CPtr>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class CRef, class CPtr>
    bool operator!=(const hash_table_const_local_iterator<Value, CRef, CPtr>& lhs,
                    const hash_table_const_local_iterator<Value, CRef, CPtr>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class Ref, class Ptr, class CRef, class CPtr>
    bool operator==(const hash_table_local_iterator<Value, Ref, Ptr>& lhs,
                    const hash_table_const_local_iterator<Value, CRef, CPtr>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class Ref, class Ptr, class CRef, class CPtr>
    bool operator!=(const hash_table_local_iterator<Value, Ref, Ptr>& lhs,
                    const hash_table_const_local_iterator<Value, CRef, CPtr>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Value, class CRef, class CPtr, class Ref, class Ptr>
    bool operator==(const hash_table_const_local_iterator<Value, CRef, CPtr>& lhs,
                    const hash_table_local_iterator<Value, Ref, Ptr>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class CRef, class CPtr, class Ref, class Ptr>
    bool operator!=(const hash_table_const_local_iterator<Value, CRef, CPtr>& lhs,
                    const hash_table_local_iterator<Value, Ref, Ptr>& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif
