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
#include <bitset>
#include <initializer_list>
#include <sstream>
#include <string>

namespace std::test
{
    bool bitset_test::run(bool report)
    {
        report_ = report;
        start();

        test_constructors_and_assignment();
        test_strings();
        test_operations();

        return end();
    }

    const char* bitset_test::name()
    {
        return "bitset";
    }

    void bitset_test::test_constructors_and_assignment()
    {
        // 00101010bin == 42dec (note that the bits are reversed in bitset)
        auto check1 = {false, true, false, true, false, true, false, false};
        std::bitset<8> b1{42};
        test_eq(
            "from number to number equivalence",
            b1.to_ulong(), 42UL
        );

        auto it = check1.begin();
        size_t i{};
        for (; i < 8U; ++i)
        {
            if (*it++ != b1[i])
                break;
        }
        test_eq("from number iterating over bits", i, 8U);

        std::bitset<8> b2{"00101010"};
        test_eq(
            "from string to number equivalence",
            b2.to_ulong(), 42UL
        );

        it = check1.begin();
        i = size_t{};
        for (; i < 8U; ++i)
        {
            if (*it++ != b2[i])
                break;
        }
        test_eq("from string iterating over bits", i, 8U);

        std::bitset<16> b3{0b1111'1101'1011'1010};
        test_eq(
            "from binary to number equivalence",
            b3.to_ulong(), 0b1111'1101'1011'1010UL
        );

        std::bitset<64> b4{0xABCD'DCBA'DEAD'BEEFULL};
        test_eq(
            "from hex to number equivalence",
            b4.to_ulong(), static_cast<unsigned long>(0xABCD'DCBA'DEAD'BEEFULL)
        );

        std::bitset<8> b5{"XXYXYXYX", 8U, 'X', 'Y'};
        test_eq(
            "from string with different 0/1 values equivalence",
            b5.to_ulong(), 42U
        );

        std::bitset<8> b6{"XXYXYXYXXXX IGNORED", 8U, 'X', 'Y'};
        test_eq(
            "from prefix string with different 0/1 values equivalence",
            b6.to_ulong(), 42U
        );

        std::bitset<8> b7{std::string{"XXXXYXYXYX"}, 2U, 8U, 'X', 'Y'};
        test_eq(
            "from substring with different 0/1 values equivalence",
            b7.to_ulong(), 42U
        );
    }

    void bitset_test::test_strings()
    {
        std::bitset<8> b1{42};
        auto s1 = b1.to_string();
        test_eq(
            "to string",
            s1, "00101010"
        );

        auto s2 = b1.to_string('X', 'Y');
        test_eq(
            "to string string with different 0/1 values",
            s2, "XXYXYXYX"
        );

        std::istringstream iss{"00101010"};
        std::bitset<8> b2{};
        iss >> b2;
        test_eq(
            "istream operator>>",
            b2.to_ulong(), 42UL
        );

        std::ostringstream oss{};
        oss << b2;
        std::string s3{oss.str()};
        std::string s4{"00101010"};
        test_eq(
            "ostream operator<<",
            s3, s4
        );
    }

    void bitset_test::test_operations()
    {
        std::bitset<8> b1{};

        b1.set(3);
        test_eq("set", b1[3], true);

        b1.reset(3);
        test_eq("reset", b1[3], false);

        b1.flip(3);
        test_eq("flip", b1[3], true);

        b1 >>= 1;
        test_eq("lshift new", b1[2], true);
        test_eq("lshift old", b1[3], false);

        b1 <<= 1;
        test_eq("rshift new", b1[2], false);
        test_eq("rshift old", b1[3], true);

        test_eq("any1", b1.any(), true);
        test_eq("none1", b1.none(), false);
        test_eq("all1", b1.all(), false);

        /* b1 <<= 7; */
        /* test_eq("any2", b1.any(), false); */
        /* test_eq("none2", b1.none(), true); */
        /* test_eq("all2", b1.all(), false); */

        b1.set();
        test_eq("set + all", b1.all(), true);

        b1.reset();
        test_eq("reset + none", b1.none(), true);

        std::bitset<8> b2{0b0101'0101};
        std::bitset<8> b3{0b1010'1010};
        b2.flip();
        test_eq("flip all", b2, b3);

        std::bitset<8> b4{0b0011'0101};
        std::bitset<8> b5{0b1010'1100};
        test_eq("and", (b4 & b5), std::bitset<8>{0b0010'0100});
        test_eq("or", (b4 | b5), std::bitset<8>{0b1011'1101});
        test_eq("count", b4.count(), 4U);
    }
}
