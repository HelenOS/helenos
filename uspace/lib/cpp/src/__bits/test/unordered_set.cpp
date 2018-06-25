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
#include <unordered_set>
#include <string>
#include <utility>

namespace std::test
{
    bool unordered_set_test::run(bool report)
    {
        report_ = report;
        start();

        test_constructors_and_assignment();
        test_emplace_insert();
        test_multi();

        return end();
    }

    const char* unordered_set_test::name()
    {
        return "unordered_set";
    }

    void unordered_set_test::test_constructors_and_assignment()
    {
        auto check1 = {1, 2, 3, 4, 5, 6, 7};
        auto src1 = {3, 1, 5, 2, 7, 6, 4};

        std::unordered_set<int> s1{src1};
        test_contains(
            "initializer list initialization",
            check1.begin(), check1.end(), s1
        );
        test_eq("size", s1.size(), 7U);

        std::unordered_set<int> s2{src1.begin(), src1.end()};
        test_contains(
            "iterator range initialization",
            check1.begin(), check1.end(), s2
        );

        std::unordered_set<int> s3{s1};
        test_contains(
            "copy initialization",
            check1.begin(), check1.end(), s3
        );

        std::unordered_set<int> s4{std::move(s1)};
        test_contains(
            "move initialization",
            check1.begin(), check1.end(), s4
        );
        test_eq("move initialization - origin empty", s1.size(), 0U);
        test_eq("empty", s1.empty(), true);

        s1 = s4;
        test_contains(
            "copy assignment",
            check1.begin(), check1.end(), s1
        );

        s4 = std::move(s1);
        test_contains(
            "move assignment",
            check1.begin(), check1.end(), s4
        );
        test_eq("move assignment - origin empty", s1.size(), 0U);

        s1 = src1;
        test_contains(
            "initializer list assignment",
            check1.begin(), check1.end(), s1
        );
    }

    void unordered_set_test::test_emplace_insert()
    {
        std::unordered_set<int> set1{};

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

        std::unordered_set<std::string> set2{};
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

        std::unordered_set<int> set3{};
        set3.insert(1);
        auto res13 = set3.erase(1);
        test_eq("erase only element by key pt1", res13, 1U);
        test_eq("erase only element by key pt2", set3.empty(), true);

        set3.insert(3);
        auto res14 = set3.erase(set3.begin());
        test_eq("erase only element by iterator pt1", res14, set3.end());
        test_eq("erase only element by iterator pt2", set3.empty(), true);

        set2.clear();
        test_eq("clear", set2.empty(), true);

        set3.insert(1);
        auto res15 = set3.count(1);
        test_eq("count", res15, 1U);

        set3.insert(15);
        auto res16 = set3.find(15);
        test_eq("find", *res16, 15);
    }

    void unordered_set_test::test_multi()
    {
        auto check_keys = {1, 2, 3, 4, 5, 6, 7};
        auto check_counts = {1U, 1U, 2U, 1U, 1U, 3U, 1U};
        auto src1 = {3, 6, 1, 5, 6, 3, 2, 7, 6, 4};

        std::unordered_multiset<int> mset{src1};
        test_contains_multi(
            "multi construction",
            check_keys.begin(), check_keys.end(),
            check_counts.begin(), mset
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
}
