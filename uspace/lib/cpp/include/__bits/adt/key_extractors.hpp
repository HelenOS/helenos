/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ADT_KEY_EXTRACTORS
#define LIBCPP_BITS_ADT_KEY_EXTRACTORS

#include <utility>

namespace std::aux
{
    template<class Key, class Value>
    struct key_value_key_extractor
    {
        const Key& operator()(const pair<const Key, Value>& p) const noexcept
        {
            return p.first;
        }
    };

    template<class Key>
    struct key_no_value_key_extractor
    {
        Key& operator()(Key& k) const noexcept
        {
            return k;
        }

        const Key& operator()(const Key& k) const noexcept
        {
            return k;
        }
    };
}

#endif
