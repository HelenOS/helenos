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
#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace std::test
{
    bool algorithm_test::run(bool report)
    {
        report_ = report;
        start();

        test_non_modifying();
        test_mutating();

        return end();
    }

    const char* algorithm_test::name()
    {
        return "algorithm";
    }

    void algorithm_test::test_non_modifying()
    {
        auto data1 = {1, 2, 3, 4, 5};
        auto res1 = std::all_of(
            data1.begin(), data1.end(),
            [](auto x){ return x > 0; }
        );
        auto res2 = std::all_of(
            data1.begin(), data1.end(),
            [](auto x){ return x < 4; }
        );

        test("all_of pt1", res1);
        test("all_of pt2", !res2);

        auto res3 = std::any_of(
            data1.begin(), data1.end(),
            [](auto x){ return x > 4; }
        );
        auto res4 = std::any_of(
            data1.begin(), data1.end(),
            [](auto x){ return x == 10; }
        );

        test("any_of pt1", res3);
        test("any_of pt2", !res4);

        auto res5 = std::none_of(
            data1.begin(), data1.end(),
            [](auto x){ return x < 0; }
        );
        auto res6 = std::none_of(
            data1.begin(), data1.end(),
            [](auto x){ return x == 4; }
        );

        test("none_of pt1", res5);
        test("none_of pt2", !res6);

        std::array<int, 5> data2{1, 2, 3, 4, 5};
        auto check1 = {1, 20, 3, 40, 5};
        std::for_each(
            data2.begin(), data2.end(),
            [](auto& x){
                if (x % 2 == 0)
                    x *= 10;
            }
        );

        test_eq(
            "for_each", check1.begin(), check1.end(),
            data2.begin(), data2.end()
        );

        auto res7 = std::find(data2.begin(), data2.end(), 40);
        test_eq("find", res7, &data2[3]);

        auto res8 = std::find_if(
            data2.begin(), data2.end(),
            [](auto x){ return x > 30; }
        );
        test_eq("find_if", res8, &data2[3]);

        auto res9 = std::find_if_not(
            data2.begin(), data2.end(),
            [](auto x){ return x < 30; }
        );
        test_eq("find_if_not", res9, &data2[3]);

        // TODO: find_end, find_first_of

        std::array<int, 7> data3{1, 2, 3, 3, 4, 6, 5};
        auto res10 = std::adjacent_find(data3.begin(), data3.end());
        test_eq("adjacent_find pt1", res10, &data3[2]);

        auto res11 = std::adjacent_find(
            data3.begin(), data3.end(),
            [](auto lhs, auto rhs){
                return rhs < lhs;
            }
        );
        test_eq("adjacent_find pt2", res11, &data3[5]);

        auto res12 = std::count(data3.begin(), data3.end(), 3);
        test_eq("count", res12, 2);

        auto res13 = std::count_if(
            data3.begin(), data3.end(),
            [](auto x){
                return x % 2 == 1;
            }
        );
        test_eq("count_if", res13, 4);

        std::array<int, 6> data4{1, 2, 3, 4, 5, 6};
        std::array<int, 6> data5{1, 2, 3, 4, 6, 5};
        auto res14 = std::mismatch(
            data4.begin(), data4.end(),
            data5.begin(), data5.end()
        );

        test_eq("mismatch pt1", res14.first, &data4[4]);
        test_eq("mismatch pt2", res14.second, &data5[4]);

        auto res15 = std::equal(
            data4.begin(), data4.end(),
            data4.begin(), data4.end()
        );
        test("equal pt1", res15);

        auto res16 = std::equal(
            data4.begin(), data4.end(),
            data5.begin(), data5.end()
        );
        test("equal pt2", !res16);

        auto res17 = std::equal(
            data4.begin(), data4.end(),
            data4.begin(), data4.end(),
            [](auto lhs, auto rhs){
                return lhs == rhs;
            }
        );
        test("equal pt3", res17);

        auto res18 = std::equal(
            data4.begin(), data4.end(),
            data5.begin(), data5.end(),
            [](auto lhs, auto rhs){
                return lhs == rhs;
            }
        );
        test("equal pt4", !res18);

        // TODO: is_permutation, search
    }

    void algorithm_test::test_mutating()
    {
        auto check1 = {1, 2, 3, 10, 20, 30, 40};
        std::array<int, 4> data1{10, 20, 30, 40};
        std::array<int, 7> data2{1, 2, 3, 4, 5, 6, 7};

        auto res1 = std::copy(
            data1.begin(), data1.end(), data2.begin() + 3
        );

        test_eq(
            "copy pt1", check1.begin(), check1.end(),
            data2.begin(), data2.end()
        );
        test_eq("copy pt2", res1, data2.end());

        auto check2 = {1, 2, 3, 10, 20, 30, 7, 8};
        std::array<int, 8> data3{1, 2, 3, 4, 5, 6, 7, 8};

        auto res2 = std::copy_n(
            data1.begin(), 3, data3.begin() + 3
        );
        test_eq(
            "copy_n pt1", check2.begin(), check2.end(),
            data3.begin(), data3.end()
        );
        test_eq("copy_n pt2", res2, &data3[6]);

        auto check3 = {2, 4, 6, 8};
        std::array<int, 4> data4{0, 0, 0, 0};
        std::array<int, 9> data5{1, 2, 3, 4, 5, 6, 7, 8, 9};

        auto res3 = std::copy_if(
            data5.begin(), data5.end(), data4.begin(),
            [](auto x){ return x % 2 == 0; }
        );

        test_eq(
            "copy_if pt1", check3.begin(), check3.end(),
            data4.begin(), data4.end()
        );
        test_eq("copy_if pt2", res3, data4.end());

        /**
         * Note: Not testing copy_backwards as it isn't
         *       really easy to test, but since it is
         *       widely used in the rest of the library
         *       (mainly right shifting in sequence
         *       containers) it's tested by other tests.
         */

        auto check4 = {std::string{"A"}, std::string{"B"}, std::string{"C"}};
        std::array<std::string, 3> data6{"A", "B", "C"};
        std::array<std::string, 3> data7{"", "", ""};

        auto res4 = std::move(
            data6.begin(), data6.end(), data7.begin()
        );
        test_eq(
            "move pt1", check4.begin(), check4.end(),
            data7.begin(), data7.end()
        );
        test(
            "move pt2", (data6[0].empty() && data6[1].empty(),
            data6[2].empty())
        );
        test_eq("move pt3", res4, data7.end());

        auto check5 = {1, 2, 3, 4};
        auto check6 = {10, 20, 30, 40};
        std::array<int, 4> data8{1, 2, 3, 4};
        std::array<int, 4> data9{10, 20, 30, 40};

        auto res5 = std::swap_ranges(
            data8.begin(), data8.end(),
            data9.begin()
        );

        test_eq(
            "swap_ranges pt1", check6.begin(), check6.end(),
            data8.begin(), data8.end()
        );
        test_eq(
            "swap_ranges pt2", check5.begin(), check5.end(),
            data9.begin(), data9.end()
        );
        test_eq("swap_ranges pt3", res5, data9.end());

        std::iter_swap(data8.begin(), data9.begin());
        test_eq("swap_iter pt1", data8[0], 1);
        test_eq("swap_iter pt2", data9[0], 10);

        auto check7 = {2, 3, 4, 5};
        std::array<int, 4> data10{1, 2, 3, 4};

        auto res6 = std::transform(
            data10.begin(), data10.end(), data10.begin(),
            [](auto x) { return x + 1; }
        );
        test_eq(
            "transform pt1",
            check7.begin(), check7.end(),
            data10.begin(), data10.end()
        );
        test_eq("transform pt2", res6, data10.end());
    }
}
