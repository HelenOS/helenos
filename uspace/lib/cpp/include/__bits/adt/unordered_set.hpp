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

#ifndef LIBCPP_BITS_ADT_UNORDERED_SET
#define LIBCPP_BITS_ADT_UNORDERED_SET

#include <__bits/adt/hash_table.hpp>
#include <initializer_list>
#include <functional>
#include <memory>
#include <utility>

namespace std
{
    /**
     * 23.5.6, class template unordered_set:
     */

    template<
        class Key,
        class Hash = hash<Key>,
        class Pred = equal_to<Key>,
        class Alloc = allocator<Key>
    >
    class unordered_set
    {
        public:
            using key_type        = Key;
            using value_type      = Key;
            using hasher          = Hash;
            using key_equal       = Pred;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            /**
             * Note: Both the iterator and const_iterator (and their local variants)
             *       types are constant iterators, the standard does not require them
             *       to be the same type, but why not? :)
             */
            using iterator             = aux::hash_table_const_iterator<
                value_type, const_reference, const_pointer, size_type
            >;
            using const_iterator       = iterator;
            using local_iterator       = aux::hash_table_const_local_iterator<
                value_type, const_reference, const_pointer
            >;
            using const_local_iterator = local_iterator;

            /**
             * Note: We need () to delegate the constructor,
             *       otherwise it could be deduced as the initializer
             *       list constructor when size_type is convertible
             *       to value_type.
             */
            unordered_set()
                : unordered_set(default_bucket_count_)
            { /* DUMMY BODY */ }

            explicit unordered_set(size_type bucket_count,
                                   const hasher& hf = hasher{},
                                   const key_equal& eql = key_equal{},
                                   const allocator_type& alloc = allocator_type{})
                : table_{bucket_count, hf, eql}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_set(InputIterator first, InputIterator last,
                          size_type bucket_count = default_bucket_count_,
                          const hasher& hf = hasher{},
                          const key_equal& eql = key_equal{},
                          const allocator_type& alloc = allocator_type{})
                : unordered_set{bucket_count, hf, eql, alloc}
            {
                insert(first, last);
            }

            unordered_set(const unordered_set& other)
                : unordered_set{other, other.allocator_}
            { /* DUMMY BODY */ }

            unordered_set(unordered_set&& other)
                : table_{move(other.table_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit unordered_set(const allocator_type& alloc)
                : table_{default_bucket_count_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_set(const unordered_set& other, const allocator_type& alloc)
                : table_{other.table_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_set(unordered_set&& other, const allocator_type& alloc)
                : table_{move(other.table_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_set(initializer_list<value_type> init,
                          size_type bucket_count = default_bucket_count_,
                          const hasher& hf = hasher{},
                          const key_equal& eql = key_equal{},
                          const allocator_type& alloc = allocator_type{})
                : unordered_set{bucket_count, hf, eql, alloc}
            {
                insert(init.begin(), init.end());
            }

            unordered_set(size_type bucket_count, const allocator_type& alloc)
                : unordered_set{bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_set(size_type bucket_count, const hasher& hf, const allocator_type& alloc)
                : unordered_set{bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_set(InputIterator first, InputIterator last,
                          size_type bucket_count, const allocator_type& alloc)
                : unordered_set{first, last, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_set(InputIterator first, InputIterator last,
                          size_type bucket_count, const hasher& hf, const allocator_type& alloc)
                : unordered_set{first, last, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_set(initializer_list<value_type> init, size_type bucket_count,
                          const allocator_type& alloc)
                : unordered_set{init, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_set(initializer_list<value_type> init, size_type bucket_count,
                          const hasher& hf, const allocator_type& alloc)
                : unordered_set{init, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            ~unordered_set()
            { /* DUMMY BODY */ }

            unordered_set& operator=(const unordered_set& other)
            {
                table_ = other.table_;
                allocator_ = other.allocator_;

                return *this;
            }

            unordered_set& operator=(unordered_set&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<hasher>::value &&
                         is_nothrow_move_assignable<key_equal>::value)
            {
                table_ = move(other.table_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            unordered_set& operator=(initializer_list<value_type>& init)
            {
                table_.clear();
                table_.reserve(init.size());

                insert(init.begin(), init.end());

                return *this;
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_;
            }

            bool empty() const noexcept
            {
                return table_.empty();
            }

            size_type size() const noexcept
            {
                return table_.size();
            }

            size_type max_size() const noexcept
            {
                return table_.max_size(allocator_);
            }

            iterator begin() noexcept
            {
                return table_.begin();
            }

            const_iterator begin() const noexcept
            {
                return table_.begin();
            }

            iterator end() noexcept
            {
                return table_.end();
            }

            const_iterator end() const noexcept
            {
                return table_.end();
            }

            const_iterator cbegin() const noexcept
            {
                return table_.cbegin();
            }

            const_iterator cend() const noexcept
            {
                return table_.cend();
            }

            template<class... Args>
            pair<iterator, bool> emplace(Args&&... args)
            {
                return table_.emplace(forward<Args>(args)...);
            }

            template<class... Args>
            iterator emplace_hint(const_iterator, Args&&... args)
            {
                return emplace(forward<Args>(args)...).first;
            }

            pair<iterator, bool> insert(const value_type& val)
            {
                return table_.insert(val);
            }

            pair<iterator, bool> insert(value_type&& val)
            {
                return table_.insert(forward<value_type>(val));
            }

            iterator insert(const_iterator, const value_type& val)
            {
                return insert(val).first;
            }

            iterator insert(const_iterator, value_type&& val)
            {
                return insert(forward<value_type>(val)).first;
            }

            template<class InputIterator>
            void insert(InputIterator first, InputIterator last)
            {
                while (first != last)
                    insert(*first++);
            }

            void insert(initializer_list<value_type> init)
            {
                insert(init.begin(), init.end());
            }

            iterator erase(const_iterator position)
            {
                return table_.erase(position);
            }

            size_type erase(const key_type& key)
            {
                return table_.erase(key);
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                while (first != last)
                    first = erase(first);

                return iterator{
                    table_.table(), first.idx(),
                    table_.bucket_count(), first.node()
                };
            }

            void clear() noexcept
            {
                table_.clear();
            }

            void swap(unordered_set& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         noexcept(std::swap(declval<hasher>(), declval<hasher>())) &&
                         noexcept(std::swap(declval<key_equal>(), declval<key_equal>())))
            {
                table_.swap(other.table_);
                std::swap(allocator_, other.allocator_);
            }

            hasher hash_function() const
            {
                return table_.hash_function();
            }

            key_equal key_eq() const
            {
                return table_.key_eq();
            }

            iterator find(const key_type& key)
            {
                return table_.find(key);
            }

            const_iterator find(const key_type& key) const
            {
                return table_.find(key);
            }

            size_type count(const key_type& key) const
            {
                return table_.count(key);
            }

            pair<iterator, iterator> equal_range(const key_type& key)
            {
                return table_.equal_range(key);
            }

            pair<const_iterator, const_iterator> equal_range(const key_type& key) const
            {
                return table_.equal_range(key);
            }

            size_type bucket_count() const noexcept
            {
                return table_.bucket_count();
            }

            size_type max_bucket_count() const noexcept
            {
                return table_.max_bucket_count();
            }

            size_type bucket_size(size_type idx) const
            {
                return table_.bucket_size(idx);
            }

            size_type bucket(const key_type& key) const
            {
                return table_.bucket(key);
            }

            local_iterator begin(size_type idx)
            {
                return table_.begin(idx);
            }

            const_local_iterator begin(size_type idx) const
            {
                return table_.begin(idx);
            }

            local_iterator end(size_type idx)
            {
                return table_.end(idx);
            }

            const_local_iterator end(size_type idx) const
            {
                return table_.end(idx);
            }

            const_local_iterator cbegin(size_type idx) const
            {
                return table_.cbegin(idx);
            }

            const_local_iterator cend(size_type idx) const
            {
                return table_.cend(idx);
            }

            float load_factor() const noexcept
            {
                return table_.load_factor();
            }

            float max_load_factor() const noexcept
            {
                return table_.max_load_factor();
            }

            void max_load_factor(float factor)
            {
                table_.max_load_factor(factor);
            }

            void rehash(size_type bucket_count)
            {
                table_.rehash(bucket_count);
            }

            void reserve(size_type count)
            {
                table_.reserve(count);
            }

        private:
            using table_type = aux::hash_table<
                key_type, key_type, aux::key_no_value_key_extractor<key_type>,
                hasher, key_equal, allocator_type, size_type,
                iterator, const_iterator, local_iterator, const_local_iterator,
                aux::hash_single_policy
            >;

            table_type table_;
            allocator_type allocator_;

            static constexpr size_type default_bucket_count_{16};

            template<class K, class H, class P, class A>
            friend bool operator==(const unordered_set<K, H, P, A>&,
                                   const unordered_set<K, H, P, A>&);
    };

    /**
     * 23.5.7, class template unordered_multiset:
     */

    template<
        class Key,
        class Hash = hash<Key>,
        class Pred = equal_to<Key>,
        class Alloc = allocator<Key>
    >
    class unordered_multiset
    {
        public:
            using key_type        = Key;
            using value_type      = Key;
            using hasher          = Hash;
            using key_equal       = Pred;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            /**
             * Note: Both the iterator and const_iterator (and their local variants)
             *       types are constant iterators, the standard does not require them
             *       to be the same type, but why not? :)
             */
            using iterator             = aux::hash_table_const_iterator<
                value_type, const_reference, const_pointer, size_type
            >;
            using const_iterator       = iterator;
            using local_iterator       = aux::hash_table_const_local_iterator<
                value_type, const_reference, const_pointer
            >;
            using const_local_iterator = local_iterator;

            /**
             * Note: We need () to delegate the constructor,
             *       otherwise it could be deduced as the initializer
             *       list constructor when size_type is convertible
             *       to value_type.
             */
            unordered_multiset()
                : unordered_multiset(default_bucket_count_)
            { /* DUMMY BODY */ }

            explicit unordered_multiset(size_type bucket_count,
                                        const hasher& hf = hasher{},
                                        const key_equal& eql = key_equal{},
                                        const allocator_type& alloc = allocator_type{})
                : table_{bucket_count, hf, eql}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_multiset(InputIterator first, InputIterator last,
                               size_type bucket_count = default_bucket_count_,
                               const hasher& hf = hasher{},
                               const key_equal& eql = key_equal{},
                               const allocator_type& alloc = allocator_type{})
                : unordered_multiset{bucket_count, hf, eql, alloc}
            {
                insert(first, last);
            }

            unordered_multiset(const unordered_multiset& other)
                : unordered_multiset{other, other.allocator_}
            { /* DUMMY BODY */ }

            unordered_multiset(unordered_multiset&& other)
                : table_{move(other.table_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit unordered_multiset(const allocator_type& alloc)
                : table_{default_bucket_count_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_multiset(const unordered_multiset& other, const allocator_type& alloc)
                : table_{other.table_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_multiset(unordered_multiset&& other, const allocator_type& alloc)
                : table_{move(other.table_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_multiset(initializer_list<value_type> init,
                               size_type bucket_count = default_bucket_count_,
                               const hasher& hf = hasher{},
                               const key_equal& eql = key_equal{},
                               const allocator_type& alloc = allocator_type{})
                : unordered_multiset{bucket_count, hf, eql, alloc}
            {
                insert(init.begin(), init.end());
            }

            unordered_multiset(size_type bucket_count, const allocator_type& alloc)
                : unordered_multiset{bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_multiset(size_type bucket_count, const hasher& hf, const allocator_type& alloc)
                : unordered_multiset{bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_multiset(InputIterator first, InputIterator last,
                               size_type bucket_count, const allocator_type& alloc)
                : unordered_multiset{first, last, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_multiset(InputIterator first, InputIterator last,
                               size_type bucket_count, const hasher& hf, const allocator_type& alloc)
                : unordered_multiset{first, last, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_multiset(initializer_list<value_type> init, size_type bucket_count,
                               const allocator_type& alloc)
                : unordered_multiset{init, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_multiset(initializer_list<value_type> init, size_type bucket_count,
                               const hasher& hf, const allocator_type& alloc)
                : unordered_multiset{init, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            ~unordered_multiset()
            { /* DUMMY BODY */ }

            unordered_multiset& operator=(const unordered_multiset& other)
            {
                table_ = other.table_;
                allocator_ = other.allocator_;

                return *this;
            }

            unordered_multiset& operator=(unordered_multiset&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<hasher>::value &&
                         is_nothrow_move_assignable<key_equal>::value)
            {
                table_ = move(other.table_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            unordered_multiset& operator=(initializer_list<value_type>& init)
            {
                table_.clear();
                table_.reserve(init.size());

                insert(init.begin(), init.end());

                return *this;
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_;
            }

            bool empty() const noexcept
            {
                return table_.empty();
            }

            size_type size() const noexcept
            {
                return table_.size();
            }

            size_type max_size() const noexcept
            {
                return table_.max_size(allocator_);
            }

            iterator begin() noexcept
            {
                return table_.begin();
            }

            const_iterator begin() const noexcept
            {
                return table_.begin();
            }

            iterator end() noexcept
            {
                return table_.end();
            }

            const_iterator end() const noexcept
            {
                return table_.end();
            }

            const_iterator cbegin() const noexcept
            {
                return table_.cbegin();
            }

            const_iterator cend() const noexcept
            {
                return table_.cend();
            }

            template<class... Args>
            iterator emplace(Args&&... args)
            {
                return table_.emplace(forward<Args>(args)...);
            }

            template<class... Args>
            iterator emplace_hint(const_iterator, Args&&... args)
            {
                return emplace(forward<Args>(args)...);
            }

            iterator insert(const value_type& val)
            {
                return table_.insert(val);
            }

            iterator insert(value_type&& val)
            {
                return table_.insert(forward<value_type>(val));
            }

            iterator insert(const_iterator, const value_type& val)
            {
                return insert(val);
            }

            iterator insert(const_iterator, value_type&& val)
            {
                return insert(forward<value_type>(val));
            }

            template<class InputIterator>
            void insert(InputIterator first, InputIterator last)
            {
                while (first != last)
                    insert(*first++);
            }

            void insert(initializer_list<value_type> init)
            {
                insert(init.begin(), init.end());
            }

            iterator erase(const_iterator position)
            {
                return table_.erase(position);
            }

            size_type erase(const key_type& key)
            {
                return table_.erase(key);
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                while (first != last)
                    first = erase(first);

                return iterator{
                    table_.table(), first.idx(),
                    table_.bucket_count(), table_.head(first.idx())
                };
            }

            void clear() noexcept
            {
                table_.clear();
            }

            void swap(unordered_multiset& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         noexcept(std::swap(declval<hasher>(), declval<hasher>())) &&
                         noexcept(std::swap(declval<key_equal>(), declval<key_equal>())))
            {
                table_.swap(other.table_);
                std::swap(allocator_, other.allocator_);
            }

            hasher hash_function() const
            {
                return table_.hash_function();
            }

            key_equal key_eq() const
            {
                return table_.key_eq();
            }

            iterator find(const key_type& key)
            {
                return table_.find(key);
            }

            const_iterator find(const key_type& key) const
            {
                return table_.find(key);
            }

            size_type count(const key_type& key) const
            {
                return table_.count(key);
            }

            pair<iterator, iterator> equal_range(const key_type& key)
            {
                return table_.equal_range(key);
            }

            pair<const_iterator, const_iterator> equal_range(const key_type& key) const
            {
                return table_.equal_range(key);
            }

            size_type bucket_count() const noexcept
            {
                return table_.bucket_count();
            }

            size_type max_bucket_count() const noexcept
            {
                return table_.max_bucket_count();
            }

            size_type bucket_size(size_type idx) const
            {
                return table_.bucket_size(idx);
            }

            size_type bucket(const key_type& key) const
            {
                return table_.bucket(key);
            }

            local_iterator begin(size_type idx)
            {
                return table_.begin(idx);
            }

            const_local_iterator begin(size_type idx) const
            {
                return table_.begin(idx);
            }

            local_iterator end(size_type idx)
            {
                return table_.end(idx);
            }

            const_local_iterator end(size_type idx) const
            {
                return table_.end(idx);
            }

            const_local_iterator cbegin(size_type idx) const
            {
                return table_.cbegin(idx);
            }

            const_local_iterator cend(size_type idx) const
            {
                return table_.cend(idx);
            }

            float load_factor() const noexcept
            {
                return table_.load_factor();
            }

            float max_load_factor() const noexcept
            {
                return table_.max_load_factor();
            }

            void max_load_factor(float factor)
            {
                table_.max_load_factor(factor);
            }

            void rehash(size_type bucket_count)
            {
                table_.rehash(bucket_count);
            }

            void reserve(size_type count)
            {
                table_.reserve(count);
            }

        private:
            using table_type = aux::hash_table<
                key_type, key_type, aux::key_no_value_key_extractor<key_type>,
                hasher, key_equal, allocator_type, size_type,
                iterator, const_iterator, local_iterator, const_local_iterator,
                aux::hash_multi_policy
            >;

            table_type table_;
            allocator_type allocator_;

            static constexpr size_type default_bucket_count_{16};

            template<class K, class H, class P, class A>
            friend bool operator==(const unordered_multiset<K, H, P, A>&,
                                   const unordered_multiset<K, H, P, A>&);
    };

    template<class Key, class Hash, class Pred, class Alloc>
    void swap(unordered_set<Key, Hash, Pred, Alloc>& lhs,
              unordered_set<Key, Hash, Pred, Alloc>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class Key, class Hash, class Pred, class Alloc>
    void swap(unordered_multiset<Key, Hash, Pred, Alloc>& lhs,
              unordered_multiset<Key, Hash, Pred, Alloc>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class Key, class Hash, class Pred, class Alloc>
    bool operator==(const unordered_set<Key, Hash, Pred, Alloc>& lhs,
                    const unordered_set<Key, Hash, Pred, Alloc>& rhs)
    {
        return lhs.table_.is_eq_to(rhs.table_);
    }

    template<class Key, class Hash, class Pred, class Alloc>
    bool operator!=(const unordered_set<Key, Hash, Pred, Alloc>& lhs,
                    const unordered_set<Key, Hash, Pred, Alloc>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Key, class Hash, class Pred, class Alloc>
    bool operator==(const unordered_multiset<Key, Hash, Pred, Alloc>& lhs,
                    const unordered_multiset<Key, Hash, Pred, Alloc>& rhs)
    {
        return lhs.table_.is_eq_to(rhs.table_);
    }

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    bool operator!=(const unordered_multiset<Key, Hash, Pred, Alloc>& lhs,
                    const unordered_multiset<Key, Hash, Pred, Alloc>& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif
