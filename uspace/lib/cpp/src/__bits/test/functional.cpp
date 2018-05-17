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
#include <type_traits>
#include <utility>

using namespace std::placeholders;

namespace std::test
{
    namespace aux
    {
        int f1(int a, int b)
        {
            return a + b;
        }

        int f2(int a, int b)
        {
            return a * 10 + b;
        }

        void f3(int& a, int& b)
        {
            a = 42;
            b = 1337;
        }

        struct Foo
        {
            int add(int a)
            {
                return a + data;
            }

            int data;
        };
    }

    bool functional_test::run(bool report)
    {
        report_ = report;
        start();

        test_reference_wrapper();
        test_function();
        test_bind();

        return end();
    }

    const char* functional_test::name()
    {
        return "functional";
    }

    void functional_test::test_reference_wrapper()
    {
        int x{4};
        auto ref = std::ref(x);

        test("reference_wrapper equivalence after construction (cast)", (ref == x));
        test("reference_wrapper equivalence after construction (get)", (ref.get() == x));

        ref.get() = 5;
        test_eq("reference_wrapper equivalence after modification pt1", ref.get(), 5);
        test_eq("reference_wrapper equivalence after modification pt2", x, 5);

        int y{10};
        ref = y;
        test_eq("reference_wrapper equivalence after assignment pt1", ref.get(), 10);
        test_eq("reference_wrapper equivalence after assignment pt2", x, 5);

        std::function<int(int, int)> wrapped_f1{&aux::f1};
        auto fref = std::ref(wrapped_f1);
        auto res = fref(2, 5);
        test_eq("reference_wrapper function invoke", res, 7);
    }

    void functional_test::test_function()
    {
        std::function<int(int, int)> f1{&aux::f1};
        auto res1 = f1(1, 2);

        test_eq("function from function pointer", res1, 3);

        int x{};
        std::function<char(char)> f2{[&](auto c){ x = 42; return ++c; }};
        auto res2 = f2('B');

        test_eq("function from lambda", res2, 'C');
        test_eq("function from lambda - capture", x, 42);

        test("function operator bool", (bool)f2);
        f2 = nullptr;
        test("function nullptr assignment", !f2);
    }

    void functional_test::test_bind()
    {
        auto f1 = std::bind(aux::f1, _1, 1);
        auto res1 = f1(3);

        test_eq("bind placeholder", res1, 4);

        auto f2 = std::bind(aux::f2, _2, _1);
        auto res2 = f2(5, 6);

        test_eq("bind reverse placeholder order", res2, 65);

        int x{};
        int y{};
        auto f3 = std::bind(aux::f3, _1, std::ref(y));
        f3(std::ref(x));

        test_eq("bind std::ref as bound", y, 1337);
        test_eq("bind std::ref as unbound", x, 42);

        auto f4 = std::bind(aux::f2, x, y);
        auto res3 = f4();

        test_eq("bind all arguments bound", res3, 1757);

        aux::Foo foo{5};
        auto f5 = std::mem_fn(&aux::Foo::add);
        auto res4 = f5(&foo, 4);

        test_eq("mem_fn", res4, 9);

        /* auto f5 = std::bind(&aux::Foo::add, _1, _2); */
        /* auto res4 = f4(&foo, 10); */

        /* test_eq("bind to member function", res4, 19); */
    }
}
