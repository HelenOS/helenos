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
#include <initializer_list>
#include <list>
#include <utility>

namespace std::test
{
    bool list_test::run(bool report)
    {
        report_ = report;
        start();

        test_construction_and_assignment();
        test_modifiers();

        return end();
    }

    const char* list_test::name()
    {
        return "list";
    }

    void list_test::test_construction_and_assignment()
    {
        auto check1 = {1, 1, 1, 1, 1, 1};
        auto check2 = {1, 2, 3, 4, 5, 6};

        std::list<int> l1(6U, 1);
        test_eq(
            "n*value initialization",
            check1.begin(), check1.end(),
            l1.begin(), l1.end()
        );

        std::list<int> l2{check2};
        test_eq(
            "initializer_list initialization",
            check2.begin(), check2.end(),
            l2.begin(), l2.end()
        );

        std::list<int> l3{check2.begin(), check2.end()};
        test_eq(
            "iterator range initialization",
            check2.begin(), check2.end(),
            l3.begin(), l3.end()
        );

        std::list<int> l4{l3};
        test_eq(
            "copy initialization",
            check2.begin(), check2.end(),
            l4.begin(), l4.end()
        );
        test_eq("size", l4.size(), 6U);
        test_eq("not empty", l4.empty(), false);

        std::list<int> l5{std::move(l4)};
        test_eq(
            "move initialization",
            check2.begin(), check2.end(),
            l5.begin(), l5.end()
        );
        test_eq("move initializaiton - origin empty pt1", l4.empty(), true);
        test_eq("move initializaiton - origin empty pt2", l4.size(), 0U);

        l4 = l5;
        test_eq(
            "copy assignment",
            l5.begin(), l5.end(),
            l4.begin(), l4.end()
        );
        test_eq("copy assignment size", l4.size(), l5.size());

        l1 = std::move(l4);
        test_eq(
            "move assignment",
            l5.begin(), l5.end(),
            l1.begin(), l1.end()
        );
        test_eq("move assignment - origin empty", l4.empty(), true);

        auto check3 = {5, 4, 3, 2, 1};
        l4 = check3;
        test_eq(
            "initializer_list assignment pt1",
            check3.begin(), check3.end(),
            l4.begin(), l4.end()
        );
        test_eq("initializer_list assignment pt2", l4.size(), 5U);

        l5.assign(check3.begin(), check3.end());
        test_eq(
            "iterator range assign() pt1",
            check3.begin(), check3.end(),
            l5.begin(), l5.end()
        );
        test_eq("iterator range assign() pt2", l5.size(), 5U);

        l5.assign(6U, 1);
        test_eq(
            "n*value assign() pt1",
            check1.begin(), check1.end(),
            l5.begin(), l5.end()
        );
        test_eq("n*value assign() pt2", l5.size(), 6U);

        l5.assign(check3);
        test_eq(
            "initializer_list assign() pt1",
            check3.begin(), check3.end(),
            l5.begin(), l5.end()
        );
        test_eq("initializer_list assign() pt2", l5.size(), 5U);

        auto check4 = {1, 2, 3, 4, 5};
        test_eq(
            "reverse iterators",
            check4.begin(), check4.end(),
            l5.rbegin(), l5.rend()
        );

        test_eq("front", l5.front(), 5);
        test_eq("back", l5.back(), 1);
    }

    void list_test::test_modifiers()
    {
        std::list<int> l1{};
        test_eq("empty list", l1.empty(), true);

        l1.push_back(1);
        test_eq("empty list push_back pt1", l1.size(), 1U);
        test_eq("empty list push_back pt2", l1.empty(), false);
        test_eq("empty list push_back pt3", l1.front(), 1);
        test_eq("empty list push_back pt4", l1.back(), 1);

        l1.push_front(2);
        test_eq("push_front pt1", l1.size(), 2U);
        test_eq("push_front pt2", l1.front(), 2);
        test_eq("push_front pt3", l1.back(), 1);

        l1.pop_back();
        test_eq("pop_back pt1", l1.size(), 1U);
        test_eq("pop_back pt2", l1.back(), 2);

        l1.push_front(3);
        test_eq("size", l1.size(), 2U);

        l1.pop_front();
        test_eq("pop_front", l1.front(), 2);

        auto check1 = {2, 42, 42, 42, 42, 42};
        l1.insert(l1.begin(), 5U, 42);
        test_eq(
            "insert n*value",
            check1.begin(), check1.end(),
            l1.begin(), l1.end()
        );

        auto data1 = {33, 34};
        auto check2 = {2, 42, 33, 34, 42, 42, 42, 42};
        auto it1 = l1.begin();
        std::advance(it1, 2);

        l1.insert(it1, data1.begin(), data1.end());
        test_eq(
            "insert iterator range",
            check2.begin(), check2.end(),
            l1.begin(), l1.end()
        );

        auto check3 = {2, 42, 33, 34, 42, 33, 34, 42, 42, 42};
        auto it2 = l1.begin();
        std::advance(it2, 5);

        l1.insert(it2, data1);
        test_eq(
            "insert initializer_list",
            check3.begin(), check3.end(),
            l1.begin(), l1.end()
        );

        auto check4 = {2, 42, 33, 34, 33, 34, 42, 42, 42};
        auto it3 = l1.begin();
        std::advance(it3, 4);

        l1.erase(it3);
        test_eq(
            "erase iterator",
            check4.begin(), check4.end(),
            l1.begin(), l1.end()
        );

        auto check5 = {33, 34, 42, 42, 42};
        auto it4 = l1.begin();
        auto it5 = l1.begin();
        std::advance(it5, 4);

        l1.erase(it4, it5);
        test_eq(
            "erase iterator range",
            check5.begin(), check5.end(),
            l1.begin(), l1.end()
        );

        l1.clear();
        test_eq("clear empty", l1.empty(), true);
        test_eq("clear size", l1.size(), 0U);

        std::list<int> l2{1, 2, 3, 4, 5};
        std::list<int> l3{10, 20, 30, 40, 50};

        auto check6 = {1, 2, 10, 20, 30, 40, 50, 3, 4, 5};
        auto check7 = {1, 2, 10, 20, 30, 40, 50};
        auto check8 = {3, 4, 5};

        auto it6 = l2.begin();
        std::advance(it6, 2);

        l2.splice(it6, l3);
        test_eq(
            "splice pt1",
            check6.begin(), check6.end(),
            l2.begin(), l2.end()
        );
        test_eq("splice pt2", l3.empty(), true);

        l3.splice(l3.begin(), l2, it6, l2.end());
        test_eq(
            "splice pt3",
            check7.begin(), check7.end(),
            l2.begin(), l2.end()
        );
        test_eq(
            "splice pt4",
            check8.begin(), check8.end(),
            l3.begin(), l3.end()
        );
        test_eq("splice size pt1", l2.size(), 7U);
        test_eq("splice size pt2", l3.size(), 3U);

        auto check9 = {1, -1, 2, -2, 3, -3, 4, -4};
        auto check10 = {1, 2, 3, 4};
        std::list<int> l4{1, -1, 2, 5, -2, 5, 3, -3, 5, 4, -4};

        l4.remove(5);
        test_eq(
            "remove",
            check9.begin(), check9.end(),
            l4.begin(), l4.end()
        );
        test_eq("remove size", l4.size(), 8U);

        l4.remove_if([](auto x){ return x < 0; });
        test_eq(
            "remove_if",
            check10.begin(), check10.end(),
            l4.begin(), l4.end()
        );
        test_eq("remove_if size", l4.size(), 4U);

        auto check11 = {1, 2, 3, 2, 4, 5};
        std::list<int> l5{1, 1, 2, 3, 3, 2, 2, 4, 5, 5};

        l5.unique();
        test_eq(
            "unique",
            check11.begin(), check11.end(),
            l5.begin(), l5.end()
        );
        test_eq("unique size", l5.size(), 6U);

        auto check12 = {1, 3, 3, 5, 7, 9, 9};
        std::list<int> l6{1, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 9, 9};

        l6.unique([](auto lhs, auto rhs){ return lhs == rhs + 1; });
        test_eq(
            "unique predicate",
            check12.begin(), check12.end(),
            l6.begin(), l6.end()
        );
        test_eq("unique predicate size", l6.size(), 7U);
    }
}

