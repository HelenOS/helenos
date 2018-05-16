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
