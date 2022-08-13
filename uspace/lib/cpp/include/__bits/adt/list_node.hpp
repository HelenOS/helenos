/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ADT_LIST_NODE
#define LIBCPP_BITS_ADT_LIST_NODE

namespace std::aux
{
    template<class T>
    struct list_node
    {
        T value;
        list_node* next;
        list_node* prev;

        template<class... Args>
        list_node(Args&&... args)
            : value{forward<Args>(args)...},
              next{}, prev{}
        {
            next = this;
            prev = this;
        }

        list_node(const T& val)
            : value{val}, next{}, prev{}
        {
            next = this;
            prev = this;
        }

        list_node(T&& val)
            : value{forward<T>(val)}, next{}, prev{}
        {
            next = this;
            prev = this;
        }

        void append(list_node* node)
        {
            if (!node)
                return;

            node->next = next;
            node->prev = this;
            next->prev = node;
            next = node;
        }

        void prepend(list_node* node)
        {
            if (!node)
                return;

            node->next = this;
            node->prev = prev;
            prev->next = node;
            prev = node;
        }

        void unlink()
        {
            prev->next = next;
            next->prev = prev;
            next = this;
            prev = this;
        }
    };
}

#endif
