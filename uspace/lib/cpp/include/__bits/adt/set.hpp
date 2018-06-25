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

#ifndef LIBCPP_BITS_ADT_SET
#define LIBCPP_BITS_ADT_SET

#include <__bits/adt/rbtree.hpp>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>

namespace std
{
    /**
     * 23.4.6, class template set:
     */

    template<
        class Key,
        class Compare = less<Key>,
        class Alloc = allocator<Key>
    >
    class set
    {
        public:
            using key_type        = Key;
            using value_type      = Key;
            using key_compare     = Compare;
            using value_compare   = Compare;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            using node_type = aux::rbtree_single_node<value_type>;

            /**
             * Note: Both the iterator and const_iterator (and their local variants)
             *       types are constant iterators, the standard does not require them
             *       to be the same type, but why not? :)
             */
            using iterator             = aux::rbtree_const_iterator<
                value_type, const_reference, const_pointer, size_type, node_type
            >;
            using const_iterator       = iterator;

            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            set()
                : set{key_compare{}}
            { /* DUMMY BODY */ }

            explicit set(const key_compare& comp,
                         const allocator_type& alloc = allocator_type{})
                : tree_{comp}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            set(InputIterator first, InputIterator last,
                const key_compare& comp = key_compare{},
                const allocator_type& alloc = allocator_type{})
                : set{comp, alloc}
            {
                insert(first, last);
            }

            set(const set& other)
                : set{other, other.allocator_}
            { /* DUMMY BODY */ }

            set(set&& other)
                : tree_{move(other.tree_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit set(const allocator_type& alloc)
                : tree_{}, allocator_{alloc}
            { /* DUMMY BODY */ }

            set(const set& other, const allocator_type& alloc)
                : tree_{other.tree_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            set(set&& other, const allocator_type& alloc)
                : tree_{move(other.tree_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            set(initializer_list<value_type> init,
                const key_compare& comp = key_compare{},
                const allocator_type& alloc = allocator_type{})
                : set{comp, alloc}
            {
                insert(init.begin(), init.end());
            }

            template<class InputIterator>
            set(InputIterator first, InputIterator last,
                const allocator_type& alloc)
                : set{first, last, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            set(initializer_list<value_type> init,
                const allocator_type& alloc)
                : set{init, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            ~set()
            { /* DUMMY BODY */ }

            set& operator=(const set& other)
            {
                tree_ = other.tree_;
                allocator_ = other.allocator_;

                return *this;
            }

            set& operator=(set&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<key_compare>::value)
            {
                tree_ = move(other.tree_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            set& operator=(initializer_list<value_type>& init)
            {
                tree_.clear();

                insert(init.begin(), init.end());

                return *this;
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_;
            }

            iterator begin() noexcept
            {
                return tree_.begin();
            }

            const_iterator begin() const noexcept
            {
                return tree_.begin();
            }

            iterator end() noexcept
            {
                return tree_.end();
            }

            const_iterator end() const noexcept
            {
                return tree_.end();
            }

            reverse_iterator rbegin() noexcept
            {
                return tree_.rbegin();
            }

            const_reverse_iterator rbegin() const noexcept
            {
                return tree_.rbegin();
            }

            reverse_iterator rend() noexcept
            {
                return tree_.rend();
            }

            const_reverse_iterator rend() const noexcept
            {
                return tree_.rend();
            }

            const_iterator cbegin() const noexcept
            {
                return tree_.cbegin();
            }

            const_iterator cend() const noexcept
            {
                return tree_.cend();
            }

            const_reverse_iterator crbegin() const noexcept
            {
                return tree_.crbegin();
            }

            const_reverse_iterator crend() const noexcept
            {
                return tree_.crend();
            }

            bool empty() const noexcept
            {
                return tree_.empty();
            }

            size_type size() const noexcept
            {
                return tree_.size();
            }

            size_type max_size() const noexcept
            {
                return tree_.max_size(allocator_);
            }

            template<class... Args>
            pair<iterator, bool> emplace(Args&&... args)
            {
                return tree_.emplace(forward<Args>(args)...);
            }

            template<class... Args>
            iterator emplace_hint(const_iterator, Args&&... args)
            {
                return emplace(forward<Args>(args)...).first;
            }

            pair<iterator, bool> insert(const value_type& val)
            {
                return tree_.insert(val);
            }

            pair<iterator, bool> insert(value_type&& val)
            {
                return tree_.insert(forward<value_type>(val));
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
                return tree_.erase(position);
            }

            size_type erase(const key_type& key)
            {
                return tree_.erase(key);
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                while (first != last)
                    first = erase(first);

                return iterator{
                    first.node(), first.end()
                };
            }

            void swap(set& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         noexcept(std::swap(declval<key_compare>(), declval<key_compare>())))
            {
                tree_.swap(other.tree_);
                std::swap(allocator_, other.allocator_);
            }

            void clear() noexcept
            {
                tree_.clear();
            }

            key_compare key_comp() const
            {
                return tree_.key_comp();
            }

            value_compare value_comp() const
            {
                return tree_.value_comp();
            }

            iterator find(const key_type& key)
            {
                return tree_.find(key);
            }

            const_iterator find(const key_type& key) const
            {
                return tree_.find(key);
            }

            template<class K>
            iterator find(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.find(key);
            }

            template<class K>
            const_iterator find(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.find(key);
            }

            size_type count(const key_type& key) const
            {
                return tree_.count(key);
            }

            template<class K>
            size_type count(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.count(key);
            }

            iterator lower_bound(const key_type& key)
            {
                return tree_.lower_bound(key);
            }

            const_iterator lower_bound(const key_type& key) const
            {
                return tree_.lower_bound(key);
            }

            template<class K>
            iterator lower_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.lower_bound(key);
            }

            template<class K>
            const_iterator lower_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.lower_bound(key);
            }

            iterator upper_bound(const key_type& key)
            {
                return tree_.upper_bound(key);
            }

            const_iterator upper_bound(const key_type& key) const
            {
                return tree_.upper_bound(key);
            }

            template<class K>
            iterator upper_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.upper_bound(key);
            }

            template<class K>
            const_iterator upper_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.upper_bound(key);
            }

            pair<iterator, iterator> equal_range(const key_type& key)
            {
                return tree_.equal_range(key);
            }

            pair<const_iterator, const_iterator> equal_range(const key_type& key) const
            {
                return tree_.equal_range(key);
            }

            template<class K>
            pair<iterator, iterator> equal_range(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.equal_range(key);
            }

            template<class K>
            pair<const_iterator, const_iterator> equal_range(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.equal_range(key);
            }

        private:
            using tree_type = aux::rbtree<
                key_type, key_type, aux::key_no_value_key_extractor<key_type>,
                key_compare, allocator_type, size_type,
                iterator, const_iterator,
                aux::rbtree_single_policy, node_type
            >;

            tree_type tree_;
            allocator_type allocator_;

            template<class K, class C, class A>
            friend bool operator==(const set<K, C, A>&,
                                   const set<K, C, A>&);
    };

    template<class Key, class Compare, class Allocator>
    bool operator==(const set<Key, Compare, Allocator>& lhs,
                    const set<Key, Compare, Allocator>& rhs)
    {
        return lhs.tree_.is_eq_to(rhs.tree_);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<(const set<Key, Compare, Allocator>& lhs,
                   const set<Key, Compare, Allocator>& rhs)
    {
        return lexicographical_compare(
            lhs.begin(), lhs.end(),
            rhs.begin(), rhs.end(),
            lhs.key_comp()
        );
    }

    template<class Key, class Compare, class Allocator>
    bool operator!=(const set<Key, Compare, Allocator>& lhs,
                    const set<Key, Compare, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator>(const set<Key, Compare, Allocator>& lhs,
                   const set<Key, Compare, Allocator>& rhs)
    {
        return rhs < lhs;
    }

    template<class Key, class Compare, class Allocator>
    bool operator>=(const set<Key, Compare, Allocator>& lhs,
                    const set<Key, Compare, Allocator>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<=(const set<Key, Compare, Allocator>& lhs,
                    const set<Key, Compare, Allocator>& rhs)
    {
        return !(rhs < lhs);
    }

    /**
     * 23.4.7, class template multiset:
     */

    template<
        class Key,
        class Compare = less<Key>,
        class Alloc = allocator<Key>
    >
    class multiset
    {
        public:
            using key_type        = Key;
            using value_type      = Key;
            using key_compare     = Compare;
            using value_compare   = Compare;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            using node_type = aux::rbtree_multi_node<value_type>;

            /**
             * Note: Both the iterator and const_iterator types are constant
             *       iterators, the standard does not require them
             *       to be the same type, but why not? :)
             */
            using iterator             = aux::rbtree_const_iterator<
                value_type, const_reference, const_pointer, size_type, node_type
            >;
            using const_iterator       = iterator;

            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            multiset()
                : multiset{key_compare{}}
            { /* DUMMY BODY */ }

            explicit multiset(const key_compare& comp,
                              const allocator_type& alloc = allocator_type{})
                : tree_{comp}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            multiset(InputIterator first, InputIterator last,
                     const key_compare& comp = key_compare{},
                     const allocator_type& alloc = allocator_type{})
                : multiset{comp, alloc}
            {
                insert(first, last);
            }

            multiset(const multiset& other)
                : multiset{other, other.allocator_}
            { /* DUMMY BODY */ }

            multiset(multiset&& other)
                : tree_{move(other.tree_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit multiset(const allocator_type& alloc)
                : tree_{}, allocator_{alloc}
            { /* DUMMY BODY */ }

            multiset(const multiset& other, const allocator_type& alloc)
                : tree_{other.tree_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            multiset(multiset&& other, const allocator_type& alloc)
                : tree_{move(other.tree_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            multiset(initializer_list<value_type> init,
                     const key_compare& comp = key_compare{},
                     const allocator_type& alloc = allocator_type{})
                : multiset{comp, alloc}
            {
                insert(init.begin(), init.end());
            }

            template<class InputIterator>
            multiset(InputIterator first, InputIterator last,
                     const allocator_type& alloc)
                : multiset{first, last, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            multiset(initializer_list<value_type> init,
                     const allocator_type& alloc)
                : multiset{init, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            ~multiset()
            { /* DUMMY BODY */ }

            multiset& operator=(const multiset& other)
            {
                tree_ = other.tree_;
                allocator_ = other.allocator_;

                return *this;
            }

            multiset& operator=(multiset&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<key_compare>::value)
            {
                tree_ = move(other.tree_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            multiset& operator=(initializer_list<value_type>& init)
            {
                tree_.clear();

                insert(init.begin(), init.end());

                return *this;
            }

            allocator_type get_allocator() const noexcept
            {
                return allocator_;
            }

            iterator begin() noexcept
            {
                return tree_.begin();
            }

            const_iterator begin() const noexcept
            {
                return tree_.begin();
            }

            iterator end() noexcept
            {
                return tree_.end();
            }

            const_iterator end() const noexcept
            {
                return tree_.end();
            }

            reverse_iterator rbegin() noexcept
            {
                return tree_.rbegin();
            }

            const_reverse_iterator rbegin() const noexcept
            {
                return tree_.rbegin();
            }

            reverse_iterator rend() noexcept
            {
                return tree_.rend();
            }

            const_reverse_iterator rend() const noexcept
            {
                return tree_.rend();
            }

            const_iterator cbegin() const noexcept
            {
                return tree_.cbegin();
            }

            const_iterator cend() const noexcept
            {
                return tree_.cend();
            }

            const_reverse_iterator crbegin() const noexcept
            {
                return tree_.crbegin();
            }

            const_reverse_iterator crend() const noexcept
            {
                return tree_.crend();
            }

            bool empty() const noexcept
            {
                return tree_.empty();
            }

            size_type size() const noexcept
            {
                return tree_.size();
            }

            size_type max_size() const noexcept
            {
                return tree_.max_size(allocator_);
            }

            template<class... Args>
            iterator emplace(Args&&... args)
            {
                return tree_.emplace(forward<Args>(args)...);
            }

            template<class... Args>
            iterator emplace_hint(const_iterator, Args&&... args)
            {
                return emplace(forward<Args>(args)...);
            }

            iterator insert(const value_type& val)
            {
                return tree_.insert(val);
            }

            iterator insert(value_type&& val)
            {
                return tree_.insert(forward<value_type>(val));
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
                return tree_.erase(position);
            }

            size_type erase(const key_type& key)
            {
                return tree_.erase(key);
            }

            iterator erase(const_iterator first, const_iterator last)
            {
                while (first != last)
                    first = erase(first);

                return iterator{
                    first.node(), first.end()
                };
            }

            void swap(multiset& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         noexcept(std::swap(declval<key_compare>(), declval<key_compare>())))
            {
                tree_.swap(other.tree_);
                std::swap(allocator_, other.allocator_);
            }

            void clear() noexcept
            {
                tree_.clear();
            }

            key_compare key_comp() const
            {
                return tree_.key_comp();
            }

            value_compare value_comp() const
            {
                return tree_.value_comp();
            }

            iterator find(const key_type& key)
            {
                return tree_.find(key);
            }

            const_iterator find(const key_type& key) const
            {
                return tree_.find(key);
            }

            template<class K>
            iterator find(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.find(key);
            }

            template<class K>
            const_iterator find(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.find(key);
            }

            size_type count(const key_type& key) const
            {
                return tree_.count(key);
            }

            template<class K>
            size_type count(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.count(key);
            }

            iterator lower_bound(const key_type& key)
            {
                return tree_.lower_bound(key);
            }

            const_iterator lower_bound(const key_type& key) const
            {
                return tree_.lower_bound(key);
            }

            template<class K>
            iterator lower_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.lower_bound(key);
            }

            template<class K>
            const_iterator lower_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.lower_bound(key);
            }

            iterator upper_bound(const key_type& key)
            {
                return tree_.upper_bound(key);
            }

            const_iterator upper_bound(const key_type& key) const
            {
                return tree_.upper_bound(key);
            }

            template<class K>
            iterator upper_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.upper_bound(key);
            }

            template<class K>
            const_iterator upper_bound(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.upper_bound(key);
            }

            pair<iterator, iterator> equal_range(const key_type& key)
            {
                return tree_.equal_range(key);
            }

            pair<const_iterator, const_iterator> equal_range(const key_type& key) const
            {
                return tree_.equal_range(key);
            }

            template<class K>
            pair<iterator, iterator> equal_range(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            )
            {
                return tree_.equal_range(key);
            }

            template<class K>
            pair<const_iterator, const_iterator> equal_range(
                const K& key,
                enable_if_t<aux::is_transparent_v<key_compare>, K>* = nullptr
            ) const
            {
                return tree_.equal_range(key);
            }

        private:
            using tree_type = aux::rbtree<
                key_type, key_type, aux::key_no_value_key_extractor<key_type>,
                key_compare, allocator_type, size_type,
                iterator, const_iterator,
                aux::rbtree_multi_policy, node_type
            >;

            tree_type tree_;
            allocator_type allocator_;

            template<class K, class C, class A>
            friend bool operator==(const multiset<K, C, A>&,
                                   const multiset<K, C, A>&);
    };

    template<class Key, class Compare, class Allocator>
    bool operator==(const multiset<Key, Compare, Allocator>& lhs,
                    const multiset<Key, Compare, Allocator>& rhs)
    {
        return lhs.tree_.is_eq_to(rhs.tree_);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<(const multiset<Key, Compare, Allocator>& lhs,
                   const multiset<Key, Compare, Allocator>& rhs)
    {
        return lexicographical_compare(
            lhs.begin(), lhs.end(),
            rhs.begin(), rhs.end(),
            lhs.value_comp()
        );
    }

    template<class Key, class Compare, class Allocator>
    bool operator!=(const multiset<Key, Compare, Allocator>& lhs,
                    const multiset<Key, Compare, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator>(const multiset<Key, Compare, Allocator>& lhs,
                   const multiset<Key, Compare, Allocator>& rhs)
    {
        return rhs < lhs;
    }

    template<class Key, class Compare, class Allocator>
    bool operator>=(const multiset<Key, Compare, Allocator>& lhs,
                    const multiset<Key, Compare, Allocator>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<=(const multiset<Key, Compare, Allocator>& lhs,
                    const multiset<Key, Compare, Allocator>& rhs)
    {
        return !(rhs < lhs);
    }
}

#endif
