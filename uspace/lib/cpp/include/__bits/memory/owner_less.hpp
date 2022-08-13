/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_MEMORY_OWNER_LESS
#define LIBCPP_BITS_MEMORY_OWNER_LESS

#include <__bits/memory/shared_ptr.hpp>
#include <__bits/memory/weak_ptr.hpp>

namespace std
{
    /**
     * 20.8.2.4, class template owner_less:
     */

    template<class>
    struct owner_less;

    template<class T>
    struct owner_less<shared_ptr<T>>
    {
        using retult_type          = bool;
        using first_argument_type  = shared_ptr<T>;
        using second_argument_type = shared_ptr<T>;

        bool operator()(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) const
        {
            return lhs.owner_before(rhs);
        }

        bool operator()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const
        {
            return lhs.owner_before(rhs);
        }

        bool operator()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const
        {
            return lhs.owner_before(rhs);
        }
    };

    template<class T>
    struct owner_less<weak_ptr<T>>
    {
        using retult_type          = bool;
        using first_argument_type  = weak_ptr<T>;
        using second_argument_type = weak_ptr<T>;

        bool operator()(const weak_ptr<T>& lhs, const weak_ptr<T>& rhs) const
        {
            return lhs.owner_before(rhs);
        }

        bool operator()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const
        {
            return lhs.owner_before(rhs);
        }

        bool operator()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const
        {
            return lhs.owner_before(rhs);
        }
    };
}

#endif
