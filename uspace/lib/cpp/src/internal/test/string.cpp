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
        test_insert();
        test_erase();
        test_replace();
        test_copy();
        test_find();

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
            str1.size(), 5ul
        );
        test_eq(
            "initialization from a cstring literal",
            str1.begin(), str1.end(),
            check1, check1 + 5
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
        str5.append({'w', 'o', 'r', 'l', 'd'});
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

    void string_test::test_insert()
    {
        std::string check{"hello, world"};

        std::string str1{", world"};
        str1.insert(0, "hello");
        test_eq(
            "insert at the beggining",
            str1.begin(), str1.end(),
            check.begin(), check.end()
        );

        std::string str2{"hello,world"};
        str2.insert(str2.begin() + 6, ' ');
        test_eq(
            "insert char in the middle",
            str2.begin(), str2.end(),
            check.begin(), check.end()
        );

        std::string str3{"heo, world"};
        str3.insert(str3.begin() + 2, 2ul, 'l');
        test_eq(
            "insert n chars",
            str3.begin(), str3.end(),
            check.begin(), check.end()
        );

        std::string str4{"h, world"};
        std::string insertee{"ello"};
        str4.insert(str4.begin() + 1, insertee.begin(),
                    insertee.end());
        test_eq(
            "insert iterator range",
            str4.begin(), str4.end(),
            check.begin(), check.end()
        );

        std::string str5{"hel, world"};
        std::initializer_list<char> init{'l', 'o'};
        str5.insert(str5.begin() + 3, init);
        test_eq(
            "insert initializer list",
            str5.begin(), str5.end(),
            check.begin(), check.end()
        );
    }

    void string_test::test_erase()
    {
        std::string check{"hello"};

        std::string str1{"heXllo"};
        str1.erase(str1.begin() + 2);
        test_eq(
            "erase single char in the middle",
            str1.begin(), str1.end(),
            check.begin(), check.end()
        );

        std::string str2{"Xhello"};
        str2.erase(str2.begin());
        test_eq(
            "erase single char at the beginning",
            str2.begin(), str2.end(),
            check.begin(), check.end()
        );

        std::string str3{"helloX"};
        str3.erase(str3.begin() + 5);
        test_eq(
            "erase single char at the end",
            str3.begin(), str3.end(),
            check.begin(), check.end()
        );

        std::string str4{"XXXhello"};
        str4.erase(0, 3);
        test_eq(
            "erase string at the beginning",
            str4.begin(), str4.end(),
            check.begin(), check.end()
        );

        std::string str5{"heXXXllo"};
        str5.erase(2, 3);
        test_eq(
            "erase string in the middle",
            str5.begin(), str5.end(),
            check.begin(), check.end()
        );

        std::string str6{"helloXXX"};
        str6.erase(5);
        test_eq(
            "erase string at the end",
            str6.begin(), str6.end(),
            check.begin(), check.end()
        );

        std::string str7{"hellXXXo"};
        str7.erase(str7.begin() + 4, str7.begin() + 7);
        test_eq(
            "erase iterator range",
            str7.begin(), str7.end(),
            check.begin(), check.end()
        );
    }

    void string_test::test_replace()
    {
        std::string check{"hello, world"};

        std::string str1{"helXXX world"};
        str1.replace(3, 3, "lo,", 3);
        test_eq(
            "replace with full string",
            str1.begin(), str1.end(),
            check.begin(), check.end()
        );

        std::string str2{"helXXX world"};
        str2.replace(3, 3, "lo,YYY", 3);
        test_eq(
            "replace with prefix of a string",
            str2.begin(), str2.end(),
            check.begin(), check.end()
        );

        std::string str3{"helXXX world"};
        str3.replace(3, 3, "YYlo,YYY", 2, 3);
        test_eq(
            "replace with substring of a string",
            str3.begin(), str3.end(),
            check.begin(), check.end()
        );

        std::string str4{"heXXo, world"};
        str4.replace(2, 2, 2, 'l');
        test_eq(
            "replace with repeated characters",
            str4.begin(), str4.end(),
            check.begin(), check.end()
        );

        std::string str5{"heXXXXo, world"};
        str5.replace(2, 4, 2, 'l');
        test_eq(
            "replace with repeated characters (shrinking)",
            str5.begin(), str5.end(),
            check.begin(), check.end()
        );

        std::string str6{"helXXXXX world"};
        str6.replace(3, 5, "YYlo,YYY", 2, 3);
        test_eq(
            "replace with substring of a string (shrinking)",
            str6.begin(), str6.end(),
            check.begin(), check.end()
        );

        std::string str7{"helXXXXX world"};
        std::string replacer{"YYlo,YYY"};
        str7.replace(3, 5, replacer, 2, 3);
        test_eq(
            "replace with substring of a string (shrinking, std::string)",
            str7.begin(), str7.end(),
            check.begin(), check.end()
        );

        std::string str8{"helXXXXX world"};
        str8.replace(str8.begin() + 3, str8.begin() + 8, "lo,");
        test_eq(
            "replace with a string (iterators)",
            str8.begin(), str8.end(),
            check.begin(), check.end()
        );

        std::string str9{"heXXXXo, world"};
        str9.replace(str9.begin() + 2, str9.begin() + 6, 2, 'l');
        test_eq(
            "replace with repeated characters (shrinking, iterators)",
            str9.begin(), str9.end(),
            check.begin(), check.end()
        );

        std::string str10{"helXXXXX world"};
        str10.replace(str10.begin() + 3, str10.begin() + 8,
                      replacer.begin() + 2, replacer.begin() + 5);
        test_eq(
            "replace with substring of a string (shrinking, iterators x2)",
            str10.begin(), str10.end(),
            check.begin(), check.end()
        );

        std::string str11{"helXXXXX world"};
        str11.replace(str11.begin() + 3, str11.begin() + 8,
                      {'l', 'o', ','});
        test_eq(
            "replace with an initializer list (shrinking, iterators)",
            str11.begin(), str11.end(),
            check.begin(), check.end()
        );

        std::string str12{"helXXX world"};
        str12.replace(str12.begin() + 3, str12.begin() + 6,
                      {'l', 'o', ','});
        test_eq(
            "replace with an initializer list (iterators)",
            str12.begin(), str12.end(),
            check.begin(), check.end()
        );
    }

    void string_test::test_copy()
    {
        std::string check{"CCABB"};

        std::string str1{"ACCCA"};
        std::string str2{"BBBBB"};

        str1.copy(const_cast<char*>(str2.c_str()), 3, 2);
        test_eq(
            "copy",
            str2.begin(), str2.end(),
            check.begin(), check.end()
        );
    }

    void string_test::test_find()
    {
        std::string target{"ABC"};
        auto miss = std::string::npos;

        std::string str1{"xxABCxx"};

        auto idx = str1.find(target, 0);
        test_eq(
            "find from start (success)",
            idx, 2ul
        );

        idx = str1.find(target, 3);
        test_eq(
            "find from start (fail, late start)",
            idx, miss
        );

        idx = str1.rfind(target, miss);
        test_eq(
            "rfind from start (success)",
            idx, 2ul
        );

        idx = str1.rfind(target, 1);
        test_eq(
            "rfind from start (fail, late start)",
            idx, miss
        );

        std::string str2{"xxABCxxABCxx"};

        idx = str2.find(target, 0);
        test_eq(
            "find from start (success, multiple)",
            idx, 2ul
        );
    }
}
