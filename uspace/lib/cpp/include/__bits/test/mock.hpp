/*
 * SPDX-FileCopyrightText: 2017 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
