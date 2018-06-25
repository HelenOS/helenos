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

#ifndef LIBCPP_BITS_ADT_UNORDERED_MAP
#define LIBCPP_BITS_ADT_UNORDERED_MAP

#include <__bits/adt/hash_table.hpp>
#include <initializer_list>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace std
{
    /**
     * 23.5.4, class template unordered_map:
     */

    template<
        class Key, class Value,
        class Hash = hash<Key>,
        class Pred = equal_to<Key>,
        class Alloc = allocator<pair<const Key, Value>>
    >
    class unordered_map
    {
        public:
            using key_type        = Key;
            using mapped_type     = Value;
            using value_type      = pair<const key_type, mapped_type>;
            using hasher          = Hash;
            using key_equal       = Pred;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            using iterator             = aux::hash_table_iterator<
                value_type, reference, pointer, size_type
            >;
            using const_iterator       = aux::hash_table_const_iterator<
                value_type, const_reference, const_pointer, size_type
            >;
            using local_iterator       = aux::hash_table_local_iterator<
                value_type, reference, pointer
            >;
            using const_local_iterator = aux::hash_table_const_local_iterator<
                value_type, const_reference, const_pointer
            >;

            unordered_map()
                : unordered_map{default_bucket_count_}
            { /* DUMMY BODY */ }

            explicit unordered_map(size_type bucket_count,
                                   const hasher& hf = hasher{},
                                   const key_equal& eql = key_equal{},
                                   const allocator_type& alloc = allocator_type{})
                : table_{bucket_count, hf, eql}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_map(InputIterator first, InputIterator last,
                          size_type bucket_count = default_bucket_count_,
                          const hasher& hf = hasher{},
                          const key_equal& eql = key_equal{},
                          const allocator_type& alloc = allocator_type{})
                : unordered_map{bucket_count, hf, eql, alloc}
            {
                insert(first, last);
            }

            unordered_map(const unordered_map& other)
                : unordered_map{other, other.allocator_}
            { /* DUMMY BODY */ }

            unordered_map(unordered_map&& other)
                : table_{move(other.table_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit unordered_map(const allocator_type& alloc)
                : table_{default_bucket_count_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_map(const unordered_map& other, const allocator_type& alloc)
                : table_{other.table_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_map(unordered_map&& other, const allocator_type& alloc)
                : table_{move(other.table_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_map(initializer_list<value_type> init,
                          size_type bucket_count = default_bucket_count_,
                          const hasher& hf = hasher{},
                          const key_equal& eql = key_equal{},
                          const allocator_type& alloc = allocator_type{})
                : unordered_map{bucket_count, hf, eql, alloc}
            {
                insert(init.begin(), init.end());
            }

            unordered_map(size_type bucket_count, const allocator_type& alloc)
                : unordered_map{bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_map(size_type bucket_count, const hasher& hf, const allocator_type& alloc)
                : unordered_map{bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_map(InputIterator first, InputIterator last,
                          size_type bucket_count, const allocator_type& alloc)
                : unordered_map{first, last, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_map(InputIterator first, InputIterator last,
                          size_type bucket_count, const hasher& hf, const allocator_type& alloc)
                : unordered_map{first, last, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_map(initializer_list<value_type> init, size_type bucket_count,
                          const allocator_type& alloc)
                : unordered_map{init, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_map(initializer_list<value_type> init, size_type bucket_count,
                          const hasher& hf, const allocator_type& alloc)
                : unordered_map{init, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            ~unordered_map()
            { /* DUMMY BODY */ }

            unordered_map& operator=(const unordered_map& other)
            {
                table_ = other.table_;
                allocator_ = other.allocator_;

                return *this;
            }

            unordered_map& operator=(unordered_map&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<hasher>::value &&
                         is_nothrow_move_assignable<key_equal>::value)
            {
                table_ = move(other.table_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            unordered_map& operator=(initializer_list<value_type>& init)
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

            template<class T>
            pair<iterator, bool> insert(
                T&& val,
                enable_if_t<is_constructible_v<value_type, T&&>>* = nullptr
            )
            {
                return emplace(forward<T>(val));
            }

            iterator insert(const_iterator, const value_type& val)
            {
                return insert(val).first;
            }

            iterator insert(const_iterator, value_type&& val)
            {
                return insert(forward<value_type>(val)).first;
            }

            template<class T>
            iterator insert(
                const_iterator hint,
                T&& val,
                enable_if_t<is_constructible_v<value_type, T&&>>* = nullptr
            )
            {
                return emplace_hint(hint, forward<T>(val));
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

            template<class... Args>
            pair<iterator, bool> try_emplace(const key_type& key, Args&&... args)
            {
                /**
                 * Note: If anyone (like me) wonders what is the difference between
                 *       emplace and try_emplace, the former always constructs the value
                 *       (in order to get the key) and the latter does it only if
                 *       an insertion takes place.
                 */

                table_.increment_size();

                auto [bucket, target, idx] = table_.find_insertion_spot(key);

                if (!bucket)
                    return make_pair(end(), false);

                if (target && table_.keys_equal(key, target->value))
                {
                    table_.decrement_size();

                    return make_pair(
                        iterator{
                            table_.table(), idx, table_.bucket_count(),
                            target
                        },
                        false
                    );
                }
                else
                {
                    auto node = new node_type{key, forward<Args>(args)...};
                    bucket->append(node);

                    return make_pair(iterator{
                        table_.table(), idx,
                        table_.bucket_count(),
                        node
                    }, true);
                }
            }

            template<class... Args>
            pair<iterator, bool> try_emplace(key_type&& key, Args&&... args)
            {
                table_.increment_size();

                auto [bucket, target, idx] = table_.find_insertion_spot(key);

                if (!bucket)
                    return make_pair(end(), false);

                if (target && table_.keys_equal(key, target->value))
                {
                    table_.decrement_size();

                    return make_pair(
                        iterator{
                            table_.table(), idx, table_.bucket_count(),
                            target
                        },
                        false
                    );
                }
                else
                {
                    auto node = new node_type{move(key), forward<Args>(args)...};
                    bucket->append(node);

                    return make_pair(iterator{
                        table_.table(), idx,
                        table_.bucket_count(),
                        node
                    }, true);
                }
            }

            template<class... Args>
            iterator try_emplace(const_iterator, const key_type& key, Args&&... args)
            {
                return try_emplace(key, forward<Args>(args)...).first;
            }

            template<class... Args>
            iterator try_emplace(const_iterator, key_type&& key, Args&&... args)
            {
                return try_emplace(move(key), forward<Args>(args)...).first;
            }

            template<class T>
            pair<iterator, bool> insert_or_assign(const key_type& key, T&& val)
            {
                table_.increment_size();

                auto [bucket, target, idx] = table_.find_insertion_spot(key);

                if (!bucket)
                    return make_pair(end(), false);

                if (target && table_.keys_equal(key, target->value))
                {
                    table_.decrement_size();
                    target->value.second = forward<T>(val);

                    return make_pair(
                        iterator{
                            table_.table(), idx, table_.bucket_count(),
                            target
                        },
                        false
                    );
                }
                else
                {
                    auto node = new node_type{key, forward<T>(val)};
                    bucket->append(node);

                    return make_pair(iterator{
                        table_.table(), idx,
                        table_.bucket_count(),
                        node
                    }, true);
                }
            }

            template<class T>
            pair<iterator, bool> insert_or_assign(key_type&& key, T&& val)
            {
                table_.increment_size();

                auto [bucket, target, idx] = table_.find_insertion_spot(key);

                if (!bucket)
                    return make_pair(end(), false);

                if (target && table_.keys_equal(key, target->value))
                {
                    table_.decrement_size();
                    target->value.second = forward<T>(val);

                    return make_pair(
                        iterator{
                            table_.table(), idx, table_.bucket_count(),
                            target
                        },
                        false
                    );
                }
                else
                {
                    auto node = new node_type{move(key), forward<T>(val)};
                    bucket->append(node);

                    return make_pair(iterator{
                        table_.table(), idx,
                        table_.bucket_count(),
                        node
                    }, true);
                }
            }

            template<class T>
            iterator insert_or_assign(const_iterator, const key_type& key, T&& val)
            {
                return insert_or_assign(key, forward<T>(val)).first;
            }

            template<class T>
            iterator insert_or_assign(const_iterator, key_type&& key, T&& val)
            {
                return insert_or_assign(move(key), forward<T>(val)).first;
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

            void swap(unordered_map& other)
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

            mapped_type& operator[](const key_type& key)
            {
                auto spot = table_.find_insertion_spot(key);
                auto bucket = get<0>(spot);

                if (bucket->head)
                {
                    auto head = bucket->head;
                    auto current = bucket->head;

                    do
                    {
                        if (table_.keys_equal(key, current->value))
                            return current->value.second;
                        else
                            current = current->next;
                    }
                    while (current != head);
                }

                auto node = new node_type{key, mapped_type{}};
                bucket->append(node);

                table_.increment_size();
                table_.rehash_if_needed();
                return node->value.second;
            }

            mapped_type& operator[](key_type&& key)
            {
                auto spot = table_.find_insertion_spot(key);
                auto bucket = get<0>(spot);

                if (bucket->head)
                {
                    auto head = bucket->head;
                    auto current = bucket->head;

                    do
                    {
                        if (table_.keys_equal(key, current->value))
                            return current->value.second;
                        else
                            current = current->next;
                    }
                    while (current != head);
                }

                auto node = new node_type{move(key), mapped_type{}};
                bucket->append(node);

                table_.increment_size();
                table_.rehash_if_needed();
                return node->value.second;
            }

            mapped_type& at(const key_type& key)
            {
                auto it = find(key);

                // TODO: throw out_of_range if it == end()
                return it->second;
            }

            const mapped_type& at(const key_type& key) const
            {
                auto it = find(key);

                // TODO: throw out_of_range if it == end()
                return it->second;
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
                value_type, key_type, aux::key_value_key_extractor<key_type, mapped_type>,
                hasher, key_equal, allocator_type, size_type,
                iterator, const_iterator, local_iterator, const_local_iterator,
                aux::hash_single_policy
            >;
            using node_type = typename table_type::node_type;

            table_type table_;
            allocator_type allocator_;

            static constexpr size_type default_bucket_count_{16};

            template<class K, class V, class H, class P, class A>
            friend bool operator==(unordered_map<K, V, H, P, A>&,
                                   unordered_map<K, V, H, P, A>&);
    };

    /**
     * 23.5.5, class template unordered_multimap:
     */

    template<
        class Key, class Value,
        class Hash = hash<Key>,
        class Pred = equal_to<Key>,
        class Alloc = allocator<pair<const Key, Value>>
    >
    class unordered_multimap
    {
        public:
            using key_type        = Key;
            using mapped_type     = Value;
            using value_type      = pair<const key_type, mapped_type>;
            using hasher          = Hash;
            using key_equal       = Pred;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            using iterator             = aux::hash_table_iterator<
                value_type, reference, pointer, size_type
            >;
            using const_iterator       = aux::hash_table_const_iterator<
                value_type, const_reference, const_pointer, size_type
            >;
            using local_iterator       = aux::hash_table_local_iterator<
                value_type, reference, pointer
            >;
            using const_local_iterator = aux::hash_table_const_local_iterator<
                value_type, const_reference, const_pointer
            >;

            unordered_multimap()
                : unordered_multimap{default_bucket_count_}
            { /* DUMMY BODY */ }

            explicit unordered_multimap(size_type bucket_count,
                                        const hasher& hf = hasher{},
                                        const key_equal& eql = key_equal{},
                                        const allocator_type& alloc = allocator_type{})
                : table_{bucket_count, hf, eql}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_multimap(InputIterator first, InputIterator last,
                               size_type bucket_count = default_bucket_count_,
                               const hasher& hf = hasher{},
                               const key_equal& eql = key_equal{},
                               const allocator_type& alloc = allocator_type{})
                : unordered_multimap{bucket_count, hf, eql, alloc}
            {
                insert(first, last);
            }

            unordered_multimap(const unordered_multimap& other)
                : unordered_multimap{other, other.allocator_}
            { /* DUMMY BODY */ }

            unordered_multimap(unordered_multimap&& other)
                : table_{move(other.table_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit unordered_multimap(const allocator_type& alloc)
                : table_{default_bucket_count_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_multimap(const unordered_multimap& other, const allocator_type& alloc)
                : table_{other.table_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_multimap(unordered_multimap&& other, const allocator_type& alloc)
                : table_{move(other.table_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            unordered_multimap(initializer_list<value_type> init,
                               size_type bucket_count = default_bucket_count_,
                               const hasher& hf = hasher{},
                               const key_equal& eql = key_equal{},
                               const allocator_type& alloc = allocator_type{})
                : unordered_multimap{bucket_count, hf, eql, alloc}
            {
                insert(init.begin(), init.end());
            }

            unordered_multimap(size_type bucket_count, const allocator_type& alloc)
                : unordered_multimap{bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_multimap(size_type bucket_count, const hasher& hf,
                               const allocator_type& alloc)
                : unordered_multimap{bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_multimap(InputIterator first, InputIterator last,
                               size_type bucket_count, const allocator_type& alloc)
                : unordered_multimap{first, last, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            unordered_multimap(InputIterator first, InputIterator last,
                               size_type bucket_count, const hasher& hf,
                               const allocator_type& alloc)
                : unordered_multimap{first, last, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_multimap(initializer_list<value_type> init, size_type bucket_count,
                               const allocator_type& alloc)
                : unordered_multimap{init, bucket_count, hasher{}, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            unordered_multimap(initializer_list<value_type> init, size_type bucket_count,
                               const hasher& hf, const allocator_type& alloc)
                : unordered_multimap{init, bucket_count, hf, key_equal{}, alloc}
            { /* DUMMY BODY */ }

            ~unordered_multimap()
            { /* DUMMY BODY */ }

            unordered_multimap& operator=(const unordered_multimap& other)
            {
                table_ = other.table_;
                allocator_ = other.allocator_;

                return *this;
            }

            unordered_multimap& operator=(unordered_multimap&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<hasher>::value &&
                         is_nothrow_move_assignable<key_equal>::value)
            {
                table_ = move(other.table_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            unordered_multimap& operator=(initializer_list<value_type>& init)
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

            template<class T>
            iterator insert(
                T&& val,
                enable_if_t<is_constructible_v<value_type, T&&>>* = nullptr
            )
            {
                return emplace(forward<T>(val));
            }

            iterator insert(const_iterator, const value_type& val)
            {
                return insert(val);
            }

            iterator insert(const_iterator, value_type&& val)
            {
                return insert(forward<value_type>(val));
            }

            template<class T>
            iterator insert(
                const_iterator hint,
                T&& val,
                enable_if_t<is_constructible_v<value_type, T&&>>* = nullptr
            )
            {
                return emplace_hint(hint, forward<T>(val));
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

            void swap(unordered_multimap& other)
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
                value_type, key_type, aux::key_value_key_extractor<key_type, mapped_type>,
                hasher, key_equal, allocator_type, size_type,
                iterator, const_iterator, local_iterator, const_local_iterator,
                aux::hash_multi_policy
            >;

            table_type table_;
            allocator_type allocator_;

            static constexpr size_type default_bucket_count_{16};

            template<class K, class V, class H, class P, class A>
            friend bool operator==(unordered_multimap<K, V, H, P, A>&,
                                   unordered_multimap<K, V, H, P, A>&);
    };

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    void swap(unordered_map<Key, Value, Hash, Pred, Alloc>& lhs,
              unordered_map<Key, Value, Hash, Pred, Alloc>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    void swap(unordered_multimap<Key, Value, Hash, Pred, Alloc>& lhs,
              unordered_multimap<Key, Value, Hash, Pred, Alloc>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    bool operator==(unordered_map<Key, Value, Hash, Pred, Alloc>& lhs,
                    unordered_map<Key, Value, Hash, Pred, Alloc>& rhs)
    {
        // TODO: this does not compare values, use is_permutation when we have it
        return lhs.table_.is_eq_to(rhs.table_);
    }

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    bool operator!=(unordered_map<Key, Value, Hash, Pred, Alloc>& lhs,
                    unordered_map<Key, Value, Hash, Pred, Alloc>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    bool operator==(unordered_multimap<Key, Value, Hash, Pred, Alloc>& lhs,
                    unordered_multimap<Key, Value, Hash, Pred, Alloc>& rhs)
    {
        return lhs.table_.is_eq_to(rhs.table_);
    }

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    bool operator!=(unordered_multimap<Key, Value, Hash, Pred, Alloc>& lhs,
                    unordered_multimap<Key, Value, Hash, Pred, Alloc>& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif
