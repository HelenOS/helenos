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
#include <utility>

namespace std::test
{
    bool algorithm_test::run(bool report)
    {
        report_ = report;
        start();

        test_non_modifying();

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
}
