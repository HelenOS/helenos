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

#include <array>
#include <initializer_list>
#include <internal/test/tests.hpp>
#include <numeric>
#include <utility>

namespace std::test
{
    bool numeric_test::run(bool report)
    {
        report_ = report;
        start();

        test_algorithms();

        return end();
    }

    const char* numeric_test::name()
    {
        return "numeric";
    }

    void numeric_test::test_algorithms()
    {
        auto data1 = {1, 2, 3, 4, 5};

        auto res1 = std::accumulate(data1.begin(), data1.end(), 5);
        test_eq("accumulate pt1", res1, 20);

        auto res2 = std::accumulate(
            data1.begin(), data1.end(), 2,
            [](const auto& lhs, const auto& rhs){
                return lhs * rhs;
            }
        );
        test_eq("accumulate pt2", res2, 240);

        auto res3 = std::accumulate(data1.begin(), data1.begin(), 10);
        test_eq("accumulate pt3", res3, 10);

        auto data2 = {3, 5, 2, 8, 7};
        auto data3 = {4, 6, 1, 0, 5};

        auto res4 = std::inner_product(
            data2.begin(), data2.end(), data3.begin(), 0
        );
        test_eq("inner_product pt1", res4, 79);

        auto res5 = std::inner_product(
            data2.begin(), data2.end(), data3.begin(), 10,
            [](const auto& lhs, const auto& rhs){
                return lhs + rhs;
            },
            [](const auto& lhs, const auto& rhs){
                return 2 * (lhs + rhs);
            }
        );
        test_eq("inner_product pt2", res5, 92);

        auto data4 = {1, 3, 2, 4, 5};
        auto check1 = {1, 4, 6, 10, 15};
        std::array<int, 5> result{};

        auto res6 = std::partial_sum(
            data4.begin(), data4.end(), result.begin()
        );
        test_eq(
            "partial sum pt1",
            check1.begin(), check1.end(),
            result.begin(), result.end()
        );
        test_eq("partial sum pt2", res6, result.end());

        auto check2 = {1, 3, 6, 24, 120};
        auto res7 = std::partial_sum(
            data4.begin(), data4.end(), result.begin(),
            [](const auto& lhs, const auto& rhs){
                return lhs * rhs;
            }
        );
        test_eq(
            "partial sum pt3",
            check2.begin(), check2.end(),
            result.begin(), result.end()
        );
        test_eq("partial sum pt4", res7, result.end());

        auto check3 = {1, 2, -1, 2, 1};
        auto res8 = std::adjacent_difference(
            data4.begin(), data4.end(), result.begin()
        );
        test_eq(
            "adjacent_difference pt1",
            check3.begin(), check3.end(),
            result.begin(), result.end()
        );
        test_eq("adjacent_difference pt2", res8, result.end());

        auto check4 = {1, 3, 6, 8, 20};
        auto res9 = std::adjacent_difference(
            data4.begin(), data4.end(), result.begin(),
            [](const auto& lhs, const auto& rhs){
                return lhs * rhs;
            }
        );
        test_eq(
            "adjacent_difference pt3",
            check4.begin(), check4.end(),
            result.begin(), result.end()
        );
        test_eq("adjacent_difference pt4", res9, result.end());

        auto check5 = {4, 5, 6, 7, 8};
        std::iota(result.begin(), result.end(), 4);
        test_eq(
            "iota", check5.begin(), check5.end(),
            result.begin(), result.end()
        );
    }
}

