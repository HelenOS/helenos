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
#include <initializer_list>
#include <utility>
#include <vector>

namespace std::test
{
    bool vector_test::run(bool report)
    {
        report_ = report;
        start();

        test_construction_and_assignment();
        test_insert();
        test_erase();

        return end();
    }

    const char* vector_test::name()
    {
        return "vector";
    }

    void vector_test::test_construction_and_assignment()
    {
        auto check1 = {1, 2, 3, 4};
        auto check2 = {4, 3, 2, 1};
        auto check3 = {5, 5, 5, 5};

        std::vector<int> vec1{};
        vec1.push_back(1);
        vec1.push_back(2);
        vec1.push_back(3);
        vec1.push_back(4);
        test_eq(
            "default constructor + push_back",
            vec1.begin(), vec1.end(),
            check1.begin(), check1.end()
        );

        std::vector<int> vec2{4, 3, 2, 1};
        test_eq(
            "initializer list constructor",
            vec2.begin(), vec2.end(),
            check2.begin(), check2.end()
        );

        std::vector<int> vec3(11);
        test_eq("capacity constructor", 11ul, vec3.capacity());

        std::vector<int> vec4(4ul, 5);
        test_eq(
            "replication constructor",
            vec4.begin(), vec4.end(),
            check3.begin(), check3.end()
        );

        // TODO: iterator constructor when implemented

        std::vector<int> vec6{vec4};
        test_eq(
            "copy constructor",
            vec6.begin(), vec6.end(),
            vec4.begin(), vec4.end()
        );

        std::vector<int> vec7{std::move(vec6)};
        test_eq(
            "move constructor equality",
            vec7.begin(), vec7.end(),
            vec4.begin(), vec4.end()
        );
        test_eq(
            "move constructor source empty",
            vec6.size(), 0ul
        );

        std::vector<int> vec8{check1};
        test_eq(
            "explicit initializer list constructor",
            vec8.begin(), vec8.end(),
            check1.begin(), check1.end()
        );

        std::vector<int> vec9{};
        vec9 = vec8;
        test_eq(
            "copy assignment",
            vec9.begin(), vec9.end(),
            vec8.begin(), vec8.end()
        );

        // TODO: move assignment when implemented
        std::vector<int> vec10{};
        vec10 = std::move(vec9);
        test_eq(
            "move assignment",
            vec10.begin(), vec10.end(),
            vec8.begin(), vec8.end()
        );

        test_eq("move assignment origin empty", vec9.size(), 0U);
    }

    void vector_test::test_insert()
    {
        auto check1 = {1, 2, 3, 99, 4, 5};
        auto check2 = {1, 2, 3, 99, 99, 99, 99, 99, 4, 5};
        auto check3 = {1, 2, 3, 1, 2, 3, 99, 4, 5, 4, 5};

        std::vector<int> vec1{1, 2, 3, 4, 5};
        auto it = vec1.insert(vec1.begin() + 3, 99);
        test_eq(
            "single element insert",
            vec1.begin(), vec1.end(),
            check1.begin(), check1.end()
        );
        test_eq(
            "iterator returned from insert",
            *it, 99
        );

        std::vector<int> vec2{1, 2, 3, 4, 5};
        vec2.insert(vec2.begin() + 3, 5ul, 99);
        test_eq(
            "multiple element insert",
            vec2.begin(), vec2.end(),
            check2.begin(), check2.end()
        );

        std::vector<int> vec3{1, 2, 3, 4, 5};
        vec3.insert(vec3.begin() + 3, vec2.begin() + 3, vec2.begin() + 8);
        test_eq(
            "iterator insert",
            vec3.begin(), vec3.end(),
            check2.begin(), check2.end()
        );

        std::vector<int> vec4{1, 2, 3, 4, 5};
        vec4.insert(vec4.begin() + 3, check1);
        test_eq(
            "initializer list insert",
            vec4.begin(), vec4.end(),
            check3.begin(), check3.end()
        );

        std::vector<int> vec5{1, 2, 3, 4, 5};
        vec5.insert(vec5.begin() + 3, {1, 2, 3, 99, 4, 5});
        test_eq(
            "implicit initializer list insert",
            vec5.begin(), vec5.end(),
            check3.begin(), check3.end()
        );

        std::vector<int> vec6{};
        vec6.insert(vec6.begin(), check3);
        test_eq(
            "insert to empty vector",
            vec6.begin(), vec6.end(),
            check3.begin(), check3.end()
        );
    }

    void vector_test::test_erase()
    {
        auto check1 = {1, 2, 3, 5};
        auto check2 = {1, 5};
        auto check3 = {1, 3, 5};

        std::vector<int> vec1{1, 2, 3, 4, 5};
        vec1.erase(vec1.begin() + 3);
        test_eq(
            "single element erase",
            vec1.begin(), vec1.end(),
            check1.begin(), check1.end()
        );

        std::vector<int> vec2{1, 2, 3, 4, 5};
        vec2.erase(vec2.begin() + 1, vec2.begin() + 4);
        test_eq(
            "range erase",
            vec2.begin(), vec2.end(),
            check2.begin(), check2.end()
        );

        std::vector<int> vec3{1, 2, 3, 4, 5};
        for (auto it = vec3.begin(); it != vec3.end();)
        {
            if (*it % 2 == 0)
                it = vec3.erase(it);
            else
                ++it;
        }
        test_eq(
            "erase all even numbers",
            vec3.begin(), vec3.end(),
            check3.begin(), check3.end()
        );
    }
}
