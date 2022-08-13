/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
