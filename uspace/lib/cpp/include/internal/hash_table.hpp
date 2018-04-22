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
#include <memory>
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

    template<class Key, Value>
    struct key_value_key_extractor
    {
        Key& operator()(pair<Key, Value>& p) const noexcept
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
    };

    struct key_value_allocator
    { /* DUMMY BODY */ };

    template<>
    struct allocator_traits<key_value_allocator>
    {
        template<class Alloc, class Key, class Value, class... Args>
        static void construct(Alloc& alloc, pair<Key, Value>* ptr, Args&&... args)
        {
            alloc.construct(&ptr->second, forward<Args>(args)...);
        }
    };

    struct hash_single_policy
    {
        // TODO: umap/uset operations
    };

    struct hash_multi_policy
    {
        // TODO: umultimap/umultiset operations
    };

    template<class Value, class Size>
    struct hash_table_bucket
    {
        /**
         * Note: We use a doubly linked list because
         *       we need to use hints, which point to the
         *       element after the hinted spot.
         */

        Size count;
        link_node<Value>* head;

        ~hash_table_bucket()
        {
            // TODO: deallocate the entire list
        }
    };

    // TODO: iterator, const iterator, local iterator, const local iterator
    //       and also possibly two versions of each for umap and uset

    template<
        class Value, class Key, class KeyExtractor,
        class Hasher, class KeyEq,
        class Alloc, class Size,
        class Iterator, class ConstIterator,
        class LocalIterator, class LocalConstIterator,
        class Policy
    >
    class hash_table
    {
        /**
         * What we need:
         *  - insert
         *  - emplace
         *  - erase
         *  - iterator types
         *  - set hint (+ clear it after each operation)
         *  - copy + move
         *  - empty/size/max_size
         *  - try emplace
         *  - insert or assign
         *  - clear
         *  - find, count
         *  - equal range?
         *  - rehash/reserve
         *  - bucket stuff, local iterators (use list iterators?)
         *  - load factor stuff
         *  - multi versions for operations
         *  - eq/neq operators (or just functions that are called in the upper
         *    level operator?)
         *  - hasher and key_eq getter
         */
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

            hash_table(size_type buckets, float max_load_factor)
            {
                // TODO: implement
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
                // TODO: implement
            }

            const_iterator begin() const noexcept
            {
                // TODO: implement
            }

            iterator end() noexcept
            {
                // TODO: implement
            }

            const_iterator end() const noexcept
            {
                // TODO: implement
            }

            const_iterator cbegin() const noexcept
            {
                // TODO: implement
            }

            const_iterator cend() const noexcept
            {
                // TODO: implement
            }

            template<class Allocator, class... Args>
            pair<iterator, bool> emplace(Allocator& alloc, Args&&... args)
            {
                // TODO: use allocator traits of allocator_type but pass alloc!
                // TODO: also, try_emplace should be one level above (we don't know
                //       keys)
            }

            void insert(iterator it, value_type&& val)
            {
                // TODO: implement, make find_for_insert that will be used with this
                //       to find it to avoid unnecessary pair creations in umaps
                // TODO: also, insert_or_assign should be done one level above
            }

            size_type erase(const key_type& key)
            {
                // TODO: implement
            }

            iterator erase(const_iterator it)
            {
                // TODO: implement
            }

            void clear() noexcept
            {
                // TODO: implement
            }

            template<class Allocator>
            void swap(hash_table& other)
                noexcept(allocator_traits<Allocator>::is_always_equal::value &&
                         noexcept(swap(declval<Hasher&>(), declval<Hasher&>())) &&
                         noexcept(swap(declval<KeyEq&>(), declval<KeyEq&>())))
            {
                // TODO: implement
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
                // TODO: implement
            }

            const_iterator find(const key_type& key) const
            {
                // TODO: implement
            }

            size_type count(const key_type& key) const
            {
                // TODO: implement
            }

            pair<iterator, iterator> equal_range(const key_type& key)
            {
                // TODO: implement
            }

            pair<const_iterator, const_iterator> equal_range(const key_type& key) const
            {
                // TODO: implement
            }

            size_type bucket_count() const noexcept
            {
                return bucket_count_;
            }

            size_type max_bucket_count() const noexcept
            {
                // TODO: implement
                return 0;
            }

            size_type bucket_size(size_type n) const
            {
                // TODO: implement
            }

            size_type bucket(const key_type& key) const
            {
                // TODO: implement
            }

            local_iterator begin(size_type n)
            {
                // TODO: implement
            }

            const_local_iterator begin(size_type n) const
            {
                // TODO: implement
            }

            local_iterator end(size_type n) const
            {
                // TODO: implement
            }

            const_local_iterator cbegin(size_type n) const
            {
                // TODO: implement
            }

            const_local_iterator cend(size_type n) const
            {
                // TODO: implement
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
                /**
                 * Note: According to the standard, this function
                 *       can have no effect.
                 */
            }

            void rehash(size_type n)
            {
                // TODO: implement
            }

            void reserve(size_type n)
            {
                // TODO: implement
            }

            bool is_eq_to(hash_table& other)
            {
                // TODO: implement
            }

            ~hash_table()
            {
                // Lists are deleted in ~hash_table_bucket.
                delete[] table_;
            }

            void set_hint(const_iterator hint)
            {
                // TODO: hint_ should be a ptr and we extract it here,
                //       then set it to nullptr after each operation
            }

        private:
            hash_table_bucket<value_type, size_type>* table_;
            size_type bucket_count_;
            size_type size_;
            hasher hasher_;
            key_equal key_eq_;
            float max_load_factor_;
    };

    template<
        class Key, class Value,
        class Hasher, class KeyEq,
        class Alloc, class Size
    >
    class hash_table_with_value
    {
        public:
            using extractor_type  = key_value_key_extractor<Key, Value>;
            using table_type      = hash_table<
                pair<Key, Value>, extractor_type,
                KeyEq, key_value_allocator, Size
            >;

        private:
            table_type table_;
            extractor_type extractor_;
    };

    template<
        class Key,
        class Hasher, class KeyEq,
        class Alloc, class Size
    >
    class hash_table_without_value
    {
            using extractor_type  = key_no_value_key_extractor<Key>;
            using table_type      = hash_table<
                Key, extractor_type, KeyEq, Alloc, Size
            >;

        private:
            table_type table_;
    };
}

#endif
