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

#ifndef LIBCPP_BITS_ADT_MAP
#define LIBCPP_BITS_ADT_MAP

#include <__bits/adt/rbtree.hpp>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>
#include <type_traits>

namespace std
{
    /**
     * 23.4.4, class template map:
     */

    template<
        class Key, class Value,
        class Compare = less<Key>,
        class Alloc = allocator<pair<const Key, Value>>
    >
    class map
    {
        public:
            using key_type        = Key;
            using mapped_type     = Value;
            using value_type      = pair<const key_type, mapped_type>;
            using key_compare     = Compare;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            using node_type = aux::rbtree_single_node<value_type>;

            using iterator             = aux::rbtree_iterator<
                value_type, reference, pointer, size_type, node_type
            >;
            using const_iterator       = aux::rbtree_const_iterator<
                value_type, const_reference, const_pointer, size_type, node_type
            >;

            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            class value_compare
            {
                friend class map;

                protected:
                    key_compare comp;

                    value_compare(key_compare c)
                        : comp{c}
                    { /* DUMMY BODY */ }

                public:
                    using result_type          = bool;
                    using first_argument_type  = value_type;
                    using second_argument_type = value_type;

                    bool operator()(const value_type& lhs, const value_type& rhs) const
                    {
                        return comp(lhs.first, rhs.first);
                    }
            };

            /**
             * 24.4.4.2, construct/copy/destroy:
             */

            map()
                : map{key_compare{}}
            { /* DUMMY BODY */ }

            explicit map(const key_compare& comp,
                         const allocator_type& alloc = allocator_type{})
                : tree_{comp}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            map(InputIterator first, InputIterator last,
                const key_compare& comp = key_compare{},
                const allocator_type& alloc = allocator_type{})
                : map{comp, alloc}
            {
                insert(first, last);
            }

            map(const map& other)
                : map{other, other.allocator_}
            { /* DUMMY BODY */ }

            map(map&& other)
                : tree_{move(other.tree_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit map(const allocator_type& alloc)
                : tree_{}, allocator_{alloc}
            { /* DUMMY BODY */ }

            map(const map& other, const allocator_type& alloc)
                : tree_{other.tree_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            map(map&& other, const allocator_type& alloc)
                : tree_{move(other.tree_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            map(initializer_list<value_type> init,
                const key_compare& comp = key_compare{},
                const allocator_type& alloc = allocator_type{})
                : map{comp, alloc}
            {
                insert(init.begin(), init.end());
            }

            template<class InputIterator>
            map(InputIterator first, InputIterator last,
                const allocator_type& alloc)
                : map{first, last, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            map(initializer_list<value_type> init,
                const allocator_type& alloc)
                : map{init, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            ~map()
            { /* DUMMY BODY */ }

            map& operator=(const map& other)
            {
                tree_ = other.tree_;
                allocator_ = other.allocator_;

                return *this;
            }

            map& operator=(map&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<key_compare>::value)
            {
                tree_ = move(other.tree_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            map& operator=(initializer_list<value_type>& init)
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

            /**
             * 23.4.4.3, element access:
             */

            mapped_type& operator[](const key_type& key)
            {
                auto parent = tree_.find_parent_for_insertion(key);
                if (parent && tree_.keys_equal(tree_.get_key(parent->value), key))
                    return parent->value.second;

                auto node = new node_type{value_type{key, mapped_type{}}};
                tree_.insert_node(node, parent);

                return node->value.second;
            }

            mapped_type& operator[](key_type&& key)
            {
                auto parent = tree_.find_parent_for_insertion(key);
                if (parent && tree_.keys_equal(tree_.get_key(parent->value), key))
                    return parent->value.second;

                auto node = new node_type{value_type{move(key), mapped_type{}}};
                tree_.insert_node(node, parent);

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

            /**
             * 23.4.4.4, modifiers:
             */

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
                auto parent = tree_.find_parent_for_insertion(key);
                if (parent && tree_.keys_equal(tree_.get_key(parent->value), key))
                    return make_pair(iterator{parent, false}, false);
                else
                {
                    auto node = new node_type{value_type{key, forward<Args>(args)...}};
                    tree_.insert_node(node, parent);

                    return make_pair(iterator{node, false}, true);
                }
            }

            template<class... Args>
            pair<iterator, bool> try_emplace(key_type&& key, Args&&... args)
            {
                auto parent = tree_.find_parent_for_insertion(key);
                if (parent && tree_.keys_equal(tree_.get_key(parent->value), key))
                    return make_pair(iterator{parent, false}, false);
                else
                {
                    auto node = new node_type{value_type{move(key), forward<Args>(args)...}};
                    tree_.insert_node(node, parent);

                    return make_pair(iterator{node, false}, true);
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
                auto parent = tree_.find_parent_for_insertion(key);
                if (parent && tree_.keys_equal(tree_.get_key(parent->value), key))
                {
                    parent->value.second = forward<T>(val);

                    return make_pair(iterator{parent, false}, false);
                }
                else
                {
                    auto node = new node_type{value_type{key, forward<T>(val)}};
                    tree_.insert_node(node, parent);

                    return make_pair(iterator{node, false}, true);
                }
            }

            template<class T>
            pair<iterator, bool> insert_or_assign(key_type&& key, T&& val)
            {
                auto parent = tree_.find_parent_for_insertion(key);
                if (parent && tree_.keys_equal(tree_.get_key(parent->value), key))
                {
                    parent->value.second = forward<T>(val);

                    return make_pair(iterator{parent, false}, false);
                }
                else
                {
                    auto node = new node_type{value_type{move(key), forward<T>(val)}};
                    tree_.insert_node(node, parent);

                    return make_pair(iterator{node, false}, true);
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

            void swap(map& other)
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
                return value_compare{tree_.key_comp()};
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
                value_type, key_type, aux::key_value_key_extractor<key_type, mapped_type>,
                key_compare, allocator_type, size_type,
                iterator, const_iterator,
                aux::rbtree_single_policy, node_type
            >;

            tree_type tree_;
            allocator_type allocator_;

            template<class K, class C, class A>
            friend bool operator==(const map<K, C, A>&,
                                   const map<K, C, A>&);
    };

    template<class Key, class Compare, class Allocator>
    bool operator==(const map<Key, Compare, Allocator>& lhs,
                    const map<Key, Compare, Allocator>& rhs)
    {
        return lhs.tree_.is_eq_to(rhs.tree_);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<(const map<Key, Compare, Allocator>& lhs,
                   const map<Key, Compare, Allocator>& rhs)
    {
        return lexicographical_compare(
            lhs.begin(), lhs.end(),
            rhs.begin(), rhs.end(),
            lhs.value_comp()
        );
    }

    template<class Key, class Compare, class Allocator>
    bool operator!=(const map<Key, Compare, Allocator>& lhs,
                    const map<Key, Compare, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator>(const map<Key, Compare, Allocator>& lhs,
                   const map<Key, Compare, Allocator>& rhs)
    {
        return rhs < lhs;
    }

    template<class Key, class Compare, class Allocator>
    bool operator>=(const map<Key, Compare, Allocator>& lhs,
                    const map<Key, Compare, Allocator>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<=(const map<Key, Compare, Allocator>& lhs,
                    const map<Key, Compare, Allocator>& rhs)
    {
        return !(rhs < lhs);
    }

    /**
     * 23.4.5, class template multimap:
     */

    template<
        class Key, class Value,
        class Compare = less<Key>,
        class Alloc = allocator<pair<const Key, Value>>
    >
    class multimap
    {
        public:
            using key_type        = Key;
            using mapped_type     = Value;
            using value_type      = pair<const key_type, mapped_type>;
            using key_compare     = Compare;
            using allocator_type  = Alloc;
            using pointer         = typename allocator_traits<allocator_type>::pointer;
            using const_pointer   = typename allocator_traits<allocator_type>::const_pointer;
            using reference       = value_type&;
            using const_reference = const value_type&;
            using size_type       = size_t;
            using difference_type = ptrdiff_t;

            using node_type = aux::rbtree_multi_node<value_type>;

            class value_compare
            {
                friend class multimap;

                protected:
                    key_compare comp;

                    value_compare(key_compare c)
                        : comp{c}
                    { /* DUMMY BODY */ }

                public:
                    using result_type          = bool;
                    using first_argument_type  = value_type;
                    using second_argument_type = value_type;

                    bool operator()(const value_type& lhs, const value_type& rhs) const
                    {
                        return comp(lhs.first, rhs.first);
                    }
            };

            using iterator             = aux::rbtree_iterator<
                value_type, reference, pointer, size_type, node_type
            >;
            using const_iterator       = aux::rbtree_const_iterator<
                value_type, const_reference, const_pointer, size_type, node_type
            >;

            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            multimap()
                : multimap{key_compare{}}
            { /* DUMMY BODY */ }

            explicit multimap(const key_compare& comp,
                              const allocator_type& alloc = allocator_type{})
                : tree_{comp}, allocator_{alloc}
            { /* DUMMY BODY */ }

            template<class InputIterator>
            multimap(InputIterator first, InputIterator last,
                     const key_compare& comp = key_compare{},
                     const allocator_type& alloc = allocator_type{})
                : multimap{comp, alloc}
            {
                insert(first, last);
            }

            multimap(const multimap& other)
                : multimap{other, other.allocator_}
            { /* DUMMY BODY */ }

            multimap(multimap&& other)
                : tree_{move(other.tree_)}, allocator_{move(other.allocator_)}
            { /* DUMMY BODY */ }

            explicit multimap(const allocator_type& alloc)
                : tree_{}, allocator_{alloc}
            { /* DUMMY BODY */ }

            multimap(const multimap& other, const allocator_type& alloc)
                : tree_{other.tree_}, allocator_{alloc}
            { /* DUMMY BODY */ }

            multimap(multimap&& other, const allocator_type& alloc)
                : tree_{move(other.tree_)}, allocator_{alloc}
            { /* DUMMY BODY */ }

            multimap(initializer_list<value_type> init,
                     const key_compare& comp = key_compare{},
                     const allocator_type& alloc = allocator_type{})
                : multimap{comp, alloc}
            {
                insert(init.begin(), init.end());
            }

            template<class InputIterator>
            multimap(InputIterator first, InputIterator last,
                     const allocator_type& alloc)
                : multimap{first, last, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            multimap(initializer_list<value_type> init,
                     const allocator_type& alloc)
                : multimap{init, key_compare{}, alloc}
            { /* DUMMY BODY */ }

            ~multimap()
            { /* DUMMY BODY */ }

            multimap& operator=(const multimap& other)
            {
                tree_ = other.tree_;
                allocator_ = other.allocator_;

                return *this;
            }

            multimap& operator=(multimap&& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         is_nothrow_move_assignable<key_compare>::value)
            {
                tree_ = move(other.tree_);
                allocator_ = move(other.allocator_);

                return *this;
            }

            multimap& operator=(initializer_list<value_type>& init)
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

            void swap(multimap& other)
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
                return value_compare{tree_.key_comp()};
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
                value_type, key_type, aux::key_value_key_extractor<key_type, mapped_type>,
                key_compare, allocator_type, size_type,
                iterator, const_iterator,
                aux::rbtree_multi_policy, node_type
            >;

            tree_type tree_;
            allocator_type allocator_;

            template<class K, class C, class A>
            friend bool operator==(const multimap<K, C, A>&,
                                   const multimap<K, C, A>&);
    };

    template<class Key, class Compare, class Allocator>
    bool operator==(const multimap<Key, Compare, Allocator>& lhs,
                    const multimap<Key, Compare, Allocator>& rhs)
    {
        return lhs.tree_.is_eq_to(rhs.tree_);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<(const multimap<Key, Compare, Allocator>& lhs,
                   const multimap<Key, Compare, Allocator>& rhs)
    {
        return lexicographical_compare(
            lhs.begin(), lhs.end(),
            rhs.begin(), rhs.end(),
            lhs.value_comp()
        );
    }

    template<class Key, class Compare, class Allocator>
    bool operator!=(const multimap<Key, Compare, Allocator>& lhs,
                    const multimap<Key, Compare, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator>(const multimap<Key, Compare, Allocator>& lhs,
                   const multimap<Key, Compare, Allocator>& rhs)
    {
        return rhs < lhs;
    }

    template<class Key, class Compare, class Allocator>
    bool operator>=(const multimap<Key, Compare, Allocator>& lhs,
                    const multimap<Key, Compare, Allocator>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class Key, class Compare, class Allocator>
    bool operator<=(const multimap<Key, Compare, Allocator>& lhs,
                    const multimap<Key, Compare, Allocator>& rhs)
    {
        return !(rhs < lhs);
    }
}

#endif
