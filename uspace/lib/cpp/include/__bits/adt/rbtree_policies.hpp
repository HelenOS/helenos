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

#ifndef LIBCPP_BITS_ADT_RBTREE_POLICIES
#define LIBCPP_BITS_ADT_RBTREE_POLICIES

#include <__bits/adt/rbtree_node.hpp>
#include <utility>

namespace std::aux
{
    struct rbtree_single_policy
    {
        template<class Tree, class Key>
        static typename Tree::size_type count(const Tree& tree, const Key& key)
        {
            return tree.find(key) == tree.end() ? 0 : 1;
        }

        template<class Tree, class Key>
        static typename Tree::size_type erase(Tree& tree, const Key& key)
        {
            using size_type = typename Tree::size_type;

            auto it = tree.find(key);
            if (it == tree.end())
                return size_type{};
            else
                tree.delete_node(it.node());
            return size_type{1};
        }

        template<class Tree, class Key>
        static typename Tree::iterator lower_bound(const Tree& tree, const Key& key)
        {
            using iterator  = typename Tree::iterator;
            using node_type = typename Tree::node_type;

            auto it = lower_bound_const(tree, key);

            return iterator{const_cast<node_type*>(it.node()), it.end()};
        }

        template<class Tree, class Key>
        static typename Tree::const_iterator lower_bound_const(const Tree& tree, const Key& key)
        {
            using const_iterator = typename Tree::const_iterator;

            auto node = tree.find_parent_for_insertion(key);
            const_iterator it{node, false};
            auto beg = tree.begin();
            auto end = tree.end();

            if (tree.key_compare_(tree.get_key(*it), key))
            {
                // Predecessor.
                if (it != end)
                    return ++it;
                else
                    return it;
            }
            else if (tree.key_compare_(key, tree.get_key(*it)))
            {
                // Successor.
                if (it != beg)
                    return --it;
                else
                    return it;
            }
            else // Perfect match.
                return it;

            return it;
        }

        template<class Tree, class Key>
        static typename Tree::iterator upper_bound(const Tree& tree, const Key& key)
        {
            using iterator  = typename Tree::iterator;
            using node_type = typename Tree::node_type;

            auto it = upper_bound_const(tree, key);

            return iterator{const_cast<node_type*>(it.node()), it.end()};
        }

        template<class Tree, class Key>
        static typename Tree::const_iterator upper_bound_const(const Tree& tree, const Key& key)
        {
            /**
             * If key isn't in the tree, we get it's
             * predecessor or tree.end(). If key is
             * in the tree, we get it. So unless it
             * is equal to end(), we can increment it
             * to get the upper bound.
             */
            auto it = lower_bound_const(tree, key);
            if (it == tree.end())
                return it;
            else
                return ++it;
        }

        template<class Tree, class Key>
        static pair<
            typename Tree::iterator,
            typename Tree::iterator
        > equal_range(Tree& tree, const Key& key)
        {
            return make_pair(
                lower_bound(tree, key),
                upper_bound(tree, key)
            );
        }

        template<class Tree, class Key>
        static pair<
            typename Tree::const_iterator,
            typename Tree::const_iterator
        > equal_range_const(const Tree& tree, const Key& key)
        {
            return make_pair(
                lower_bound_const(tree, key),
                upper_bound_const(tree, key)
            );
        }

        /**
         * Note: We have to duplicate code for emplace, insert(const&)
         *       and insert(&&) here, because the node (which makes distinction
         *       between the arguments) is only created if the value isn't
         *       in the tree already.
         */

        template<class Tree, class... Args>
        static pair<
            typename Tree::iterator, bool
        > emplace(Tree& tree, Args&&... args)
        {
            using value_type = typename Tree::value_type;
            using iterator   = typename Tree::iterator;
            using node_type  = typename Tree::node_type;

            auto val = value_type{forward<Args>(args)...};
            auto parent = tree.find_parent_for_insertion(tree.get_key(val));

            if (parent && tree.keys_equal(tree.get_key(parent->value), tree.get_key(val)))
                return make_pair(iterator{parent, false}, false);

            auto node = new node_type{move(val)};

            return insert(tree, node, parent);
        }

        template<class Tree, class Value>
        static pair<
            typename Tree::iterator, bool
        > insert(Tree& tree, const Value& val)
        {
            using iterator  = typename Tree::iterator;
            using node_type = typename Tree::node_type;

            auto parent = tree.find_parent_for_insertion(tree.get_key(val));
            if (parent && tree.keys_equal(tree.get_key(parent->value), tree.get_key(val)))
                return make_pair(iterator{parent, false}, false);

            auto node = new node_type{val};

            return insert(tree, node, parent);
        }

        template<class Tree, class Value>
        static pair<
            typename Tree::iterator, bool
        > insert(Tree& tree, Value&& val)
        {
            using iterator  = typename Tree::iterator;
            using node_type = typename Tree::node_type;

            auto parent = tree.find_parent_for_insertion(tree.get_key(val));
            if (parent && tree.keys_equal(tree.get_key(parent->value), tree.get_key(val)))
                return make_pair(iterator{parent, false}, false);

            auto node = new node_type{forward<Value>(val)};

            return insert(tree, node, parent);
        }

        template<class Tree>
        static pair<
            typename Tree::iterator, bool
        > insert(
            Tree& tree, typename Tree::node_type* node,
            typename Tree::node_type* parent
        )
        {
            using iterator  = typename Tree::iterator;

            if (!node)
                return make_pair(tree.end(), false);

            ++tree.size_;
            if (!parent)
            {
                node->color = rbcolor::black;
                tree.root_ = node;
            }
            else
            {
                if (tree.keys_comp(tree.get_key(node->value), parent->value))
                    parent->add_left_child(node);
                else
                    parent->add_right_child(node);

                tree.repair_after_insert_(node);
                tree.update_root_(node);
            }

            return make_pair(iterator{node, false}, true);
        }
    };

    struct rbtree_multi_policy
    {
        template<class Tree, class Key>
        static typename Tree::size_type count(const Tree& tree, const Key& key)
        {
            using size_type = typename Tree::size_type;

            auto it = tree.find(key);
            if (it == tree.end())
                return size_type{};

            size_type res{};
            while (it != tree.end() && tree.keys_equal(tree.get_key(*it), key))
            {
                ++res;
                ++it;
            }

            return res;
        }

        template<class Tree, class Key>
        static typename Tree::size_type erase(Tree& tree, const Key& key)
        {
            using size_type = typename Tree::size_type;

            auto it = tree.find(key);
            if (it == tree.end())
                return size_type{};

            size_type res{};
            while (it != tree.end() && tree.keys_equal(tree.get_key(*it), key))
            {
                ++res;
                it = tree.erase(it);
            }

            return res;
        }

        template<class Tree, class Key>
        static typename Tree::iterator lower_bound(const Tree& tree, const Key& key)
        {
            auto it = lower_bound_const(tree, key);

            return typename Tree::iterator{
                const_cast<typename Tree::node_type*>(it.node()), it.end()
            };
        }

        template<class Tree, class Key>
        static typename Tree::const_iterator lower_bound_const(const Tree& tree, const Key& key)
        {
            using const_iterator = typename Tree::const_iterator;

            auto node = tree.find_parent_for_insertion(key);
            const_iterator it{node, false};
            auto beg = tree.begin();
            auto end = tree.end();

            if (tree.keys_comp(key, *it))
                --it; // Incase we are on a successor.
            while (tree.keys_equal(tree.get_key(*it), key) && it != beg)
                --it; // Skip keys that are equal.
            if (it != beg)
                ++it; // If we moved all the way to the start, key is the smallest.

            if (tree.key_compare_(tree.get_key(*it), key))
            {
                // Predecessor.
                if (it != end)
                    return ++it;
                else
                    return it;
            }

            return it;
        }

        template<class Tree, class Key>
        static typename Tree::iterator upper_bound(const Tree& tree, const Key& key)
        {
            auto it = upper_bound_const(tree, key);

            return typename Tree::iterator{
                const_cast<typename Tree::node_type*>(it.node()), it.end()
            };
        }

        template<class Tree, class Key>
        static typename Tree::const_iterator upper_bound_const(const Tree& tree, const Key& key)
        {
            /**
             * If key isn't in the tree, we get it's
             * predecessor or tree.end(). If key is
             * in the tree, we get it. So unless it
             * is equal to end(), we keep incrementing
             * until we get to the next key.
             */
            auto it = lower_bound(tree, key);
            if (it == tree.end())
                return it;
            else if (tree.keys_equal(tree.get_key(*it), key))
            {
                while (it != tree.end() && tree.keys_equal(tree.get_key(*it), key))
                    ++it;

                return it;
            }

            return it;
        }

        template<class Tree, class Key>
        static pair<
            typename Tree::iterator,
            typename Tree::iterator
        > equal_range(const Tree& tree, const Key& key)
        {
            return make_pair(
                lower_bound(tree, key),
                upper_bound(tree, key)
            );
        }

        template<class Tree, class Key>
        static pair<
            typename Tree::const_iterator,
            typename Tree::const_iterator
        > equal_range_const(const Tree& tree, const Key& key)
        {
            return make_pair(
                lower_bound_const(tree, key),
                upper_bound_const(tree, key)
            );
        }

        template<class Tree, class... Args>
        static typename Tree::iterator emplace(Tree& tree, Args&&... args)
        {
            using node_type  = typename Tree::node_type;

            auto node = new node_type{forward<Args>(args)...};

            return insert(tree, node);
        }

        template<class Tree, class Value>
        static typename Tree::iterator insert(Tree& tree, const Value& val)
        {
            using node_type = typename Tree::node_type;

            auto node = new node_type{val};

            return insert(tree, node);
        }

        template<class Tree, class Value>
        static typename Tree::iterator insert(Tree& tree, Value&& val)
        {
            using node_type = typename Tree::node_type;

            auto node = new node_type{forward<Value>(val)};

            return insert(tree, node);
        }

        template<class Tree>
        static typename Tree::iterator insert(
            Tree& tree, typename Tree::node_type* node,
            typename Tree::node_type* = nullptr
        )
        {
            using iterator  = typename Tree::iterator;

            if (!node)
                return tree.end();

            auto parent = tree.find_parent_for_insertion(tree.get_key(node->value));

            ++tree.size_;
            if (!parent)
            {
                node->color = rbcolor::black;
                tree.root_ = node;
            }
            else
            {
                if (tree.keys_comp(tree.get_key(node->value), parent->value))
                    parent->add_left_child(node);
                else if (tree.keys_comp(tree.get_key(parent->value), node->value))
                    parent->add_right_child(node);
                else
                {
                    parent->add(node); // List of nodes with equivalent keys.
                    tree.update_root_(parent);

                    return iterator{node, false};
                }

                tree.repair_after_insert_(node);
                tree.update_root_(node);
            }

            return iterator{node, false};
        }
    };
}

#endif

