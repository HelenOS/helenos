/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <__bits/test/tests.hpp>
#include <algorithm>
#include <array>
#include <initializer_list>

namespace std::test
{
    bool array_test::run(bool report)
    {
        report_ = report;
        start();

        auto check1 = {1, 2, 3, 4};
        auto check2 = {4, 3, 2, 1};
        auto check3 = {5, 5, 5, 5};

        std::array<int, 4> arr1{1, 2, 3, 4};
        test_eq(
            "initializer list construction",
            arr1.begin(), arr1.end(),
            check1.begin(), check1.end()
        );

        auto it = arr1.begin();
        test_eq(
            "iterator increment",
            *(++it), arr1[1]
        );
        test_eq(
            "iterator decrement",
            *(--it), arr1[0]
        );

        std::array<int, 4> arr2{arr1};
        test_eq(
            "copy construction",
            arr1.begin(), arr1.end(),
            arr2.begin(), arr2.end()
        );

        std::reverse(arr2.begin(), arr2.end());
        test_eq(
            "reverse",
            arr2.begin(), arr2.end(),
            check2.begin(), check2.end()
        );
        test_eq(
            "reverse iterator",
            arr1.rbegin(), arr1.rend(),
            arr2.begin(), arr2.end()
        );


        std::array<int, 4> arr3{};
        arr3.fill(5);
        test_eq(
            "fill",
            arr3.begin(), arr3.end(),
            check3.begin(), check3.end()
        );

        arr2.swap(arr3);
        test_eq(
            "swap part 1",
            arr2.begin(), arr2.end(),
            check3.begin(), check3.end()
        );
        test_eq(
            "swap part 2",
            arr3.begin(), arr3.end(),
            check2.begin(), check2.end()
        );

        // TODO: test bound checking of at when implemented

        std::array<int, 3> arr4{1, 2, 3};
        auto [a, b, c] = arr4;
        test_eq("structured binding part 1", a, 1);
        test_eq("structured binding part 2", b, 2);
        test_eq("structured binding part 3", c, 3);

        return end();
    }

    const char* array_test::name()
    {
        return "array";
    }
}
