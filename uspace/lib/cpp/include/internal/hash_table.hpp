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

#ifndef LIBCPP_INTERNAL_HASH_TABLE
#define LIBCPP_INTERNAL_HASH_TABLE

#include <cstdlib>
#include <internal/list.hpp>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>

namespace std::aux
{
    /**
     * To save code, we're going to implement one hash table
     * for both unordered_map and unordered_set. To do this,
     * we create one inner hash table that is oblivious to its
     * held type (and uses a key extractor to get the key from it)
     * and two proxies that either use plain Key type or a pair
     * of a key and a value.
     * Note: I am aware the naming is very unimaginative here,
     *       not my strong side :)
     */

    template<class Key, class Value>
    struct key_value_key_extractor
    {
        const Key& operator()(const pair<const Key, Value>& p) const noexcept
        {
            return p.first;
        }
    };

    template<class Key>
    struct key_no_value_key_extractor
    {
        Key& operator()(Key& k) const noexcept
        {
            return k;
        }

        const Key& operator()(const Key& k) const noexcept
        {
            return k;
        }
    };

    template<class Value, class Size>
    struct hash_table_bucket
    {
        /**
         * Note: We use a doubly linked list because
         *       we need to use hints, which point to the
         *       element after the hinted spot.
         */

        list_node<Value>* head;

        hash_table_bucket()
            : head{}
        { /* DUMMY BODY */ }

        Size size() const noexcept
        {
            auto current = head;
            Size res{};

            do
            {
                ++res;
                current = current->next;
            }
            while (current != head);

            return res;
        }

        void append(list_node<Value>* node)
        {
            if (!head)
                head = node;
            else
                head->append(node);
        }

        void prepend(list_node<Value>* node)
        {
            if (!head)
                head = node;
            else
                head->prepend(node);
        }

        void clear()
        {
            if (!head)
                return;

            auto current = head;
            do
            {
                auto tmp = current;
                current = current->next;
                delete tmp;
            }
            while (current != head);

            head = nullptr;
        }

        ~hash_table_bucket()
        {
            clear();
        }
    };

    struct hash_single_policy
    {
        template<class Table, class Key>
        static typename Table::size_type count(const Table& table, const Key& key)
        {
            return table.find(key) == table.end() ? 0 : 1;
        }

        template<class Table, class Key>
        static typename Table::hint_type find_insertion_spot(const Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            return make_tuple(
                &table.table_[idx],
                table.table_[idx].head,
                idx
            );
        }

        template<class Table, class Key>
        static typename Table::size_type erase(Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            auto head = table.table_[idx].head;
            auto current = head;

            do
            {
                if (table.keys_equal(key, current->value))
                {
                    --table.size_;

                    if (current == head)
                    {
                        if (current->next != head)
                            table.table_[idx].head = current->next;
                        else
                            table.table_[idx].head = nullptr;
                    }

                    current->unlink();
                    delete current;

                    return 1;
                }
                else
                    current = current->next;
            }
            while (current != head);

            return 0;
        }

        template<class Table, class Key>
        static pair<
            typename Table::iterator,
            typename Table::iterator
        > equal_range(Table& table, const Key& key)
        {
            auto it = table.find(key);
            return make_pair(it, ++it);
        }

        template<class Table, class Key>
        static pair<
            typename Table::const_iterator,
            typename Table::const_iterator
        > equal_range_const(const Table& table, const Key& key)
        { // Note: We cannot overload by return type, so we use a different name.
            auto it = table.find(key);
            return make_pair(it, ++it);
        }
    };

    struct hash_multi_policy
    {
        template<class Table, class Key>
        static typename Table::size_type count(const Table& table, const Key& key)
        {
            auto head = table.table_[get_bucket_idx_(key)].head;
            if (!head)
                return 0;

            auto current = head;
            typename Table::size_type res = 0;
            do
            {
                if (table.keys_equal(key, current->value))
                    ++res;

                current = current->next;
            }
            while (current != head);

            return res;
        }

        template<class Table, class Key>
        static typename Table::hint_type find_insertion_spot(const Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            auto head = table.table_[idx].head;

            if (head)
            {
                auto current = head;
                do
                {
                    if (table.keys_equal(key, current->value))
                    {
                        return make_tuple(
                            &table.table_[idx],
                            current,
                            idx
                        );
                    }

                    current = current->next;
                } while (current != head);
            }

            return make_tuple(
                &table.table_[idx],
                table.table_[idx].head,
                idx
            );
        }

        template<class Table, class Key>
        static typename Table::size_type erase(Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            auto it = table.begin(it);
            typename Table::size_type res{};

            while (it != table.end(it))
            {
                if (table.keys_equal(key, *it))
                {
                    while (table.keys_equal(key, *it))
                    {
                        auto node = it.node();
                        ++it;
                        ++res;

                        node.unlink();
                        delete node;
                        --table.size_;
                    }

                    // Elements with equal keys are next to each other.
                    return res;
                }

                ++it;
            }

            return res;
        }

        template<class Table, class Key>
        static pair<
            typename Table::iterator,
            typename Table::iterator
        > equal_range(Table& table, const Key& key)
        {
            auto first = table.find(key);
            if (first == table.end())
                return make_pair(table.end(), table.end());

            auto last = first;
            do
            {
                ++last;
            } while (table.keys_equal(key, *last));

            return make_pair(first, last);
        }

        template<class Table, class Key>
        static pair<
            typename Table::const_iterator,
            typename Table::const_iterator
        > equal_range_const(const Table& table, const Key& key)
        {
            auto first = table.find(key);
            if (first == table.end())
                return make_pair(table.end(), table.end());

            auto last = first;
            do
            {
                ++last;
            } while (table.keys_equal(key, *last));

            return make_pair(first, last);
        }
    };

    template<class Value, class ConstReference, class ConstPointer, class Size>
    class hash_table_const_iterator
    {
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
                        while (!table_[++idx_].head)
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
                auto tmp_current = current_;
                auto tmp_idx = idx_;

                current_ = current_->next;
                if (current_ == table_[idx_].head)
                {
                    if (idx_ < max_idx_)
                    {
                        while (!table_[++idx_].head)
                        { /* DUMMY BODY */ }

                        if (idx_ < max_idx_)
                            current_ = table_[idx_].head;
                        else
                            current_ = nullptr;
                    }
                    else
                        current_ = nullptr;
                }

                return hash_table_const_iterator{
                    table_, tmp_idx, max_idx_, tmp_current
                };
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

    template<class Value, class ConstRef, class ConstPtr, class Size>
    bool operator==(const hash_table_const_iterator<Value, ConstRef, ConstPtr, Size>& lhs,
                    const hash_table_const_iterator<Value, ConstRef, ConstPtr, Size>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class ConstRef, class ConstPtr, class Size>
    bool operator!=(const hash_table_const_iterator<Value, ConstRef, ConstPtr, Size>& lhs,
                    const hash_table_const_iterator<Value, ConstRef, ConstPtr, Size>& rhs)
    {
        return !(lhs == rhs);
    }

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
                        while (!table_[++idx_].head)
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
                auto tmp_current = current_;
                auto tmp_idx = idx_;

                current_ = current_->next;
                if (current_ == table_[idx_].head)
                {
                    if (idx_ < max_idx_)
                    {
                        while (!table_[++idx_].head)
                        { /* DUMMY BODY */ }

                        if (idx_ < max_idx_)
                            current_ = table_[idx_].head;
                        else
                            current_ = nullptr;
                    }
                    else
                        current_ = nullptr;
                }

                return hash_table_iterator{
                    table_, tmp_idx, max_idx_, tmp_current
                };
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

            template<class ConstRef, class ConstPtr>
            operator hash_table_const_iterator<
                Value, ConstRef, ConstPtr, Size
            >() const
            {
                return hash_table_const_iterator<value_type, ConstRef, ConstPtr, size_type>{
                    table_, idx_, max_idx_, current_
                };
            }

        private:
            hash_table_bucket<value_type, size_type>* table_;
            size_type idx_;
            size_type max_idx_;
            list_node<value_type>* current_;
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

    template<class Value, class ConstReference, class ConstPointer>
    class hash_table_const_local_iterator
    {
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
                auto tmp = current_;
                current_ = current_->next;
                if (current_ == head_)
                    current_ = nullptr;

                return hash_table_const_local_iterator{head_, tmp};
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

    template<class Value, class ConstRef, class ConstPtr>
    bool operator==(const hash_table_const_local_iterator<Value, ConstRef, ConstPtr>& lhs,
                    const hash_table_const_local_iterator<Value, ConstRef, ConstPtr>& rhs)
    {
        return lhs.node() == rhs.node();
    }

    template<class Value, class ConstRef, class ConstPtr>
    bool operator!=(const hash_table_const_local_iterator<Value, ConstRef, ConstPtr>& lhs,
                    const hash_table_const_local_iterator<Value, ConstRef, ConstPtr>& rhs)
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
                auto tmp = current_;
                current_ = current_->next;
                if (current_ == head_)
                    current_ = nullptr;

                return hash_table_local_iterator{head_, tmp};
            }

            list_node<value_type>* node()
            {
                return current_;
            }

            const list_node<value_type>* node() const
            {
                return current_;
            }

            template<class ConstRef, class ConstPtr>
            operator hash_table_const_local_iterator<
                Value, ConstRef, ConstPtr
            >() const
            {
                return hash_table_const_local_iterator<
                    value_type, ConstRef, ConstPtr
                >{head_, current_};
            }

        private:
            list_node<value_type>* head_;
            list_node<value_type>* current_;
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

    template<
        class Value, class Key, class KeyExtractor,
        class Hasher, class KeyEq,
        class Alloc, class Size,
        class Iterator, class ConstIterator,
        class LocalIterator, class ConstLocalIterator,
        class Policy
    >
    class hash_table
    {
        public:
            using value_type     = Value;
            using key_type       = Key;
            using size_type      = Size;
            using allocator_type = Alloc;
            using key_equal      = KeyEq;
            using hasher         = Hasher;
            using key_extract    = KeyExtractor;

            using iterator             = Iterator;
            using const_iterator       = ConstIterator;
            using local_iterator       = LocalIterator;
            using const_local_iterator = ConstLocalIterator;

            using node_type = list_node<value_type>;

            using hint_type = tuple<
                hash_table_bucket<value_type, size_type>*,
                list_node<value_type>*, size_type
            >;

            hash_table(size_type buckets, float max_load_factor = 1.f)
                : table_{new hash_table_bucket<value_type, size_type>[buckets]()},
                  bucket_count_{buckets}, size_{}, hasher_{}, key_eq_{},
                  key_extractor_{}, max_load_factor_{max_load_factor}
            { /* DUMMY BODY */ }

            hash_table(size_type buckets, const hasher& hf, const key_equal& eql,
                       float max_load_factor = 1.f)
                : table_{new hash_table_bucket<value_type, size_type>[buckets]()},
                  bucket_count_{buckets}, size_{}, hasher_{hf}, key_eq_{eql},
                  key_extractor_{}, max_load_factor_{max_load_factor}
            { /* DUMMY BODY */ }

            hash_table(const hash_table& other)
                : hash_table{other.bucket_count_, other.hasher_, other.key_eq_,
                             other.max_load_factor_}
            {
                for (const auto& x: other)
                {
                    auto spot = find_insertion_spot(key_extractor_(x));
                    insert(spot, x);
                }
            }

            hash_table(hash_table&& other)
                : table_{other.table_}, bucket_count_{other.bucket_count_},
                  size_{other.size_}, hasher_{move(other.hasher_)},
                  key_eq_{move(other.key_eq_)}, key_extractor_{move(other.key_extractor_)},
                  max_load_factor_{other.max_load_factor_}
            {
                other.table_ = nullptr;
                other.bucket_count_ = size_type{};
                other.size_ = size_type{};
                other.max_load_factor_ = 1.f;
            }

            hash_table& operator=(const hash_table& other)
            {
                hash_table tmp{other};
                tmp.swap(*this);

                return *this;
            }

            hash_table& operator=(hash_table&& other)
            {
                hash_table tmp{move(other)};
                tmp.swap(*this);

                return *this;
            }

            bool empty() const noexcept
            {
                return size_ == 0;
            }

            size_type size() const noexcept
            {
                return size_;
            }

            template<class Allocator>
            size_type max_size(Allocator& alloc)
            {
                return allocator_traits<Allocator>::max_size(alloc);
            }

            iterator begin() noexcept
            {
                return iterator{
                    table_, size_type{}, bucket_count_,
                    table_[0].head
                };
            }

            const_iterator begin() const noexcept
            {
                return cbegin();
            }

            iterator end() noexcept
            {
                return iterator{};
            }

            const_iterator end() const noexcept
            {
                return cend();
            }

            const_iterator cbegin() const noexcept
            {
                return const_iterator{
                    table_, size_type{}, bucket_count_,
                    table_[0].head
                };
            }

            const_iterator cend() const noexcept
            {
                return const_iterator{};
            }

            template<class Allocator, class... Args>
            void emplace(const hint_type& where, Allocator& alloc, Args&&... args)
            {
                if (!hint_ok_(where))
                    return;

                auto node = new list_node<value_type>{forward<Args&&>(args)...};
                if (get<1>(where) == nullptr) // Append here will create a new head.
                    get<0>(where)->append(node);
                else // Prepending before an exact position is common in the standard.
                    get<1>(where)->prepend(node);

                ++size_;

                rehash_if_needed();
            }

            void insert(const hint_type& where, const value_type& val)
            {
                if (!hint_ok_(where))
                    return;

                auto node = new list_node<value_type>{val};
                if (get<1>(where) == nullptr)
                    get<0>(where)->append(node);
                else
                    get<1>(where)->prepend(node);

                ++size_;

                rehash_if_needed();
            }

            void insert(const hint_type& where, value_type&& val)
            {
                if (!hint_ok_(where))
                    return;

                auto node = new list_node<value_type>{forward<value_type>(val)};
                if (get<1>(where) == nullptr)
                    get<0>(where)->append(node);
                else
                    get<1>(where)->prepend(node);

                ++size_;

                rehash_if_needed();
            }

            size_type erase(const key_type& key)
            {
                return Policy::erase(*this, key);
            }

            iterator erase(const_iterator it)
            {
                auto node = it.node();
                auto idx = it.idx();

                /**
                 * Note: This way we will continue on the next bucket
                 *       if this is the last element in its bucket.
                 */
                iterator res{table_, idx, size_, node};
                ++res;

                if (table_[idx].head == node)
                {
                    if (node->next != node)
                        table_[idx].head = node->next;
                    else
                        table_[idx].head = nullptr;
                }

                node->unlink();
                delete node;

                return res;
            }

            void clear() noexcept
            {
                for (size_type i = 0; i < bucket_count_; ++i)
                    table_[i].clear();
                size_ = size_type{};
            }

            void swap(hash_table& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         noexcept(swap(declval<Hasher&>(), declval<Hasher&>())) &&
                         noexcept(swap(declval<KeyEq&>(), declval<KeyEq&>())))
            {
                std::swap(table_, other.table_);
                std::swap(bucket_count_, other.bucket_count_);
                std::swap(size_, other.size_);
                std::swap(hasher_, other.hasher_);
                std::swap(key_eq_, other.key_eq_);
                std::swap(max_load_factor_, other.max_load_factor_);
            }

            hasher hash_function() const
            {
                return hasher_;
            }

            key_equal key_eq() const
            {
                return key_eq_;
            }

            iterator find(const key_type& key)
            {
                auto idx = get_bucket_idx_(key);
                auto head = table_[idx].head;

                if (!head)
                    return end();

                auto current = head;
                do
                {
                    if (key_eq_(key, key_extractor_(current->value)))
                        return iterator{table_, idx, size_, current};
                    current = current->next;
                }
                while (current != head);

                return end();
            }

            const_iterator find(const key_type& key) const
            {
                auto idx = get_bucket_idx_(key);
                auto head = table_[idx].head;

                if (!head)
                    return end();

                auto current = head;
                do
                {
                    if (key_eq_(key, key_extractor_(current->value)))
                        return iterator{table_, idx, size_, current};
                    current = current->next;
                }
                while (current != head);

                return end();
            }

            size_type count(const key_type& key) const
            {
                return Policy::count(*this, key);
            }

            pair<iterator, iterator> equal_range(const key_type& key)
            {
                return Policy::equal_range(*this, key);
            }

            pair<const_iterator, const_iterator> equal_range(const key_type& key) const
            {
                return Policy::equal_range_const(*this, key);
            }

            size_type bucket_count() const noexcept
            {
                return bucket_count_;
            }

            size_type max_bucket_count() const noexcept
            {
                return numeric_limits<size_type>::max() /
                       sizeof(hash_table_bucket<value_type, size_type>);
            }

            size_type bucket_size(size_type n) const
            {
                return table_[n].size();
            }

            size_type bucket(const key_type& key) const
            {
                return get_bucket_idx_(key);
            }

            local_iterator begin(size_type n)
            {
                return local_iterator{table_[n].head, table_[n].head};
            }

            const_local_iterator begin(size_type n) const
            {
                return cbegin(n);
            }

            local_iterator end(size_type n)
            {
                return local_iterator{};
            }

            const_local_iterator end(size_type n) const
            {
                return cend(n);
            }

            const_local_iterator cbegin(size_type n) const
            {
                return const_local_iterator{table_[n].head, table_[n].head};
            }

            const_local_iterator cend(size_type n) const
            {
                return const_local_iterator{};
            }

            float load_factor() const noexcept
            {
                return size_ / static_cast<float>(bucket_count_);
            }

            float max_load_factor() const noexcept
            {
                return max_load_factor_;
            }

            void max_load_factor(float factor)
            {
                if (factor > 0.f)
                    max_load_factor_ = factor;

                rehash_if_needed();
            }

            void rehash(size_type count)
            {
                if (count < size_ / max_load_factor_)
                    count = size_ / max_load_factor_;

                /**
                 * Note: If an exception is thrown, there
                 *       is no effect. Since this is the only
                 *       place where an exception (no mem) can
                 *       be thrown and no changes to this have been
                 *       made, we're ok.
                 */
                hash_table new_table{count, max_load_factor_};

                for (std::size_t i = 0; i < bucket_count_; ++i)
                {
                    auto head = table_[i].head;
                    if (!head)
                        continue;

                    auto current = head;

                    do
                    {
                        auto next = current->next;

                        current->next = current;
                        current->prev = current;

                        auto where = Policy::find_insertion_spot(
                            new_table, key_extractor_(current->value)
                        );

                        /**
                         * Note: We're rehashing, so we know each
                         *       key can be inserted.
                         */
                        auto new_bucket = get<0>(where);
                        auto new_successor = get<1>(where);

                        if (new_successor)
                            new_successor->append(current);
                        else
                            new_bucket->append(current);

                        current = next;
                    } while (current != head);

                    table_[i].head = nullptr;
                }

                new_table.size_ = size_;
                swap(new_table);

                delete[] new_table.table_;
                new_table.table_ = nullptr;
            }

            void reserve(size_type count)
            {
                rehash(count / max_load_factor_ + 1);
            }

            bool is_eq_to(const hash_table& other)
            {
                // TODO: implement
                return false;
            }

            ~hash_table()
            {
                // Lists are deleted in ~hash_table_bucket.
                if (table_)
                    delete[] table_;
            }

            hint_type find_insertion_spot(const key_type& key)
            {
                return Policy::find_insertion_spot(*this, key);
            }

            hint_type find_insertion_spot(key_type&& key)
            {
                return Policy::find_insertion_spot(*this, key);
            }

            const key_type& get_key(const value_type& val)
            {
                return key_extractor_(val);
            }

            bool keys_equal(const key_type& key, const value_type& val)
            {
                return key_eq_(key, key_extractor_(val));
            }

            bool keys_equal(const key_type& key, const value_type& val) const
            {
                return key_eq_(key, key_extractor_(val));
            }

            hash_table_bucket<value_type, size_type>* table()
            {
                return table_;
            }

            hash_table_bucket<value_type, size_type>* head(size_type idx)
            {
                if (idx < bucket_count_)
                    return &table_[idx];
                else
                    return nullptr;
            }

            void rehash_if_needed()
            {
                if (size_ > max_load_factor_ * bucket_count_)
                    rehash(bucket_count_ * bucket_count_growth_factor_);
            }

            void increment_size()
            {
                ++size_;

                rehash_if_needed();
            }

            void decrement_size()
            {
                --size_;
            }

            node_type* find_node_or_return_head(const key_type& key,
                                                const hash_table_bucket<value_type, size_type>& bucket)
            {
                if (bucket.head)
                {
                    auto head = bucket.head;
                    auto current = bucket.head;

                    do
                    {
                        if (keys_equal(key, current->value))
                            return current;
                        else
                            current = current->next;
                    }
                    while (current != head);

                    return head;
                }
                else
                    return nullptr;
            }

        private:
            hash_table_bucket<value_type, size_type>* table_;
            size_type bucket_count_;
            size_type size_;
            hasher hasher_;
            key_equal key_eq_;
            key_extract key_extractor_;
            float max_load_factor_;

            static constexpr float bucket_count_growth_factor_{1.25};

            size_type get_bucket_idx_(const key_type& key) const
            {
                return hasher_(key) % bucket_count_;
            }

            bool hint_ok_(const hint_type& hint)
            {
                // TODO: pass this to the policy, because the multi policy
                //       will need to check if a similar key is close,
                //       that is something like:
                //          return get<1>(hint)->prev->key == key || !bucket.contains(key)
                // TODO: also, make it public and make hint usage one level above?
                //       (since we already have insert with decisive hint)
                return get<0>(hint) != nullptr && get<2>(hint) < bucket_count_;
            }

            // Praise C++11 for this.
            friend Policy;
    };
}

#endif
