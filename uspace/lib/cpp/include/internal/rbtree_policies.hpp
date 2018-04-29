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

#ifndef LIBCPP_INTERNAL_RBTREE_POLICIES
#define LIBCPP_INTERNAL_RBTREE_POLICIES

#include <internal/rbtree_node.hpp>
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
        static pair<
            typename Tree::node_type*,
            typename Tree::node_type*
        > erase(const Tree& tree, const Key& key)
        {
            // TODO:
        }

        template<class Tree, class Key>
        static typename Tree::iterator lower_bound(const Tree& tree, const Key& key)
        {
            // TODO:
        }

        template<class Tree, class Key>
        static typename Tree::iterator upper_bound(const Tree& tree, const Key& key)
        {
            // TODO:
        }

        template<class Tree, class Key>
        static pair<
            typename Tree::iterator,
            typename Tree::iterator
        > equal_range(const Tree& tree, const Key& key)
        {
            // TODO:
        }

        template<class Tree, class Key>
        static pair<
            typename Tree::const_iterator,
            typename Tree::const_iterator
        > equal_range_const(const Tree& tree, const Key& key)
        {
            // TODO:
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
            auto parent = tree.find_parent_for_insertion(val);
            if (!parent)
            {
                tree.root_ = new node_type{move(val)};

                return make_pair(iterator{tree.root_}, true);
            }

            if (tree.get_key(parent->value) == tree.get_key(val))
                return make_pair(iterator{parent}, false);

            auto node = new node_type{move(val)};
            if (tree.keys_comp(tree.get_key(val), parent->value))
                parent->add_left_child(node);
            else
                parent->add_right_child(node);

            return make_pair(iterator{node}, true);
        }

        template<class Tree, class Value>
        static pair<
            typename Tree::iterator, bool
        > insert(Tree& tree, const Value& val)
        {
            using iterator  = typename Tree::iterator;
            using node_type = typename Tree::node_type;

            auto parent = tree.find_parent_for_insertion(val);
            if (!parent)
            {
                tree.root_ = new node_type{val};

                return make_pair(iterator{tree.root_}, true);
            }

            if (tree.get_key(parent->value) == tree.get_key(val))
                return make_pair(iterator{parent}, false);

            auto node = new node_type{val};
            if (tree.keys_comp(tree.get_key(val), parent->value))
                parent->add_left_child(node);
            else
                parent->add_right_child(node);

            return make_pair(iterator{node}, true);
        }

        template<class Tree, class Value>
        static pair<
            typename Tree::iterator, bool
        > insert(Tree& tree, Value&& val)
        {
            using iterator  = typename Tree::iterator;
            using node_type = typename Tree::node_type;

            auto parent = tree.find_parent_for_insertion(val);
            if (!parent)
            {
                tree.root_ = new node_type{forward<Value>(val)};

                return make_pair(iterator{tree.root_}, true);
            }

            if (tree.get_key(parent->value) == tree.get_key(val))
                return make_pair(iterator{parent}, false);

            auto node = new node_type{forward<Value>(val)};
            if (tree.keys_comp(tree.get_key(val), parent->value))
                parent->add_left_child(node);
            else
                parent->add_right_child(node);

            return make_pair(iterator{node}, true);
        }
    };

    struct rbtree_multi_policy
    {
        // TODO:
    };
}

#endif

