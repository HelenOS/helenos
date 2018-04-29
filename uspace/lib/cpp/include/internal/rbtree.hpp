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

#ifndef LIBCPP_INTERNAL_RBTREE
#define LIBCPP_INTERNAL_RBTREE

#include <internal/key_extractors.hpp>
#include <internal/rbtree_iterators.hpp>
#include <internal/rbtree_node.hpp>
#include <internal/rbtree_policies.hpp>

namespace std::aux
{
    template<
        class Value, class Key, class KeyExtractor,
        class KeyComp, class Alloc, class Size,
        class Iterator, class ConstIterator,
        class Policy
    >
    class rbtree
    {
        public:
            using value_type     = Value;
            using key_type       = Key;
            using size_type      = Size;
            using allocator_type = Alloc;
            using key_compare    = KeyComp;
            using key_extract    = KeyExtractor;

            using iterator             = Iterator;
            using const_iterator       = ConstIterator;

            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            using node_type = rbtree_node<value_type>;

            // TODO: make find/bounds etc templated with key type
            //       for transparent comparators and leave their management for the
            //       outer containers

            rbtree(const key_compare& kcmp = key_compare{})
                : root_{nullptr}, size_{}, key_compare_{},
                  key_extractor_{}
            { /* DUMMY BODY */ }

            rbtree(const rbtree& other)
                : rbtree{other.key_compare_}
            {
                for (const auto& x: other)
                    insert(x);
            }

            rbtree(rbtree&& other)
                : root_{other.root_}, size_{other.size_},
                  key_compare_{move(other.key_compare_)},
                  key_extractor_{move(other.key_extractor_)}
            {
                other.root_ = nullptr;
                other.size_ = size_type{};
            }

            rbtree& operator=(const rbtree& other)
            {
                auto tmp{other};
                tmp.swap(*this);

                return *this;
            }

            rbtree& operator=(rbtree&& other)
            {
                rbtree tmp{move(other)};
                tmp.swap(*this);

                return *this;
            }

            bool empty() const noexcept
            {
                return size_;
            }

            size_type size() const noexcept
            {
                return size_;
            }

            size_type max_size(allocator_type& alloc)
            {
                return allocator_traits<allocator_type>::max_size(alloc);
            }

            iterator begin()
            {
                return iterator{find_smallest_(), false};
            }

            const_iterator begin() const
            {
                return cbegin();
            }

            iterator end()
            {
                return iterator{find_largest_(), true};
            }

            const_iterator end() const
            {
                return cend();
            }

            reverse_iterator rbegin()
            {
                return make_reverse_iterator(end());
            }

            const_reverse_iterator rbegin() const
            {
                return make_reverse_iterator(cend());
            }

            reverse_iterator rend()
            {
                return make_reverse_iterator(begin());
            }

            const_reverse_iterator rend() const
            {
                return make_reverse_iterator(cbegin());
            }

            const_iterator cbegin() const
            {
                return const_iterator{find_smallest_(), false};
            }

            const_iterator cend() const
            {
                return const_iterator{find_largest_(), true};
            }

            const_reverse_iterator crbegin() const
            {
                return make_reverse_iterator(cend());
            }

            const_reverse_iterator crend() const
            {
                return make_reverse_iterator(cbegin());
            }

            template<class... Args>
            pair<iterator, bool> emplace(Args&&... args)
            {
                auto ret = Policy::emplace(*this, forward<Args>(args)...);
                if (!ret.second)
                    return ret;
                ++size_;

                repair_after_insert_(ret.first.node());
                update_root_(ret.first.node());

                return ret;
            }

            pair<iterator, bool> insert(const value_type& val)
            {
                auto ret = Policy::insert(*this, val);
                if (!ret.second)
                    return ret;
                ++size_;

                repair_after_insert_(ret.first.node());
                update_root_(ret.first.node());

                return ret;
            }

            pair<iterator, bool> insert(value_type&& val)
            {
                auto ret = Policy::insert(*this, forward<value_type>(val));
                if (!ret.second)
                    return ret;
                ++size_;

                repair_after_insert_(ret.first.node());
                update_root_(ret.first.node());

                return ret;
            }

            size_type erase(const key_type& key)
            {
                return Policy::erase(*this, key);
            }

            iterator erase(const_iterator it)
            {
                if (it == cend())
                    return end();

                auto node = const_cast<node_type*>(it.node());
                ++it;

                delete_node(node);
                return iterator{const_cast<node_type*>(it.node()), it.end()};
            }

            void clear() noexcept
            {
                if (root_)
                {
                    delete root_;
                    root_ = nullptr;
                    size_ = size_type{};
                }
            }

            void swap(rbtree& other)
                noexcept(allocator_traits<allocator_type>::is_always_equal::value &&
                         noexcept(swap(declval<KeyComp&>(), declval<KeyComp&>())))
            {
                std::swap(root_, other.root_);
                std::swap(size_, other.size_);
                std::swap(key_compare_, other.key_compare_);
                std::swap(key_extractor_, other.key_extractor_);
            }

            key_compare key_comp() const
            {
                return key_compare_;
            }

            iterator find(const key_type& key)
            {
                auto node = find_(key);
                if (node)
                    return iterator{node, false};
                else
                    return end();
            }

            const_iterator find(const key_type& key) const
            {
                auto node = find_(key);
                if (node)
                    return const_iterator{node, false};
                else
                    return end();
            }

            size_type count(const key_type& key) const
            {
                return Policy::count(*this, key);
            }

            iterator upper_bound(const key_type& key)
            {
                return Policy::upper_bound(*this, key);
            }

            const_iterator upper_bound(const key_type& key) const
            {
                return Policy::upper_bound_const(*this, key);
            }

            iterator lower_bound(const key_type& key)
            {
                return Policy::lower_bound(*this, key);
            }

            const_iterator lower_bound(const key_type& key) const
            {
                return Policy::lower_bound_const(*this, key);
            }

            pair<iterator, iterator> equal_range(const key_type& key)
            {
                return Policy::equal_range(*this, key);
            }

            pair<const_iterator, const_iterator> equal_range(const key_type& key) const
            {
                return Policy::equal_range_const(*this, key);
            }

            bool is_eq_to(const rbtree& other) const
            {
                if (size_ != other.size())
                    return false;

                auto it1 = begin();
                auto it2 = other.begin();

                while (keys_equal(*it1++, *it2++))
                { /* DUMMY BODY */ }

                return (it1 == end()) && (it2 == other.end());
            }

            const key_type& get_key(const value_type& val) const
            {
                return key_extractor_(val);
            }

            bool keys_comp(const key_type& key, const value_type& val) const
            {
                return key_compare_(key, key_extractor_(val));
            }

            bool keys_equal(const key_type& k1, const key_type& k2) const
            {
                return !key_compare_(k1, k2) && !key_compare_(k2, k1);
            }

            node_type* find_parent_for_insertion(const value_type& val) const
            {
                auto current = root_;
                auto parent = current;

                while (current)
                {
                    parent = current;
                    if (key_compare_(key_extractor_(val), key_extractor_(current->value)))
                        current = current->left;
                    else
                        current = current->right;
                }

                return parent;
            }

            void delete_node(node_type* node)
            {
                if (!node)
                    return;

                --size_;

                if (node->left && node->right)
                {
                    node->swap(node->successor());

                    // Node now has at most one child.
                    delete_node(node);

                    return;
                }

                auto child = node->right ? node->right : node->left;
                if (!child)
                {
                    // Simply remove the node.
                    node->unlink();

                    delete node;
                }
                else
                {
                    // Replace with the child.
                    child->parent = node->parent;
                    if (node->is_left_child())
                        child->parent->left = child;
                    else if (node->is_left_child())
                        child->parent->right = child;

                    // Repair if needed.
                    repair_after_erase_(node, child);
                    update_root_(child);
                }
            }

        private:
            node_type* root_;
            size_type size_;
            key_compare key_compare_;
            key_extract key_extractor_;

            node_type* find_(const key_type& key) const
            {
                auto current = root_;
                while (current != nullptr)
                {
                    if (key_compare_(key, key_extractor_(current->value)))
                        current = current->left;
                    else if (key_compare_(key_extractor_(current->value), key))
                        current = current->right;
                    else
                        return current;
                }

                return nullptr;
            }

            node_type* find_smallest_() const
            {
                if (root_)
                    return root_->find_smallest();
                else
                    return nullptr;
            }

            node_type* find_largest_() const
            {
                if (root_)
                    return root_->find_largest();
                else
                    return nullptr;
            }

            void update_root_(node_type* node)
            {
                if (!node)
                    return;

                root_ = node;
                while (root_->parent)
                    root_ = root_->parent;
            }

            void repair_after_insert_(node_type* node)
            {
                // TODO: implement
            }

            void repair_after_erase_(node_type* node, node_type* child)
            {
                // TODO: implement
            }

            friend Policy;
    };
}

#endif
