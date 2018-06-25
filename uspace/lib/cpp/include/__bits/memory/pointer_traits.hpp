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

#ifndef LIBCPP_BITS_MEMORY_POINTER_TRAITS
#define LIBCPP_BITS_MEMORY_POINTER_TRAITS

#include <__bits/memory/addressof.hpp>
#include <__bits/memory/type_getters.hpp>
#include <cstddef>
#include <type_traits>

namespace std
{
    /**
     * 20.7.3, pointer traits:
     */

    template<class Ptr>
    struct pointer_traits
    {
        using pointer         = Ptr;
        using element_type    = typename aux::ptr_get_element_type<Ptr>::type;
        using difference_type = typename aux::ptr_get_difference_type<Ptr>::type;

        template<class U>
        using rebind = typename aux::ptr_get_rebind<Ptr, U>::type;

        static pointer pointer_to( // If is_void_t<element_type>, this type is unspecified.
            conditional_t<is_void_v<element_type>, char, element_type&> x
        )
        {
            return Ptr::pointer_to(x);
        }
    };

    template<class T>
    struct pointer_traits<T*>
    {
        using pointer         = T*;
        using element_type    = T;
        using difference_type = ptrdiff_t;

        template<class U>
        using rebind = U*;

        static pointer pointer_to(
            conditional_t<is_void_v<element_type>, char, element_type&> x
        )
        {
            return std::addressof(x);
        }
    };

    /**
     * 20.7.4, pointer safety:
     */

    // TODO: implement

    /**
     * 20.7.5, align:
     */

    // TODO: implement
}

#endif
