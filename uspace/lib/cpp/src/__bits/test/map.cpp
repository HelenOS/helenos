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
#include <map>
#include <string>
#include <sstream>
#include <utility>

namespace std::test
{
    bool map_test::run(bool report)
    {
        report_ = report;
        start();

        test_constructors_and_assignment();
        test_histogram();
        test_emplace_insert();
        test_bounds_and_ranges();
        test_multi();
        test_reverse_iterators();
        test_multi_bounds_and_ranges();

        return end();
    }

    const char* map_test::name()
    {
        return "map";
    }

    void map_test::test_constructors_and_assignment()
    {
        auto check1 = {
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{4, 4},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{7, 7}
        };
        auto src1 = {
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{7, 7},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{4, 4}
        };

        std::map<int, int> m1{src1};
        test_eq(
            "initializer list initialization",
            check1.begin(), check1.end(),
            m1.begin(), m1.end()
        );
        test_eq("size", m1.size(), 7U);

        std::map<int, int> m2{src1.begin(), src1.end()};
        test_eq(
            "iterator range initialization",
            check1.begin(), check1.end(),
            m2.begin(), m2.end()
        );

        std::map<int, int> m3{m1};
        test_eq(
            "copy initialization",
            check1.begin(), check1.end(),
            m3.begin(), m3.end()
        );

        std::map<int, int> m4{std::move(m1)};
        test_eq(
            "move initialization",
            check1.begin(), check1.end(),
            m4.begin(), m4.end()
        );
        test_eq("move initialization - origin empty", m1.size(), 0U);
        test_eq("empty", m1.empty(), true);

        m1 = m4;
        test_eq(
            "copy assignment",
            check1.begin(), check1.end(),
            m1.begin(), m1.end()
        );

        m4 = std::move(m1);
        test_eq(
            "move assignment",
            check1.begin(), check1.end(),
            m4.begin(), m4.end()
        );
        test_eq("move assignment - origin empty", m1.size(), 0U);

        m1 = src1;
        test_eq(
            "initializer list assignment",
            check1.begin(), check1.end(),
            m1.begin(), m1.end()
        );
    }

    void map_test::test_histogram()
    {
        std::string str{"a b a a c d b e a b b e d c a e"};
        std::map<std::string, std::size_t> map{};
        std::istringstream iss{str};
        std::string word{};

        while (iss >> word)
            ++map[word];

        test_eq("histogram pt1", map["a"], 5U);
        test_eq("histogram pt2", map["b"], 4U);
        test_eq("histogram pt3", map["c"], 2U);
        test_eq("histogram pt4", map["d"], 2U);
        test_eq("histogram pt5", map["e"], 3U);
        test_eq("histogram pt6", map["f"], 0U);
        test_eq("at", map.at("a"), 5U);
    }

    void map_test::test_emplace_insert()
    {
        std::map<int, int> map1{};

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

        std::map<int, std::string> map2{};
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

        auto res10 = map2.erase(map2.find(7));
        test_eq("erase", map2.find(7), map2.end());
        test_eq("highest erased", res10, map2.end());

        auto res11 = map2.erase(6);
        test_eq("erase by key pt1", res11, 1U);
        auto res12 = map2.erase(6);
        test_eq("erase by key pt2", res12, 0U);

        std::map<int, int> map3{};
        map3[1] = 1;
        auto res13 = map3.erase(1);
        test_eq("erase root by key pt1", res13, 1U);
        test_eq("erase root by key pt2", map3.empty(), true);

        map3[2] = 2;
        auto res14 = map3.erase(map3.begin());
        test_eq("erase root by iterator pt1", res14, map3.end());
        test_eq("erase root by iterator pt2", map3.empty(), true);

        map2.clear();
        test_eq("clear", map2.empty(), true);

        map3[1] = 1;
        auto res15 = map3.count(1);
        test_eq("count", res15, 1U);
    }

    void map_test::test_bounds_and_ranges()
    {
        std::map<int, int> map{};
        for (int i = 0; i < 10; ++i)
            map[i] = i;
        for (int i = 15; i < 20; ++i)
            map[i] = i;

        auto res1 = map.lower_bound(5);
        test_eq("lower_bound of present key", res1->first, 5);

        auto res2 = map.lower_bound(13);
        test_eq("lower_bound of absent key", res2->first, 9);

        auto res3 = map.upper_bound(7);
        test_eq("upper_bound of present key", res3->first, 8);

        auto res4 = map.upper_bound(12);
        test_eq("upper_bound of absent key", res4->first, 15);

        auto res5 = map.equal_range(4);
        test_eq("equal_range of present key pt1", res5.first->first, 4);
        test_eq("equal_range of present key pt2", res5.second->first, 5);

        auto res6 = map.equal_range(14);
        test_eq("equal_range of absent key pt1", res6.first->first, 9);
        test_eq("equal_range of absent key pt2", res6.second->first, 15);
    }

    void map_test::test_multi()
    {
        auto check1 = {
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{4, 4},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{7, 7}
        };
        auto src1 = {
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

        std::multimap<int, int> mmap{src1};
        test_eq(
            "multi construction",
            check1.begin(), check1.end(),
            mmap.begin(), mmap.end()
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

        auto res7 = mmap.erase(mmap.find(7));
        test_eq("multi erase by iterator pt1", res7->first, 7);
        test_eq("multi erase by iterator pt2", mmap.count(7), 1U);
    }

    void map_test::test_reverse_iterators()
    {
        auto check1 = {
            std::pair<const int, int>{7, 7},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{4, 4},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{1, 1}
        };
        auto src1 = {
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

        std::multimap<int, int> mmap{src1};
        test_eq(
            "multi reverse iterators",
            check1.begin(), check1.end(),
            mmap.rbegin(), mmap.rend()
        );

        auto check2 = {
            std::pair<const int, int>{7, 7},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{4, 4},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{1, 1}
        };
        auto src2 = {
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{7, 7},
            std::pair<const int, int>{6, 6},
            std::pair<const int, int>{4, 4}
        };

        std::map<int, int> map{src2};
        test_eq(
            "reverse iterators",
            check2.begin(), check2.end(),
            map.rbegin(), map.rend()
        );
    }

    void map_test::test_multi_bounds_and_ranges()
    {
        auto check1 = {
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{1, 2}
        };
        auto check2 = {
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{5, 6},
            std::pair<const int, int>{5, 7}
        };
        auto check3 = {
            std::pair<const int, int>{6, 6}
        };
        auto src = {
            std::pair<const int, int>{1, 1},
            std::pair<const int, int>{1, 2},
            std::pair<const int, int>{2, 2},
            std::pair<const int, int>{3, 3},
            std::pair<const int, int>{5, 5},
            std::pair<const int, int>{5, 6},
            std::pair<const int, int>{5, 7},
            std::pair<const int, int>{6, 6}
        };

        std::multimap<int, int> mmap{src};
        auto res1 = mmap.equal_range(1);
        test_eq(
            "multi equal_range at the start",
            check1.begin(), check1.end(),
            res1.first, res1.second
        );

        auto res2 = mmap.equal_range(5);
        test_eq(
            "multi equal_range in the middle",
            check2.begin(), check2.end(),
            res2.first, res2.second
        );

        auto res3 = mmap.equal_range(6);
        test_eq(
            "multi equal_range at the end + single element range",
            check3.begin(), check3.end(),
            res3.first, res3.second
        );
    }
}
