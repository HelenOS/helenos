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

#ifndef LIBCPP_BITS_ADT_HASH_TABLE
#define LIBCPP_BITS_ADT_HASH_TABLE

#include <__bits/adt/list_node.hpp>
#include <__bits/adt/key_extractors.hpp>
#include <__bits/adt/hash_table_iterators.hpp>
#include <__bits/adt/hash_table_policies.hpp>
#include <cstdlib>
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
     * Additionally, we will use policies to represent both single
     * and multi variants of these containers at once.
     * Note: I am aware the naming is very unimaginative here,
     *       not my strong side :)
     */

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

            using place_type = tuple<
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
                    insert(x);
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

            size_type max_size(allocator_type& alloc)
            {
                return allocator_traits<allocator_type>::max_size(alloc);
            }

            iterator begin() noexcept
            {
                auto idx = first_filled_bucket_();
                return iterator{
                    table_, idx, bucket_count_,
                    table_[idx].head
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
                auto idx = first_filled_bucket_();
                return const_iterator{
                    table_, idx, bucket_count_,
                    table_[idx].head
                };
            }

            const_iterator cend() const noexcept
            {
                return const_iterator{};
            }

            template<class... Args>
            auto emplace(Args&&... args)
            {
                return Policy::emplace(*this, forward<Args>(args)...);
            }

            auto insert(const value_type& val)
            {
                return Policy::insert(*this, val);
            }

            auto insert(value_type&& val)
            {
                return Policy::insert(*this, forward<value_type>(val));
            }

            size_type erase(const key_type& key)
            {
                return Policy::erase(*this, key);
            }

            iterator erase(const_iterator it)
            {
                if (it == cend())
                    return end();

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
                --size_;

                node->unlink();
                delete node;

                if (empty())
                    return end();
                else
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
                         noexcept(std::swap(declval<Hasher&>(), declval<Hasher&>())) &&
                         noexcept(std::swap(declval<KeyEq&>(), declval<KeyEq&>())))
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
                while (current && current != head);

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

            bool is_eq_to(const hash_table& other) const
            {
                if (size() != other.size())
                    return false;

                auto it = begin();
                while (it != end())
                {
                    /**
                     * For each key K we will check how many
                     * instances of K are there in the table.
                     * Then we will check if the count for K
                     * is equal to that amount.
                     */

                    size_type cnt{};
                    auto tmp = it;

                    while (key_eq_(key_extractor_(*it), key_extractor_(*tmp)))
                    {
                        ++cnt;
                        if (++tmp == end())
                            break;
                    }

                    auto other_cnt = other.count(key_extractor_(*it));
                    if (cnt != other_cnt)
                        return false;

                    it = tmp; // tmp  is one past *it's key.
                }

                return true;
            }

            ~hash_table()
            {
                // Lists are deleted in ~hash_table_bucket.
                if (table_)
                    delete[] table_;
            }

            place_type find_insertion_spot(const key_type& key) const
            {
                return Policy::find_insertion_spot(*this, key);
            }

            place_type find_insertion_spot(key_type&& key) const
            {
                return Policy::find_insertion_spot(*this, key);
            }

            const key_type& get_key(const value_type& val) const
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
                    return table_[idx]->head;
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

            size_type first_filled_bucket_() const
            {
                size_type res{};
                while (res < bucket_count_)
                {
                    if (table_[res].head)
                        return res;
                    ++res;
                }

                /**
                 * Note: This is used for iterators,
                 *       so we need to return a valid index.
                 *       But since table_[0].head is nullptr
                 *       we know that if we return 0 the
                 *       created iterator will test as equal
                 *       to end().
                 */
                return 0;
            }

            friend Policy;
    };
}

#endif
