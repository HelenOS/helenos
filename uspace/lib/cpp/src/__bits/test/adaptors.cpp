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

#include <__bits/test/tests.hpp>
#include <cstdlib>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <queue>
#include <stack>
#include <vector>

namespace std::test
{
    namespace aux
    {
        template<class T, class Comp = std::less<T>>
        class priority_queue_iterator
        {
            public:
                using value_type      = T;
                using reference       = const T&;
                using size_type       = size_t;
                using pointer         = value_type*;
                using difference_type = size_t;

                using iterator_category = forward_iterator_tag;

                priority_queue_iterator(
                    priority_queue<value_type, std::vector<value_type>, Comp> q,
                    bool end = false
                )
                    : queue_{q}, end_{end}
                { /* DUMMY BODY */ }

                priority_queue_iterator(const priority_queue_iterator& other)
                    : queue_{other.queue_}, end_{other.end_}
                { /* DUMMY BODY */ }

                reference operator*()
                {
                    return queue_.top();
                }

                priority_queue_iterator& operator++()
                {
                    queue_.pop();

                    if (queue_.empty())
                        end_ = true;

                    return *this;
                }

                priority_queue_iterator operator++(int)
                {
                    auto old = *this;
                    ++(*this);

                    return old;
                }

                bool operator==(const priority_queue_iterator& rhs) const
                {
                    return end_ == rhs.end_;
                }

                bool operator!=(const priority_queue_iterator& rhs) const
                {
                    return !(*this == rhs);
                }

            private:
                priority_queue<value_type, std::vector<value_type>, Comp> queue_;
                bool end_;
        };
    }

    bool adaptors_test::run(bool report)
    {
        report_ = report;
        start();

        test_queue();
        test_priority_queue();
        test_stack();

        return end();
    }

    const char* adaptors_test::name()
    {
        return "adaptors";
    }

    void adaptors_test::test_queue()
    {
        std::queue<int> q{std::deque<int>{1}};

        test_eq("queue initialized from deque not empty", q.empty(), false);
        test_eq("queue initialized form queue size", q.size(), 1U);
        test_eq("single element queue front == back", q.front(), q.back());

        q.push(2);
        test_eq("queue push", q.back(), 2);
        test_eq("queue size", q.size(), 2U);

        q.pop();
        test_eq("queue pop", q.front(), 2);

        q.emplace(4);
        test_eq("queue emplace", q.back(), 4);
    }

    void adaptors_test::test_priority_queue()
    {
        auto check1 = {9, 8, 5, 4, 2, 1};
        std::vector<int> data{5, 4, 2, 8, 1};
        std::priority_queue<int> q1{data.begin(), data.end()};

        test_eq("priority_queue initialized from iterator range not empty", q1.empty(), false);
        test_eq("priority_queue initialized from iterator range size", q1.size(), 5U);

        q1.push(9);
        test_eq("priority_queue push pt1", q1.size(), 6U);
        test_eq("priority_queue push pt2", q1.top(), 9);

        test_eq(
            "priority_queue initialized from iterator range ops",
            check1.begin(), check1.end(),
            aux::priority_queue_iterator<int>{q1},
            aux::priority_queue_iterator<int>{q1, true}
        );

        auto check2 = {1, 2, 3, 4, 5, 8};
        std::priority_queue<int, std::vector<int>, std::greater<int>> q2{std::greater<int>{}, data};

        test_eq("priority_queue initialized from vector and compare not empty", q2.empty(), false);
        test_eq("priority_queue initialized from vector and compare size", q2.size(), 5U);

        q2.push(3);
        test_eq("priority_queue push pt1", q2.size(), 6U);
        test_eq("priority_queue push pt2", q2.top(), 1);

        test_eq(
            "priority_queue initialized from vector and compare ops",
            check2.begin(), check2.end(),
            aux::priority_queue_iterator<int, std::greater<int>>{q2},
            aux::priority_queue_iterator<int, std::greater<int>>{q2, true}
        );
    }

    void adaptors_test::test_stack()
    {
        std::stack<int> s{std::deque<int>{1}};

        test_eq("stack initialized from deque top", s.top(), 1);
        test_eq("stack initialized from deque size", s.size(), 1U);
        test_eq("stack initialized from deque not empty", s.empty(), false);

        s.push(2);
        test_eq("stack push top", s.top(), 2);
        test_eq("stack push size", s.size(), 2U);

        s.pop();
        test_eq("stack pop top", s.top(), 1);
        test_eq("stack pop size", s.size(), 1U);
    }
}
