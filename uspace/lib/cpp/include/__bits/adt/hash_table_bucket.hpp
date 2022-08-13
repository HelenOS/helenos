/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ADT_HASH_TABLE_BUCKET
#define LIBCPP_BITS_ADT_HASH_TABLE_BUCKET

#include <__bits/adt/list_node.hpp>

namespace std::aux
{
    template<class Value, class Size>
    struct hash_table_bucket
    {
        /**
         * Note: We use a doubly linked list because
         *       we need to use hints, which point to the
         *       element after the hinted spot.
         */

        list_node<Value>* head;

        hash_table_bucket()
            : head{}
        { /* DUMMY BODY */ }

        Size size() const noexcept
        {
            auto current = head;
            Size res{};

            do
            {
                ++res;
                current = current->next;
            }
            while (current != head);

            return res;
        }

        void append(list_node<Value>* node)
        {
            if (!head)
                head = node;
            else
                head->append(node);
        }

        void prepend(list_node<Value>* node)
        {
            if (!head)
                head = node;
            else
                head->prepend(node);
        }

        void clear()
        {
            if (!head)
                return;

            auto current = head;
            do
            {
                auto tmp = current;
                current = current->next;
                delete tmp;
            }
            while (current && current != head);

            head = nullptr;
        }

        ~hash_table_bucket()
        {
            clear();
        }
    };
}

#endif
