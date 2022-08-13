/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <__bits/test/tests.hpp>
#include <initializer_list>
#include <ratio>
#include <utility>

namespace std::test
{
    bool ratio_test::run(bool report)
    {
        report_ = report;
        start();

        test_eq(
            "ratio_add pt1",
            std::ratio_add<std::ratio<2, 3>, std::ratio<1, 6>>::num,
            5
        );
        test_eq(
            "ratio_add pt2",
            std::ratio_add<std::ratio<2, 3>, std::ratio<1, 6>>::den,
            6
        );

        test_eq(
            "ratio_subtract pt1",
            std::ratio_subtract<std::ratio<2, 3>, std::ratio<1, 6>>::num,
            1
        );
        test_eq(
            "ratio_subtract pt2",
            std::ratio_subtract<std::ratio<2, 3>, std::ratio<1, 6>>::den,
            2
        );

        test_eq(
            "ratio_multiply pt1",
            std::ratio_multiply<std::ratio<2, 3>, std::ratio<1, 6>>::num,
            1
        );
        test_eq(
            "ratio_multiply pt2",
            std::ratio_multiply<std::ratio<2, 3>, std::ratio<1, 6>>::den,
            9
        );

        test_eq(
            "ratio_divide pt1",
            std::ratio_divide<std::ratio<2, 3>, std::ratio<1, 6>>::num,
            4
        );
        test_eq(
            "ratio_divide pt2",
            std::ratio_divide<std::ratio<2, 3>, std::ratio<1, 6>>::den,
            1
        );

        test_eq(
            "ratio_equal", std::ratio_equal_v<std::ratio<2, 3>, std::ratio<6, 9>>, true
        );

        test_eq(
            "ratio_not_equal", std::ratio_not_equal_v<std::ratio<2, 3>, std::ratio<5, 9>>, true
        );

        test_eq(
            "ratio_less", std::ratio_less_v<std::ratio<2, 3>, std::ratio<5, 6>>, true
        );

        test_eq(
            "ratio_less_equal pt1", std::ratio_less_equal_v<std::ratio<2, 3>, std::ratio<5, 6>>, true
        );

        test_eq(
            "ratio_less_equal pt2", std::ratio_less_equal_v<std::ratio<2, 3>, std::ratio<2, 3>>, true
        );

        test_eq(
            "ratio_greater", std::ratio_greater_v<std::ratio<2, 3>, std::ratio<2, 6>>, true
        );

        test_eq(
            "ratio_greater_equal pt1", std::ratio_greater_equal_v<std::ratio<2, 3>, std::ratio<2, 6>>, true
        );

        test_eq(
            "ratio_greater_equal pt2", std::ratio_greater_equal_v<std::ratio<2, 3>, std::ratio<2, 3>>, true
        );

        return end();
    }

    const char* ratio_test::name()
    {
        return "ratio";
    }
}
