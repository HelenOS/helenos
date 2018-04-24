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

#ifndef LIBCPP_UNORDERED_SET
#define LIBCPP_UNORDERED_SET

#include <initializer_list>
#include <internal/hash_map.hpp>
#include <functional>
#include <memory>
#include <utility>

namespace std
{
    /**
     * 23.5.6, class template unordered_set:
     */

    template<
        class Key,
        class Hash = hash<Key>,
        class Pred = equal_to<Key>,
        class Alloc = allocator<Key>
    >
    class unordered_set;

    /**
     * 23.5.7, class template unordered_multiset:
     */

    template<
        class Key
        class Hash = hash<Key>,
        class Pred = equal_to<Key>,
        class Alloc = allocator<Key>
    >
    class unordered_multiset;

    template<class Key, class Hash, class Pred, class Alloc>
    void swap(unordered_set<Key, Hash, Pred, Alloc>& lhs,
              unordered_set<Key, Hash, Pred, Alloc>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class Key, class Hash, class Pred, class Alloc>
    void swap(unordered_multiset<Key, Hash, Pred, Alloc>& lhs,
              unordered_multiset<Key, Hash, Pred, Alloc>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class Key, class Hash, class Pred, class Alloc>
    bool operator==(unordered_set<Key, Hash, Pred, Alloc>& lhs,
                    unordered_set<Key, Hash, Pred, Alloc>& rhs)
    {
        // TODO: implement
        return false;
    }

    template<class Key, class Hash, class Pred, class Alloc>
    bool operator!=(unordered_set<Key, Hash, Pred, Alloc>& lhs,
                    unordered_set<Key, Hash, Pred, Alloc>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Key, class Hash, class Pred, class Alloc>
    bool operator==(unordered_multiset<Key, Hash, Pred, Alloc>& lhs,
                    unordered_multiset<Key, Hash, Pred, Alloc>& rhs)
    {
        // TODO: implement
        return false;
    }

    template<class Key, class Value, class Hash, class Pred, class Alloc>
    bool operator!=(unordered_multiset<Key, Hash, Pred, Alloc>& lhs,
                    unordered_multiset<Key, Hash, Pred, Alloc>& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif
