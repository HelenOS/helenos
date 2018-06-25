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

#ifndef LIBCPP_BITS_ADT_RBTREE_NODE
#define LIBCPP_BITS_ADT_RBTREE_NODE

#include <cassert>
#include <utility>

namespace std::aux
{
    enum class rbcolor
    {
        red, black
    };

    template<class Node>
    struct rbtree_utils
    {
        static Node* grandparent(Node* node)
        {
            if (node && node->parent())
                return node->parent()->parent();
            else
                return nullptr;
        }

        static Node* brother(Node* node)
        {
            if (node && node->parent())
            {
                if (node == node->parent()->left())
                    return node->parent()->right();
                else
                    return node->parent()->left();
            }
            else
                return nullptr;
        }

        static Node* uncle(Node* node)
        {
            auto gp = grandparent(node);
            if (gp)
            {
                if (node->parent() == gp->left())
                    return gp->right();
                else
                    return gp->left();
            }
            else
                return nullptr;
        }

        static bool is_left_child(const Node* node)
        {
            if (!node)
                return false;

            if (node->parent())
                return node->parent()->left() == node;
            else
                return false;
        }

        static bool is_right_child(const Node* node)
        {
            if (!node)
                return false;

            if (node->parent())
                return node->parent()->right() == node;
            else
                return false;
        }

        static void rotate_left(Node* node)
        {
            // TODO: implement
        }

        static void rotate_right(Node* node)
        {
            // TODO: implement
        }

        static Node* find_smallest(Node* node)
        {
            return const_cast<Node*>(find_smallest(const_cast<const Node*>(node)));
        }

        static const Node* find_smallest(const Node* node)
        {
            if (!node)
                return nullptr;

            while (node->left())
                node = node->left();

            return node;
        }

        static Node* find_largest(Node* node)
        {
            return const_cast<Node*>(find_largest(const_cast<const Node*>(node)));
        }

        static const Node* find_largest(const Node* node)
        {
            if (!node)
                return nullptr;

            while (node->right())
                node = node->right();

            return node;
        }

        static Node* successor(Node* node)
        {
            return const_cast<Node*>(successor(const_cast<const Node*>(node)));
        }

        static const Node* successor(const Node* node)
        {
            if (!node)
                return nullptr;

            if (node->right())
                return find_smallest(node->right());
            else
            {
                while (node && !is_left_child(node))
                    node = node->parent();

                if (node)
                    return node->parent();
                else
                    return node;
            }
        }

        static Node* predecessor(Node* node)
        {
            return const_cast<Node*>(predecessor(const_cast<const Node*>(node)));
        }

        static const Node* predecessor(const Node* node)
        {
            if (!node)
                return nullptr;

            if (node->left())
                return find_largest(node->left());
            else
            {
                while (node && is_left_child(node))
                    node = node->parent();

                if (node)
                    return node->parent();
                else
                    return node;
            }
        }

        static void add_left_child(Node* node, Node* child)
        {
            if (!node || !child)
                return;

            node->left(child);
            child->parent(node);
        }

        static void add_right_child(Node* node, Node* child)
        {
            if (!node || !child)
                return;

            node->right(child);
            child->parent(node);
        }

        static void swap(Node* node1, Node* node2)
        {
            if (!node1 || !node2)
                return;

            auto parent1 = node1->parent();
            auto left1 = node1->left();
            auto right1 = node1->right();
            auto is_right1 = is_right_child(node1);

            auto parent2 = node2->parent();
            auto left2 = node2->left();
            auto right2 = node2->right();
            auto is_right2 = is_right_child(node2);

            assimilate(node1, parent2, left2, right2, is_right2);
            assimilate(node2, parent1, left1, right1, is_right1);
        }

        static void assimilate(
            Node* node, Node* p, Node* l, Node* r, bool is_r
        )
        {
            if (!node)
                return;

            node->parent(p);
            if (node->parent())
            {
                if (is_r)
                    node->parent()->right(node);
                else
                    node->parent()->left(node);
            }

            node->left(l);
            if (node->left())
                node->left()->parent(node);

            node->right(r);
            if (node->right())
                node->right()->parent(node);
        }
    };

    template<class T>
    struct rbtree_single_node
    {
        using utils = rbtree_utils<rbtree_single_node<T>>;

        public:
            T value;
            rbcolor color;

            template<class... Args>
            rbtree_single_node(Args&&... args)
                : value{forward<Args>(args)...}, color{rbcolor::red},
                  parent_{}, left_{}, right_{}
            { /* DUMMY BODY */ }

            rbtree_single_node* parent() const
            {
                return parent_;
            }

            void parent(rbtree_single_node* node)
            {
                parent_ = node;
            }

            rbtree_single_node* left() const
            {
                return left_;
            }

            void left(rbtree_single_node* node)
            {
                left_ = node;
            }

            rbtree_single_node* right() const
            {
                return right_;
            }

            void right(rbtree_single_node* node)
            {
                right_ = node;
            }

            rbtree_single_node* grandparent()
            {
                return utils::grandparent(this);
            }

            rbtree_single_node* brother()
            {
                return utils::brother(this);
            }

            rbtree_single_node* uncle()
            {
                return utils::uncle(this);
            }

            bool is_left_child() const
            {
                return utils::is_left_child(this);
            }

            bool is_right_child() const
            {
                return utils::is_right_child(this);
            }

            void rotate_left()
            {
                utils::rotate_left(this);
            }

            void rotate_right()
            {
                utils::rotate_right(this);
            }

            rbtree_single_node* find_smallest()
            {
                return utils::find_smallest(this);
            }

            const rbtree_single_node* find_smallest() const
            {
                return utils::find_smallest(this);
            }

            rbtree_single_node* find_largest()
            {
                return utils::find_largest(this);
            }

            const rbtree_single_node* find_largest() const
            {
                return utils::find_largest(this);
            }

            rbtree_single_node* successor()
            {
                return utils::successor(this);
            }

            const rbtree_single_node* successor() const
            {
                return utils::successor(this);
            }

            rbtree_single_node* predecessor()
            {
                return utils::predecessor(this);
            }

            const rbtree_single_node* predecessor() const
            {
                return utils::predecessor(this);
            }

            void add_left_child(rbtree_single_node* node)
            {
                utils::add_left_child(this, node);
            }

            void add_right_child(rbtree_single_node* node)
            {
                utils::add_right_child(this, node);
            }

            void swap(rbtree_single_node* other)
            {
                utils::swap(this, other);
            }

            void unlink()
            {
                if (is_left_child())
                    parent_->left_ = nullptr;
                else if (is_right_child())
                    parent_->right_ = nullptr;
            }

            rbtree_single_node* get_node_for_deletion()
            {
                return nullptr;
            }

            rbtree_single_node* get_end()
            {
                return this;
            }

            const rbtree_single_node* get_end() const
            {
                return this;
            }

            ~rbtree_single_node()
            {
                parent_ = nullptr;
                if (left_)
                    delete left_;
                if (right_)
                    delete right_;
            }

        private:
            rbtree_single_node* parent_;
            rbtree_single_node* left_;
            rbtree_single_node* right_;
    };

    template<class T>
    struct rbtree_multi_node
    {
        using utils = rbtree_utils<rbtree_multi_node<T>>;

        public:
            T value;
            rbcolor color;

            template<class... Args>
            rbtree_multi_node(Args&&... args)
                : value{forward<Args>(args)...}, color{rbcolor::red},
                  parent_{}, left_{}, right_{}, next_{}, first_{}
            {
                first_ = this;
            }

            rbtree_multi_node* parent() const
            {
                return parent_;
            }

            void parent(rbtree_multi_node* node)
            {
                parent_ = node;

                auto tmp = first_;
                while (tmp)
                {
                    tmp->parent_ = node;
                    tmp = tmp->next_;
                }
            }

            rbtree_multi_node* left() const
            {
                return left_;
            }

            void left(rbtree_multi_node* node)
            {
                left_ = node;

                auto tmp = first_;
                while (tmp)
                {
                    tmp->left_ = node;
                    tmp = tmp->next_;
                }
            }

            rbtree_multi_node* right() const
            {
                return right_;
            }

            void right(rbtree_multi_node* node)
            {
                right_ = node;

                auto tmp = first_;
                while (tmp)
                {
                    tmp->right_ = node;
                    tmp = tmp->next_;
                }
            }

            rbtree_multi_node* grandparent()
            {
                return utils::grandparent(this);
            }

            rbtree_multi_node* brother()
            {
                return utils::brother(this);
            }

            rbtree_multi_node* uncle()
            {
                return utils::uncle(this);
            }

            bool is_left_child() const
            {
                return utils::is_left_child(this);
            }

            bool is_right_child()
            {
                return utils::is_right_child(this);
            }

            void rotate_left()
            {
                utils::rotate_left(this);
            }

            void rotate_right()
            {
                utils::rotate_right(this);
            }

            rbtree_multi_node* find_smallest()
            {
                return utils::find_smallest(this);
            }

            const rbtree_multi_node* find_smallest() const
            {
                return utils::find_smallest(this);
            }

            rbtree_multi_node* find_largest()
            {
                return utils::find_largest(this);
            }

            const rbtree_multi_node* find_largest() const
            {
                return utils::find_largest(this);
            }

            rbtree_multi_node* successor()
            {
                return const_cast<
                    rbtree_multi_node*
                >(const_cast<const rbtree_multi_node*>(this)->successor());
            }

            const rbtree_multi_node* successor() const
            {
                if (next_)
                    return next_;
                else
                    return utils::successor(this);
            }

            rbtree_multi_node* predecessor()
            {
                return const_cast<
                    rbtree_multi_node*
                >(const_cast<const rbtree_multi_node*>(this)->predecessor());
            }

            const rbtree_multi_node* predecessor() const
            {
                if (this != first_)
                {
                    auto tmp = first_;
                    while (tmp->next_ != this)
                        tmp = tmp->next_;

                    return tmp;
                }
                else
                {
                    auto tmp = utils::predecessor(this);

                    /**
                     * If tmp was duplicate, we got a pointer
                     * to the first node in the list. So we need
                     * to move to the end.
                     */
                    while (tmp->next_ != nullptr)
                        tmp = tmp->next_;

                    return tmp;
                }
            }

            void add_left_child(rbtree_multi_node* node)
            {
                utils::add_left_child(this, node);
            }

            void add_right_child(rbtree_multi_node* node)
            {
                utils::add_right_child(this, node);
            }

            void swap(rbtree_multi_node* other)
            {
                utils::swap(this, other);
            }

            rbtree_multi_node* get_node_for_deletion()
            {
                /**
                 * To make sure we delete nodes in
                 * the order of their insertion
                 * (not required, but sensical), we
                 * update then list and return this
                 * for deletion.
                 */
                if (next_)
                {
                    // Make next the new this.
                    next_->first_ = next_;
                    if (is_left_child())
                        parent_->left_ = next_;
                    else if (is_right_child())
                        parent_->right_ = next_;

                    if (left_)
                        left_->parent_ = next_;
                    if (right_)
                        right_->parent_ = next_;

                    /**
                     * Update the first_ pointer
                     * of the rest of the list.
                     */
                    auto tmp = next_->next_;
                    while (tmp)
                    {
                        tmp->first_ = next_;
                        tmp = tmp->next_;
                    }

                    /**
                     * Otherwise destructor could
                     * destroy them.
                     */
                    parent_ = nullptr;
                    left_ = nullptr;
                    right_ = nullptr;
                    next_ = nullptr;
                    first_ = nullptr;

                    return this; // This will get deleted.
                }
                else
                    return nullptr;
            }

            void unlink()
            {
                if (is_left_child())
                    parent_->left_ = nullptr;
                else if (is_right_child())
                    parent_->right_ = nullptr;
            }

            void add(rbtree_multi_node* node)
            {
                if (next_)
                    next_->add(node);
                else
                {
                    next_ = node;
                    next_->first_ = first_;
                    next_->parent_ = parent_;
                    next_->left_ = left_;
                    next_->right_ = right_;
                }
            }

            rbtree_multi_node* get_end()
            {
                return const_cast<rbtree_multi_node*>(
                    const_cast<const rbtree_multi_node*>(this)->get_end()
                );
            }

            const rbtree_multi_node* get_end() const
            {
                if (!next_)
                    return this;
                else
                {
                    auto tmp = next_;
                    while (tmp->next_)
                        tmp = tmp->next_;

                    return tmp;
                }
            }

            ~rbtree_multi_node()
            {
                parent_ = nullptr;
                if (left_)
                    delete left_;
                if (right_)
                    delete right_;

                // TODO: delete the list
            }

        private:
            rbtree_multi_node* parent_;
            rbtree_multi_node* left_;
            rbtree_multi_node* right_;

            rbtree_multi_node* next_;
            rbtree_multi_node* first_;
    };
}

#endif
