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

#ifndef LIBCPP_INTERNAL_RBTREE_NODE
#define LIBCPP_INTERNAL_RBTREE_NODE

#include <utility>

namespace std::aux
{
    enum class rbcolor
    {
        red, black
    };

    template<class T>
    struct rbtree_node
    {
        T value;
        rbcolor color;

        rbtree_node* parent;
        rbtree_node* left;
        rbtree_node* right;

        template<class... Args>
        rbtree_node(Args&&... args)
            : value{forward<Args>(args)...}, color{rbcolor::red},
              parent{}, left{}, right{}
        { /* DUMMY BODY */ }

        rbtree_node* grandparent() const
        {
            if (parent)
                return parent->parent;
            else
                return nullptr;
        }

        rbtree_node* brother() const
        {
            if (parent)
            {
                if (this == parent->left)
                    return parent->right;
                else
                    return parent->left;
            }
            else
                return nullptr;
        }

        rbtree_node* uncle() const
        {
            if (grandparent())
            {
                if (parent == grandparent()->left)
                    return grandparent()->right;
                else
                    return grandparent()->left;
            }
            else
                return nullptr;
        }

        bool is_left_child() const
        {
            if (parent)
                return parent->left == this;
            else
                return false;
        }

        bool is_right_child() const
        {
            if (parent)
                return parent->right == this;
            else
                return false;
        }

        void rotate_left()
        {
            // TODO:
        }

        void rotate_right()
        {
            // TODO:
        }

        rbtree_node* find_smallest()
        {
            auto res = this;
            while (res->left)
                res = res->left;

            return res;
        }

        rbtree_node* find_largest()
        {
            auto res = this;
            while (res->right)
                res = res->right;

            return res;
        }

        rbtree_node* successor()
        {
            if (right)
                return right->find_smallest();
            else
            {
                auto current = this;
                while (!current->is_left_child())
                    current = current->parent;

                return current->parent;
            }
        }

        void add_left_child(rbtree_node* node)
        {
            if (left)
                return;

            left = node;
            node->parent = this;
        }

        void add_right_child(rbtree_node* node)
        {
            if (right)
                return;

            right = node;
            node->parent = this;
        }

        void swap(rbtree_node* other)
        {
            /**
             * Parent can be null so we check both ways.
             */
            if (is_left_child())
                parent->left = other;
            else if (is_right_child())
                parent->right = other;

            if (other->is_left_child())
                other->parent->left = this;
            else if (other->is_right_child())
                other->parent->right = this;

            if (left)
                left->parent = other;
            if (right)
                right->parent = other;
            if (other->left)
                other->left->parent = this;
            if (other->right)
                other->right->parent = this;

            std::swap(parent, other->parent);
            std::swap(left, other->left);
            std::swap(right, other->right);
        }

        void unlink()
        {
            if (is_left_child())
                parent->left = nullptr;
            else if (is_right_child())
                parent->right = nullptr;
        }

        ~rbtree_node()
        {
            // TODO: delete recursively or iteratively?
        }
    };
}

#endif
