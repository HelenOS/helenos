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
#include <unordered_map>
#include <string>
#include <sstream>
#include <utility>

namespace std::test
{
    bool unordered_map_test::run(bool report)
    {
        report_ = report;
        start();

        test_constructors_and_assignment();
        test_histogram();
        test_emplace_insert();
        test_multi();

        return end();
    }

    const char* unordered_map_test::name()
    {
        return "unordered_map";
    }

    void unordered_map_test::test_constructors_and_assignment()
    {
        auto check1 = {1, 2, 3, 4, 5, 6, 7};
        auto src1 = {
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{7, 7},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{4, 4}
        };

        std::unordered_map<int, int> m1{src1};
        test_contains(
            "initializer list initialization",
            check1.begin(), check1.end(), m1
        );
        test_eq("size", m1.size(), 7U);

        std::unordered_map<int, int> m2{src1.begin(), src1.end()};
        test_contains(
            "iterator range initialization",
            check1.begin(), check1.end(), m2
        );

        std::unordered_map<int, int> m3{m1};
        test_contains(
            "copy initialization",
            check1.begin(), check1.end(), m3
        );

        std::unordered_map<int, int> m4{std::move(m1)};
        test_contains(
            "move initialization",
            check1.begin(), check1.end(), m4
        );
        test_eq("move initialization - origin empty", m1.size(), 0U);
        test_eq("empty", m1.empty(), true);

        m1 = m4;
        test_contains(
            "copy assignment",
            check1.begin(), check1.end(), m1
        );

        m4 = std::move(m1);
        test_contains(
            "move assignment",
            check1.begin(), check1.end(), m4
        );
        test_eq("move assignment - origin empty", m1.size(), 0U);

        m1 = src1;
        test_contains(
            "initializer list assignment",
            check1.begin(), check1.end(), m1
        );
    }

    void unordered_map_test::test_histogram()
    {
        std::string str{"a b a a c d b e a b b e d c a e"};
        std::unordered_map<std::string, std::size_t> unordered_map{};
        std::istringstream iss{str};
        std::string word{};

        while (iss >> word)
            ++unordered_map[word];

        test_eq("histogram pt1", unordered_map["a"], 5U);
        test_eq("histogram pt2", unordered_map["b"], 4U);
        test_eq("histogram pt3", unordered_map["c"], 2U);
        test_eq("histogram pt4", unordered_map["d"], 2U);
        test_eq("histogram pt5", unordered_map["e"], 3U);
        test_eq("histogram pt6", unordered_map["f"], 0U);
        test_eq("at", unordered_map.at("a"), 5U);
    }

    void unordered_map_test::test_emplace_insert()
    {
        std::unordered_map<int, int> map1{};

        auto res1 = map1.emplace(1, 2);
        test_eq("first emplace succession", res1.second, true);
        test_eq("first emplace equivalence pt1", res1.first->first, 1);
        test_eq("first emplace equivalence pt2", res1.first->second, 2);

        auto res2 = map1.emplace(1, 3);
        test_eq("second emplace failure", res2.second, false);
        test_eq("second emplace equivalence pt1", res2.first->first, 1);
        test_eq("second emplace equivalence pt2", res2.first->second, 2);

        auto res3 = map1.emplace_hint(map1.begin(), 2, 4);
        test_eq("first emplace_hint succession", (res3 != map1.end()), true);
        test_eq("first emplace_hint equivalence pt1", res3->first, 2);
        test_eq("first emplace_hint equivalence pt2", res3->second, 4);

        auto res4 = map1.emplace_hint(map1.begin(), 2, 5);
        test_eq("second emplace_hint failure", (res4 != map1.end()), true);
        test_eq("second emplace_hint equivalence pt1", res4->first, 2);
        test_eq("second emplace_hint equivalence pt2", res4->second, 4);

        std::unordered_map<int, std::string> map2{};
        auto res5 = map2.insert(std::pair<const int, const char*>{5, "A"});
        test_eq("conversion insert succession", res5.second, true);
        test_eq("conversion insert equivalence pt1", res5.first->first, 5);
        test_eq("conversion insert equivalence pt2", res5.first->second, std::string{"A"});

        auto res6 = map2.insert(std::pair<const int, std::string>{6, "B"});
        test_eq("first insert succession", res6.second, true);
        test_eq("first insert equivalence pt1", res6.first->first, 6);
        test_eq("first insert equivalence pt2", res6.first->second, std::string{"B"});

        auto res7 = map2.insert(std::pair<const int, std::string>{6, "C"});
        test_eq("second insert failure", res7.second, false);
        test_eq("second insert equivalence pt1", res7.first->first, 6);
        test_eq("second insert equivalence pt2", res7.first->second, std::string{"B"});

        auto res8 = map2.insert_or_assign(6, std::string{"D"});
        test_eq("insert_or_*assign* result", res8.second, false);
        test_eq("insert_or_*assign* equivalence pt1", res8.first->first, 6);
        test_eq("insert_or_*assign* equivalence pt2", res8.first->second, std::string{"D"});

        auto res9 = map2.insert_or_assign(7, std::string{"E"});
        test_eq("*insert*_or_assign result", res9.second, true);
        test_eq("*insert*_or_assign equivalence pt1", res9.first->first, 7);
        test_eq("*insert*_or_assign equivalence pt2", res9.first->second, std::string{"E"});

        map2.erase(map2.find(7));
        test_eq("erase", map2.find(7), map2.end());

        auto res10 = map2.erase(6);
        test_eq("erase by key pt1", res10, 1U);
        auto res11 = map2.erase(6);
        test_eq("erase by key pt2", res11, 0U);

        auto res12 = map2.insert(std::pair<const int, const char*>{11, "test"});
        test_eq("insert with constructible argument pt1", res12.second, true);
        test_eq("insert with constructible argument pt2", res12.first->first, 11);
        test_eq("insert with constructible argument pt3", res12.first->second, std::string{"test"});

        std::unordered_map<int, int> map3{};
        map3[1] = 1;
        auto res13 = map3.count(1);
        test_eq("count", res13, 1U);

        map2.clear();
        test_eq("clear", map2.empty(), true);
    }

    void unordered_map_test::test_multi()
    {
        auto check_keys = {1, 2, 3, 4, 5, 6, 7};
        auto check_counts = {1U, 1U, 2U, 1U, 1U, 3U, 1U};
        auto src = {
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{7, 7},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{4, 4}
        };

        std::unordered_multimap<int, int> mmap{src};
        test_contains_multi(
            "multi construction",
            check_keys.begin(), check_keys.end(),
            check_counts.begin(), mmap
        );

        auto res1 = mmap.count(6);
        test_eq("multi count", res1, 3U);

        auto res2 = mmap.emplace(7, 2);
        test_eq("multi duplicit emplace pt1", res2->first, 7);
        test_eq("multi duplicit emplace pt2", res2->second, 2);
        test_eq("multi duplicit emplace pt3", mmap.count(7), 2U);

        auto res3 = mmap.emplace(8, 5);
        test_eq("multi unique emplace pt1", res3->first, 8);
        test_eq("multi unique emplace pt2", res3->second, 5);
        test_eq("multi unique emplace pt3", mmap.count(8), 1U);

        auto res4 = mmap.insert(std::pair<const int, int>{8, 6});
        test_eq("multi duplicit insert pt1", res4->first, 8);
        test_eq("multi duplicit insert pt2", res4->second, 6);
        test_eq("multi duplicit insert pt3", mmap.count(8), 2U);

        auto res5 = mmap.insert(std::pair<const int, int>{9, 8});
        test_eq("multi unique insert pt1", res5->first, 9);
        test_eq("multi unique insert pt2", res5->second, 8);
        test_eq("multi unique insert pt3", mmap.count(9), 1U);

        auto res6 = mmap.erase(8);
        test_eq("multi erase by key pt1", res6, 2U);
        test_eq("multi erase by key pt2", mmap.count(8), 0U);

        // To check that the bucket still works.
        mmap.insert(std::pair<const int, int>{8, 8});
        test_eq("multi erase keeps bucket intact", (mmap.find(8) != mmap.end()), true);

        auto res7 = mmap.erase(mmap.find(7));
        test_eq("multi erase by iterator pt1", res7->first, 7);
        test_eq("multi erase by iterator pt2", mmap.count(7), 1U);
    }
}
