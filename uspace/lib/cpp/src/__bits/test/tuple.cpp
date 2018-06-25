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
#include <functional>
#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace std::test
{
    bool tuple_test::run(bool report)
    {
        report_ = report;
        start();

        test_constructors_and_assignment();
        test_creation();
        test_tie_and_structured_bindings();
        test_tuple_ops();

        return end();
    }

    const char* tuple_test::name()
    {
        return "tuple";
    }

    void tuple_test::test_constructors_and_assignment()
    {
        std::tuple<int, float> tpl1{1, .5f};
        test_eq("value initialization pt1", std::get<0>(tpl1), 1);
        test_eq("value initialization pt2", std::get<1>(tpl1), .5f);

        auto p = std::make_pair(2, 1.f);
        std::tuple<int, float> tpl2{p};
        test_eq("pair initialization pt1", std::get<0>(tpl2), 2);
        test_eq("pair initialization pt2", std::get<1>(tpl2), 1.f);

        tpl1 = p;
        test_eq("pair assignment pt1", std::get<0>(tpl1), 2);
        test_eq("pair assignment pt2", std::get<1>(tpl1), 1.f);

        auto tpl3 = std::make_tuple(std::string{"A"}, std::string{"B"});
        auto tpl4 = std::make_tuple(std::string{"C"}, std::string{"D"});
        tpl3 = std::move(tpl4);
        test_eq("move assignment pt1", std::get<0>(tpl3), std::string{"C"});
        test_eq("move assignment pt2", std::get<1>(tpl3), std::string{"D"});

        auto tpl5 = std::make_tuple(1, .5f);
        auto tpl6{std::move(tpl5)};
        test_eq("move initialization pt1", std::get<0>(tpl6), 1);
        test_eq("move initialization pt2", std::get<1>(tpl6), .5f);
    }

    void tuple_test::test_creation()
    {
        auto tpl1 = std::make_tuple(1, .5f, std::string{"test"}, true);
        static_assert(std::is_same_v<std::tuple_element_t<0, decltype(tpl1)>, int>);
        static_assert(std::is_same_v<std::tuple_element_t<1, decltype(tpl1)>, float>);
        static_assert(std::is_same_v<std::tuple_element_t<2, decltype(tpl1)>, std::string>);
        static_assert(std::is_same_v<std::tuple_element_t<3, decltype(tpl1)>, bool>);

        test_eq("make_tuple pt1", std::get<0>(tpl1), 1);
        test_eq("make_tuple pt2", std::get<1>(tpl1), .5f);
        test_eq("make_tuple pt3", std::get<2>(tpl1), std::string{"test"});
        test_eq("make_tuple pt4", std::get<3>(tpl1), true);

        static_assert(std::tuple_size_v<decltype(tpl1)> == 4);

        int i{};
        float f{};
        auto tpl2 = std::make_tuple(std::ref(i), std::cref(f));
        static_assert(std::is_same_v<std::tuple_element_t<0, decltype(tpl2)>, int&>);
        static_assert(std::is_same_v<std::tuple_element_t<1, decltype(tpl2)>, const float&>);

        std::get<0>(tpl2) = 3;
        test_eq("modify reference in tuple", i, 3);

        auto tpl3 = std::forward_as_tuple(i, f);
        static_assert(std::is_same_v<std::tuple_element_t<0, decltype(tpl3)>, int&>);
        static_assert(std::is_same_v<std::tuple_element_t<1, decltype(tpl3)>, float&>);

        std::get<1>(tpl3) = 1.5f;
        test_eq("modify reference in forward_as_tuple", f, 1.5f);
    }

    void tuple_test::test_tie_and_structured_bindings()
    {
        int i1{};
        float f1{};
        auto tpl = std::make_tuple(1, .5f);
        std::tie(i1, f1) = tpl;

        test_eq("tie unpack pt1", i1, 1);
        test_eq("tie unpack pt2", f1, .5f);

        std::get<0>(tpl) = 2;
        /* std::tie(i1, std::ignore) = tpl; */

        /* test_eq("tie unpack with ignore", i1, 2); */

        auto [i2, f2] = tpl;
        test_eq("structured bindings pt1", i2, 2);
        test_eq("structured bindings pt2", f2, .5f);
    }

    void tuple_test::test_tuple_ops()
    {
        auto tpl1 = std::make_tuple(1, .5f);
        auto tpl2 = std::make_tuple(1, .5f);
        auto tpl3 = std::make_tuple(1, 1.f);
        auto tpl4 = std::make_tuple(2, .5f);
        auto tpl5 = std::make_tuple(2, 1.f);

        test_eq("tuple == pt1", (tpl1 == tpl2), true);
        test_eq("tuple == pt2", (tpl1 == tpl3), false);
        test_eq("tuple == pt3", (tpl1 == tpl4), false);
        test_eq("tuple < pt1", (tpl1 < tpl2), false);
        test_eq("tuple < pt2", (tpl1 < tpl3), true);
        test_eq("tuple < pt3", (tpl1 < tpl4), true);
        test_eq("tuple < pt4", (tpl1 < tpl5), true);

        tpl1.swap(tpl5);
        test_eq("tuple swap pt1", std::get<0>(tpl1), 2);
        test_eq("tuple swap pt2", std::get<1>(tpl1), 1.f);
        test_eq("tuple swap pt3", std::get<0>(tpl5), 1);
        test_eq("tuple swap pt4", std::get<1>(tpl5), .5f);
    }
}
