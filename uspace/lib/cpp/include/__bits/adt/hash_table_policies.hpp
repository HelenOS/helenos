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

#ifndef LIBCPP_BITS_ADT_HASH_TABLE_POLICIES
#define LIBCPP_BITS_ADT_HASH_TABLE_POLICIES

#include <tuple>
#include <utility>

namespace std::aux
{
    struct hash_single_policy
    {
        template<class Table, class Key>
        static typename Table::size_type count(const Table& table, const Key& key)
        {
            return table.find(key) == table.end() ? 0 : 1;
        }

        template<class Table, class Key>
        static typename Table::place_type find_insertion_spot(const Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            auto head = table.table_[idx].head;

            if (head)
            { // Check if the key is present.
                auto current = head;
                do
                {
                    if (table.keys_equal(key, current->value))
                    {
                        return make_tuple(
                            &table.table_[idx],
                            current,
                            idx
                        );
                    }

                    current = current->next;
                } while (current && current != head);
            }

            return make_tuple(
                &table.table_[idx],
                table.table_[idx].head,
                idx
            );
        }

        template<class Table, class Key>
        static typename Table::size_type erase(Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            auto head = table.table_[idx].head;
            auto current = head;

            if (!current)
                return 0;

            do
            {
                if (table.keys_equal(key, current->value))
                {
                    --table.size_;

                    if (current == head)
                    {
                        if (current->next != head)
                            table.table_[idx].head = current->next;
                        else
                            table.table_[idx].head = nullptr;
                    }

                    current->unlink();
                    delete current;

                    return 1;
                }
                else
                    current = current->next;
            }
            while (current && current != head);

            return 0;
        }

        template<class Table, class Key>
        static pair<
            typename Table::iterator,
            typename Table::iterator
        > equal_range(Table& table, const Key& key)
        {
            auto it = table.find(key);
            return make_pair(it, ++it);
        }

        template<class Table, class Key>
        static pair<
            typename Table::const_iterator,
            typename Table::const_iterator
        > equal_range_const(const Table& table, const Key& key)
        { // Note: We cannot overload by return type, so we use a different name.
            auto it = table.find(key);
            return make_pair(it, ++it);
        }

        /**
         * Note: We have to duplicate code for emplace, insert(const&)
         *       and insert(&&) here, because the node (which makes distinction
         *       between the arguments) is only created if the value isn't
         *       in the table already.
         */

        template<class Table, class... Args>
        static pair<
            typename Table::iterator, bool
        > emplace(Table& table, Args&&... args)
        {
            using value_type = typename Table::value_type;
            using node_type  = typename Table::node_type;
            using iterator   = typename Table::iterator;

            table.increment_size();

            auto val = value_type{forward<Args>(args)...};
            const auto& key = table.get_key(val);
            auto [bucket, target, idx] = table.find_insertion_spot(key);

            if (!bucket)
                return make_pair(table.end(), false);

            if (target && table.keys_equal(key, target->value))
            {
                table.decrement_size();

                return make_pair(
                    iterator{
                        table.table(), idx, table.bucket_count(),
                        target
                    },
                    false
                );
            }
            else
            {
                auto node = new node_type{move(val)};
                bucket->prepend(node);

                return make_pair(iterator{
                    table.table(), idx,
                    table.bucket_count(),
                    node
                }, true);
            }
        }

        template<class Table, class Value>
        static pair<
            typename Table::iterator, bool
        > insert(Table& table, const Value& val)
        {
            using node_type  = typename Table::node_type;
            using iterator   = typename Table::iterator;

            table.increment_size();

            const auto& key = table.get_key(val);
            auto [bucket, target, idx] = table.find_insertion_spot(key);

            if (!bucket)
                return make_pair(table.end(), false);

            if (target && table.keys_equal(key, target->value))
            {
                table.decrement_size();

                return make_pair(
                    iterator{
                        table.table(), idx, table.bucket_count(),
                        target
                    },
                    false
                );
            }
            else
            {
                auto node = new node_type{val};
                bucket->prepend(node);

                return make_pair(iterator{
                    table.table(), idx,
                    table.bucket_count(),
                    node
                }, true);
            }
        }

        template<class Table, class Value>
        static pair<
            typename Table::iterator, bool
        > insert(Table& table, Value&& val)
        {
            using value_type = typename Table::value_type;
            using node_type  = typename Table::node_type;
            using iterator   = typename Table::iterator;

            table.increment_size();

            const auto& key = table.get_key(val);
            auto [bucket, target, idx] = table.find_insertion_spot(key);

            if (!bucket)
                return make_pair(table.end(), false);

            if (target && table.keys_equal(key, target->value))
            {
                table.decrement_size();

                return make_pair(
                    iterator{
                        table.table(), idx, table.bucket_count(),
                        target
                    },
                    false
                );
            }
            else
            {
                auto node = new node_type{forward<value_type>(val)};
                bucket->prepend(node);

                return make_pair(iterator{
                    table.table(), idx,
                    table.bucket_count(),
                    node
                }, true);
            }
        }
    };

    struct hash_multi_policy
    {
        template<class Table, class Key>
        static typename Table::size_type count(const Table& table, const Key& key)
        {
            auto head = table.table_[table.get_bucket_idx_(key)].head;
            if (!head)
                return 0;

            auto current = head;
            typename Table::size_type res = 0;
            do
            {
                if (table.keys_equal(key, current->value))
                    ++res;

                current = current->next;
            }
            while (current && current != head);

            return res;
        }

        template<class Table, class Key>
        static typename Table::place_type find_insertion_spot(const Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            auto head = table.table_[idx].head;

            if (head)
            {
                auto current = head;
                do
                {
                    if (table.keys_equal(key, current->value))
                    {
                        return make_tuple(
                            &table.table_[idx],
                            current,
                            idx
                        );
                    }

                    current = current->next;
                } while (current && current != head);
            }

            return make_tuple(
                &table.table_[idx],
                table.table_[idx].head,
                idx
            );
        }

        template<class Table, class Key>
        static typename Table::size_type erase(Table& table, const Key& key)
        {
            auto idx = table.get_bucket_idx_(key);
            auto head = table.table_[idx].head;
            auto current = head;
            table.table_[idx].head = nullptr;
            decltype(head) last = nullptr;

            if (!current)
                return 0;

            /**
             * We traverse the list and delete if the keys
             * equal, if they don't we append the nodes back
             * to the bucket.
             */
            typename Table::size_type res{};

            do
            {
                auto tmp = current;
                current = current->next;
                tmp->next = tmp;
                tmp->prev = tmp;

                if (!table.keys_equal(key, tmp->value))
                {
                    if (!last)
                        table.table_[idx].head = tmp;
                    else
                        last->append(tmp);
                    last = tmp;
                }
                else
                {
                    --table.size_;
                    ++res;

                    delete tmp;
                }
            }
            while (current && current != head);

            return res;
        }

        template<class Table, class Key>
        static pair<
            typename Table::iterator,
            typename Table::iterator
        > equal_range(Table& table, const Key& key)
        {
            auto first = table.find(key);
            if (first == table.end())
                return make_pair(table.end(), table.end());

            auto last = first;
            do
            {
                ++last;
            } while (last != table.end() && table.keys_equal(key, *last));

            return make_pair(first, last);
        }

        template<class Table, class Key>
        static pair<
            typename Table::const_iterator,
            typename Table::const_iterator
        > equal_range_const(const Table& table, const Key& key)
        {
            auto first = table.find(key);
            if (first == table.end())
                return make_pair(table.end(), table.end());

            auto last = first;
            do
            {
                ++last;
            } while (last != table.end() && table.keys_equal(key, *last));

            return make_pair(first, last);
        }

        template<class Table, class... Args>
        static typename Table::iterator emplace(Table& table, Args&&... args)
        {
            using node_type  = typename Table::node_type;

            auto node = new node_type{forward<Args>(args)...};

            return insert(table, node);
        }

        template<class Table, class Value>
        static typename Table::iterator insert(Table& table, const Value& val)
        {
            using node_type  = typename Table::node_type;

            auto node = new node_type{val};

            return insert(table, node);
        }

        template<class Table, class Value>
        static typename Table::iterator insert(Table& table, Value&& val)
        {
            using value_type = typename Table::value_type;
            using node_type  = typename Table::node_type;

            auto node = new node_type{forward<value_type>(val)};

            return insert(table, node);
        }

        template<class Table>
        static typename Table::iterator insert(Table& table, typename Table::node_type* node)
        {
            using iterator   = typename Table::iterator;

            table.increment_size();

            const auto& key = table.get_key(node->value);
            auto [bucket, target, idx] = table.find_insertion_spot(key);

            if (!bucket)
                return table.end();

            if (target && table.keys_equal(key, target->value))
                target->append(node);
            else
                bucket->prepend(node);

            return iterator{
                table.table(), idx,
                table.bucket_count(),
                node
            };
        }
    };
}

#endif
