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

#ifndef LIBCPP_BITS_TEST_MOCK
#define LIBCPP_BITS_TEST_MOCK

#include <cstdlib>
#include <tuple>

namespace std::test
{
    /**
     * Auxiliary type that lets us to monitor the number
     * of calls to various functions for the purposes
     * of testing smart pointers and other features of
     * the library.
     */
    struct mock
    {
        static size_t constructor_calls;
        static size_t copy_constructor_calls;
        static size_t destructor_calls;
        static size_t move_constructor_calls;

        mock()
        {
            ++constructor_calls;
        }

        mock(const mock&)
        {
            ++copy_constructor_calls;
        }

        mock(mock&&)
        {
            ++move_constructor_calls;
        }

        ~mock()
        {
            ++destructor_calls;
        }

        static void clear()
        {
            constructor_calls = size_t{};
            copy_constructor_calls = size_t{};
            destructor_calls = size_t{};
            move_constructor_calls = size_t{};
        }
    };
}

#endif
