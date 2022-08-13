/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <typeinfo>

namespace std
{

    type_info::~type_info()
    { /* DUMMY BODY */ }

    bool type_info::operator==(const type_info& other) const noexcept
    {
        return (this == &other) || ::strcmp(name(), other.name()) == 0;
    }

    bool type_info::operator!=(const type_info& other) const noexcept
    {
        return !(*this == other);
    }

    bool type_info::before(const type_info& other) const noexcept
    {
        /**
         * cppreference.com:
         *  Returns true if the type of this type_info precedes the type
         *  of rhs in the implementation's collation order. No guarantees
         *  are given; in particular, the collation order can change
         *  between the invocations of the same program.
         *
         * Seems completely arbitrary and the only guarantee
         * we have to provide that two comparisons of two same
         * type_info instances return the same result.
         */
        return name() < other.name();
    }

    size_t type_info::hash_code() const noexcept
    {
        /**
         * We only need for the hash codes of two type_info
         * instances describing the same type to match, nothing
         * else.
         */
        size_t res{};
        const char* str = name();
        while(*str)
            res += static_cast<size_t>(*str++);

        return res;
    }

    const char* type_info::name() const noexcept
    {
        return __name;
    }
}
