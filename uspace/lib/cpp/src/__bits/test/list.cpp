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
}

