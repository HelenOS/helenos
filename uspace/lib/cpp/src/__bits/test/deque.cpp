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
#include <deque>
#include <initializer_list>
#include <utility>

namespace std::test
{
    bool deque_test::run(bool report)
    {
        report_ = report;
        start();

        test_constructors_and_assignment();
        test_resizing();
        test_push_pop();
        test_operations();

        return end();
    }

    const char* deque_test::name()
    {
        return "deque";
    }

    void deque_test::test_constructors_and_assignment()
    {
        auto check1 = {0, 0, 0, 0, 0};
        std::deque<int> d1(5U);
        test_eq(
            "size construction",
            check1.begin(), check1.end(),
            d1.begin(), d1.end()
        );

        auto check2 = {1, 1, 1, 1, 1};
        std::deque<int> d2(5U, 1);
        test_eq(
            "value construction",
            check2.begin(), check2.end(),
            d2.begin(), d2.end()
        );

        auto check3 = {1, 2, 3, 4, 5};
        std::deque<int> d3{check3.begin(), check3.end()};
        test_eq(
            "iterator range construction",
            check3.begin(), check3.end(),
            d3.begin(), d3.end()
        );

        std::deque<int> d4{d3};
        test_eq(
            "copy construction",
            check3.begin(), check3.end(),
            d4.begin(), d4.end()
        );

        std::deque<int> d5{std::move(d4)};
        test_eq(
            "move construction",
            check3.begin(), check3.end(),
            d5.begin(), d5.end()
        );
        test_eq("move construction - origin empty", d4.empty(), true);

        std::deque<int> d6{check3};
        test_eq(
            "inializer list construction",
            check3.begin(), check3.end(),
            d6.begin(), d6.end()
        );

        d4 = d6;
        test_eq(
            "copy assignment",
            check3.begin(), check3.end(),
            d4.begin(), d4.end()
        );

        d6 = std::move(d4);
        test_eq(
            "move assignment",
            check3.begin(), check3.end(),
            d6.begin(), d6.end()
        );
        test_eq("move assignment - origin empty", d4.empty(), true);

        d4 = check3;
        test_eq(
            "initializer list assignment",
            check3.begin(), check3.end(),
            d4.begin(), d4.end()
        );

        std::deque<int> d7{};
        d7.assign(check3.begin(), check3.end());
        test_eq(
            "iterator range assign()",
            check3.begin(), check3.end(),
            d7.begin(), d7.end()
        );

        d7.assign(5U, 1);
        test_eq(
            "value assign()",
            check2.begin(), check2.end(),
            d7.begin(), d7.end()
        );

        d7.assign(check3);
        test_eq(
            "initializer list assign()",
            check3.begin(), check3.end(),
            d7.begin(), d7.end()
        );
    }

    void deque_test::test_resizing()
    {
        auto check1 = {1, 2, 3};
        auto check2 = {1, 2, 3, 0, 0};
        std::deque<int> d1{1, 2, 3, 4, 5};

        d1.resize(3U);
        test_eq(
            "downsize",
            check1.begin(), check1.end(),
            d1.begin(), d1.end()
        );

        d1.resize(5U);
        test_eq(
            "upsize",
            check2.begin(), check2.end(),
            d1.begin(), d1.end()
        );

        auto check3 = {1, 2, 3, 9, 9};
        std::deque<int> d2{1, 2, 3, 4, 5};

        d2.resize(3U, 9);
        test_eq(
            "downsize with default value",
            check1.begin(), check1.end(),
            d2.begin(), d2.end()
        );

        d2.resize(5U, 9);
        test_eq(
            "upsize with default value",
            check3.begin(), check3.end(),
            d2.begin(), d2.end()
        );

        d2.resize(0U);
        test_eq("resize to 0", d2.empty(), true);
    }

    void deque_test::test_push_pop()
    {
        std::deque<int> d1{};

        d1.push_back(42);
        test_eq("push_back to empty equivalence", d1[0], 42);
        test_eq("push_back to empty size", d1.size(), 1U);

        d1.push_front(21);
        test_eq("push_front after push_back equivalence", d1[0], 21);
        test_eq("push_front after push_back size", d1.size(), 2U);

        for (int i = 0; i <= 100; ++i)
            d1.push_back(i);
        test_eq("back after bucket test", d1.back(), 100);

        d1.pop_back();
        test_eq("back after pop_back", d1.back(), 99);
        d1.pop_front();
        test_eq("front after pop_front", d1.back(), 99);


        for (int i = 0; i <= 100; ++i)
            d1.push_front(i);
        test_eq("front after bucket test", d1.front(), 100);

        std::deque<int> d2{};

        d2.push_front(42);
        test_eq("push_front to empty equivalence", d2[0], 42);
        test_eq("push_front to empty size", d2.size(), 1U);

        d2.push_back(21);
        test_eq("push_back after push_front equivalence", d2[1], 21);
        test_eq("push_back after push_front size", d2.size(), 2U);

        d2.clear();
        test_eq("clear() - empty()", d2.empty(), true);
        test_eq("clear() - iterators", d2.begin(), d2.end());
    }

    void deque_test::test_operations()
    {
        auto check1 = {
            1, 2, 3, 4, 11, 22, 33, 44, 55, 66, 77, 88,
            5, 6, 7, 8, 9, 10, 11, 12
        };
        auto to_insert = {11, 22, 33, 44, 55, 66, 77, 88};

        std::deque<int> d1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        std::deque<int> d2{d1};
        std::deque<int> d3{d1};
        std::deque<int> d4{d1};

        d1.insert(d1.begin() + 5, to_insert.begin(), to_insert.end());
        test_eq(
            "insert iterator range",
            check1.begin(), check1.end(),
            d1.begin(), d1.end()
        );

        d2.insert(d2.begin() + 5, to_insert);
        test_eq(
            "insert initializer list",
            check1.begin(), check1.end(),
            d2.begin(), d2.end()
        );

        auto check2 = {
            1, 2, 3, 4, 99, 99, 99, 99, 99, 99, 99, 99,
            5, 6, 7, 8, 9, 10, 11, 12
        };
        d3.insert(d3.begin() + 5, 8U, 99);
        test_eq(
            "insert value n times",
            check2.begin(), check2.end(),
            d3.begin(), d3.end()
        );

        auto check3 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        d3.erase(d3.begin() + 4, d3.begin() + 12);
        test_eq(
            "erase iterator range",
            check3.begin(), check3.end(),
            d3.begin(), d3.end()
        );

        auto check4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12};
        d3.erase(d3.begin() + 10);
        test_eq(
            "erase",
            check4.begin(), check4.end(),
            d3.begin(), d3.end()
        );

        d2.swap(d3);
        test_eq(
            "swap1",
            check1.begin(), check1.end(),
            d3.begin(), d3.end()
        );
        test_eq(
            "swap2",
            check4.begin(), check4.end(),
            d2.begin(), d2.end()
        );
    }
}
