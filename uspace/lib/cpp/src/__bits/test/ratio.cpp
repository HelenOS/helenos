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
