/*
 * Copyright (c) 2017 Jaroslav Jindrak
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

#include <string>
#include <initializer_list>
#include <internal/test/tests.hpp>
#include <cstdio>

namespace std::test
{
    bool string_test::run()
    {
        test_construction_and_assignment();
        test_append();

        return true;
    }

    const char* string_test::name()
    {
        return "string";
    }

    void string_test::test_construction_and_assignment()
    {
        const char* check1 = "hello";

        std::string str1{"hello"};
        test_eq(
            "size of string",
            str1.size(), 6ul
        );
        test_eq(
            "initialization from a cstring literal",
            str1.begin(), str1.end(),
            check1, check1 + 6
        );

        std::string str2{str1};
        test_eq(
            "copy constructor",
            str1.begin(), str1.end(),
            str2.begin(), str2.end()
        );

        std::string str3{std::move(str1)};
        test_eq(
            "move constructor equality",
            str2.begin(), str2.end(),
            str3.begin(), str3.end()
        );
        test_eq(
            "move constructor source empty",
            str1.size(), 0ul
        );

        std::string str4{};
        test_eq(
            "default constructor empty",
            str4.size(), 0ul
        );

        str4.assign(str3, 2ul, 2ul);
        test_eq(
            "assign substring to an empty string",
            str4.begin(), str4.end(),
            str3.begin() + 2, str3.begin() + 4
        );

        std::string str5{str3.begin() + 2, str3.begin() + 4};
        test_eq(
            "constructor from a pair of iterators",
            str5.begin(), str5.end(),
            str3.begin() + 2, str3.begin() + 4
        );
    }

    void string_test::test_append()
    {
        std::string check{"hello, world"};

        std::string str1{"hello, "};
        str1.append("world");
        test_eq(
            "append cstring",
            str1.begin(), str1.end(),
            check.begin(), check.end()
        );

        std::string str2{"hello, "};
        str2.append(std::string{"world"});
        test_eq(
            "append rvalue string",
            str2.begin(), str2.end(),
            check.begin(), check.end()
        );

        std::string str3{"hello, "};
        std::string apendee{"world"};
        str3.append(apendee);
        test_eq(
            "append lvalue string",
            str3.begin(), str3.end(),
            check.begin(), check.end()
        );

        std::string str4{"hello, "};
        str4.append(apendee.begin(), apendee.end());
        test_eq(
            "append iterator range",
            str4.begin(), str4.end(),
            check.begin(), check.end()
        );

        std::string str5{"hello, "};
        str5.append({'w', 'o', 'r', 'l', 'd', '\0'});
        test_eq(
            "append initializer list",
            str5.begin(), str5.end(),
            check.begin(), check.end()
        );

        std::string str6{"hello, "};
        str6 += "world";
        test_eq(
            "append using +=",
            str6.begin(), str6.end(),
            check.begin(), check.end()
        );
    }
}
