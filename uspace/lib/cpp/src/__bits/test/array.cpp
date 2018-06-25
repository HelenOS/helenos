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
