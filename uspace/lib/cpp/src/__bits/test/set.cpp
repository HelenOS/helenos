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
#include <set>
#include <string>
#include <utility>

namespace std::test
{
    bool set_test::run(bool report)
    {
        report_ = report;
        start();

        test_constructors_and_assignment();
        test_emplace_insert();
        test_bounds_and_ranges();
        test_multi();
        test_reverse_iterators();
        test_multi_bounds_and_ranges();

        return end();
    }

    const char* set_test::name()
    {
        return "set";
    }

    void set_test::test_constructors_and_assignment()
    {
        auto check1 = {1, 2, 3, 4, 5, 6, 7};
        auto src1 = {3, 1, 5, 2, 7, 6, 4};

        std::set<int> s1{src1};
        test_eq(
            "initializer list initialization",
            check1.begin(), check1.end(),
            s1.begin(), s1.end()
        );
        test_eq("size", s1.size(), 7U);

        std::set<int> s2{src1.begin(), src1.end()};
        test_eq(
            "iterator range initialization",
            check1.begin(), check1.end(),
            s2.begin(), s2.end()
        );

        std::set<int> s3{s1};
        test_eq(
            "copy initialization",
            check1.begin(), check1.end(),
            s3.begin(), s3.end()
        );

        std::set<int> s4{std::move(s1)};
        test_eq(
            "move initialization",
            check1.begin(), check1.end(),
            s4.begin(), s4.end()
        );
        test_eq("move initialization - origin empty", s1.size(), 0U);
        test_eq("empty", s1.empty(), true);

        s1 = s4;
        test_eq(
            "copy assignment",
            check1.begin(), check1.end(),
            s1.begin(), s1.end()
        );

        s4 = std::move(s1);
        test_eq(
            "move assignment",
            check1.begin(), check1.end(),
            s4.begin(), s4.end()
        );
        test_eq("move assignment - origin empty", s1.size(), 0U);

        s1 = src1;
        test_eq(
            "initializer list assignment",
            check1.begin(), check1.end(),
            s1.begin(), s1.end()
        );
    }

    void set_test::test_emplace_insert()
    {
        std::set<int> set1{};

        auto res1 = set1.emplace(1);
        test_eq("first emplace succession", res1.second, true);
        test_eq("first emplace equivalence", *res1.first, 1);

        auto res2 = set1.emplace(1);
        test_eq("second emplace failure", res2.second, false);
        test_eq("second emplace equivalence", *res2.first, 1);

        auto res3 = set1.emplace_hint(set1.begin(), 2);
        test_eq("first emplace_hint succession", (res3 != set1.end()), true);
        test_eq("first emplace_hint equivalence", *res3, 2);

        auto res4 = set1.emplace_hint(set1.begin(), 2);
        test_eq("second emplace_hint failure", (res4 != set1.end()), true);
        test_eq("second emplace_hint equivalence", *res4, 2);

        std::set<std::string> set2{};
        auto res5 = set2.insert("A");
        test_eq("conversion insert succession", res5.second, true);
        test_eq("conversion insert equivalence", *res5.first, std::string{"A"});

        auto res6 = set2.insert(std::string{"B"});
        test_eq("first insert succession", res6.second, true);
        test_eq("first insert equivalence", *res6.first, std::string{"B"});

        auto res7 = set2.insert(std::string{"B"});
        test_eq("second insert failure", res7.second, false);
        test_eq("second insert equivalence", *res7.first, std::string{"B"});

        auto res10 = set1.erase(set1.find(2));
        test_eq("erase", set1.find(2), set1.end());
        test_eq("highest erased", res10, set1.end());

        set2.insert(std::string{"G"});
        set2.insert(std::string{"H"});
        set2.insert(std::string{"K"});
        auto res11 = set2.erase(std::string{"G"});
        test_eq("erase by key pt1", res11, 1U);
        auto res12 = set2.erase(std::string{"M"});
        test_eq("erase by key pt2", res12, 0U);

        std::set<int> set3{};
        set3.insert(1);
        auto res13 = set3.erase(1);
        test_eq("erase root by key pt1", res13, 1U);
        test_eq("erase root by key pt2", set3.empty(), true);

        set3.insert(3);
        auto res14 = set3.erase(set3.begin());
        test_eq("erase root by iterator pt1", res14, set3.end());
        test_eq("erase root by iterator pt2", set3.empty(), true);

        set2.clear();
        test_eq("clear", set2.empty(), true);

        set3.insert(1);
        auto res15 = set3.count(1);
        test_eq("count", res15, 1U);
    }

    void set_test::test_bounds_and_ranges()
    {
        std::set<int> set{};
        for (int i = 0; i < 10; ++i)
            set.insert(i);
        for (int i = 15; i < 20; ++i)
            set.insert(i);

        auto res1 = set.lower_bound(5);
        test_eq("lower_bound of present key", *res1, 5);

        auto res2 = set.lower_bound(13);
        test_eq("lower_bound of absent key", *res2, 9);

        auto res3 = set.upper_bound(7);
        test_eq("upper_bound of present key", *res3, 8);

        auto res4 = set.upper_bound(12);
        test_eq("upper_bound of absent key", *res4, 15);

        auto res5 = set.equal_range(4);
        test_eq("equal_range of present key pt1", *res5.first, 4);
        test_eq("equal_range of present key pt2", *res5.second, 5);

        auto res6 = set.equal_range(14);
        test_eq("equal_range of absent key pt1", *res6.first, 9);
        test_eq("equal_range of absent key pt2", *res6.second, 15);
    }

    void set_test::test_multi()
    {
        auto check1 = {1, 2, 3, 3, 4, 5, 6, 6, 6, 7};
        auto src1 = {3, 6, 1, 5, 6, 3, 2, 7, 6, 4};

        std::multiset<int> mset{src1};
        test_eq(
            "multi construction",
            check1.begin(), check1.end(),
            mset.begin(), mset.end()
        );

        auto res1 = mset.count(6);
        test_eq("multi count", res1, 3U);

        auto res2 = mset.emplace(7);
        test_eq("multi duplicit emplace pt1", *res2, 7);
        test_eq("multi duplicit emplace pt2", mset.count(7), 2U);

        auto res3 = mset.emplace(8);
        test_eq("multi unique emplace pt1", *res3, 8);
        test_eq("multi unique emplace pt2", mset.count(8), 1U);

        auto res4 = mset.insert(8);
        test_eq("multi duplicit insert pt1", *res4, 8);
        test_eq("multi duplicit insert pt2", mset.count(8), 2U);

        auto res5 = mset.insert(9);
        test_eq("multi unique insert pt1", *res5, 9);
        test_eq("multi unique insert pt2", mset.count(9), 1U);

        auto res6 = mset.erase(8);
        test_eq("multi erase by key pt1", res6, 2U);
        test_eq("multi erase by key pt2", mset.count(8), 0U);

        auto res7 = mset.erase(mset.find(7));
        test_eq("multi erase by iterator pt1", *res7, 7);
        test_eq("multi erase by iterator pt2", mset.count(7), 1U);
    }

    void set_test::test_reverse_iterators()
    {
        auto check1 = {7, 6, 6, 6, 5, 4, 3, 3, 2, 1};
        auto src1 = {3, 6, 1, 5, 6, 3, 2, 7, 6, 4};

        std::multiset<int> mset{src1};
        test_eq(
            "multi reverse iterators",
            check1.begin(), check1.end(),
            mset.rbegin(), mset.rend()
        );

        auto check2 = {7, 6, 5, 4, 3, 2, 1};
        auto src2 = {3, 1, 5, 2, 7, 6, 4};

        std::set<int> set{src2};
        test_eq(
            "reverse iterators",
            check2.begin(), check2.end(),
            set.rbegin(), set.rend()
        );
    }

    void set_test::test_multi_bounds_and_ranges()
    {
        auto check1 = {1, 1};
        auto check2 = {5, 5, 5};
        auto check3 = {6};
        auto src = {1, 1, 2, 3, 5, 5, 5, 6};

        std::multiset<int> mset{src};
        auto res1 = mset.equal_range(1);
        test_eq(
            "multi equal_range at the start",
            check1.begin(), check1.end(),
            res1.first, res1.second
        );

        auto res2 = mset.equal_range(5);
        test_eq(
            "multi equal_range in the middle",
            check2.begin(), check2.end(),
            res2.first, res2.second
        );

        auto res3 = mset.equal_range(6);
        test_eq(
            "multi equal_range at the end + single element range",
            check3.begin(), check3.end(),
            res3.first, res3.second
        );
    }
}
