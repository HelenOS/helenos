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
