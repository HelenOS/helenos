/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <__bits/test/mock.hpp>

namespace std::test
{
    size_t mock::constructor_calls{};
    size_t mock::copy_constructor_calls{};
    size_t mock::destructor_calls{};
    size_t mock::move_constructor_calls{};
}
