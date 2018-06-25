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

#ifndef LIBCPP_BITS_MEMORY_TYPE_GETTERS
#define LIBCPP_BITS_MEMORY_TYPE_GETTERS

/**
 * Allocator and pointer traits require us to provide
 * conditional typedefs based on whether the wrapped
 * allocator contains them or not. To avoid unnecessary
 * code clutter in <memory>, we moved them to their
 * separate header (though note that these metafunctions
 * have no use outside of <memory>).
 */

#include <__bits/aux.hpp>
#include <cstddef>
#include <type_traits>

namespace std
{
    template<class T>
    struct pointer_traits;
}

namespace std::aux
{
    /**
     * Pointer traits:
     */

    template<class Ptr, class = void>
    struct ptr_get_element_type
    { /* DUMMY BODY */ };

    template<class Ptr>
    struct ptr_get_element_type<Ptr, void_t<typename Ptr::element_type>>
        : aux::type_is<typename Ptr::element_type>
    { /* DUMMY BODY */ };

    template<template <class, class...> class Ptr, class T, class... Args>
    struct ptr_get_element_type<
        Ptr<T, Args...>, void_t<typename Ptr<T, Args...>::element_type>
    >: aux::type_is<typename Ptr<T, Args...>::element_type>
    { /* DUMMY BODY */ };

    template<class Ptr, class = void>
    struct ptr_get_difference_type: aux::type_is<ptrdiff_t>
    { /* DUMMY BODY */ };

    template<class Ptr>
    struct ptr_get_difference_type<Ptr, void_t<typename Ptr::difference_type>>
        : aux::type_is<typename Ptr::difference_type>
    { /* DUMMY BODY */ };

    template<class Ptr, class U, class = void>
    struct ptr_get_rebind
    { /* DUMMY BODY */ };

    template<class Ptr, class U>
    struct ptr_get_rebind<Ptr, U, void_t<typename Ptr::template rebind<U>>>
        : aux::type_is<typename Ptr::template rebind<U>>
    { /* DUMMY BODY */ };

    template<template <class, class...> class Ptr, class T, class... Args, class U>
    struct ptr_get_rebind<Ptr<T, Args...>, U>
        : aux::type_is<Ptr<U, Args...>>
    { /* DUMMY BODY */ };

    /**
     * Allocator traits:
     */

    template<class T, class = void>
    struct alloc_get_pointer: aux::type_is<typename T::value_type*>
    { /* DUMMY BODY */ };

    template<class T>
    struct alloc_get_pointer<T, void_t<typename T::pointer>>
        : aux::type_is<typename T::pointer>
    { /* DUMMY BODY */ };

    template<class T, class Ptr, class = void>
    struct alloc_get_const_pointer
        : aux::type_is<typename pointer_traits<Ptr>::template rebind<const typename T::value_type>>
    { /* DUMMY BODY */ };

    template<class T, class Ptr>
    struct alloc_get_const_pointer<T, Ptr, void_t<typename T::const_pointer>>
        : aux::type_is<typename T::const_pointer>
    { /* DUMMY BODY */ };

    template<class T, class Ptr, class = void>
    struct alloc_get_void_pointer
        : aux::type_is<typename pointer_traits<Ptr>::template rebind<void>>
    { /* DUMMY BODY */ };

    template<class T, class Ptr>
    struct alloc_get_void_pointer<T, Ptr, void_t<typename T::void_pointer>>
        : aux::type_is<typename T::void_pointer>
    { /* DUMMY BODY */ };

    template<class T, class Ptr, class = void>
    struct alloc_get_const_void_pointer
        : aux::type_is<typename pointer_traits<Ptr>::template rebind<const void>>
    { /* DUMMY BODY */ };

    template<class T, class Ptr>
    struct alloc_get_const_void_pointer<T, Ptr, void_t<typename T::const_void_pointer>>
        : aux::type_is<typename T::const_void_pointer>
    { /* DUMMY BODY */ };

    template<class T, class Ptr, class = void>
    struct alloc_get_difference_type
        : aux::type_is<typename pointer_traits<Ptr>::difference_type>
    { /* DUMMY BODY */ };

    template<class T, class Ptr>
    struct alloc_get_difference_type<T, Ptr, void_t<typename T::difference_type>>
        : aux::type_is<typename T::difference_type>
    { /* DUMMY BODY */ };

    template<class T, class Difference, class = void>
    struct alloc_get_size_type: aux::type_is<make_unsigned_t<Difference>>
    { /* DUMMY BODY */ };

    template<class T, class Difference>
    struct alloc_get_size_type<T, Difference, void_t<typename T::size_type>>
        : aux::type_is<typename T::size_type>
    { /* DUMMY BODY */ };

    template<class T, class = void>
    struct alloc_get_copy_propagate: aux::type_is<false_type>
    { /* DUMMY BODY */ };

    template<class T>
    struct alloc_get_copy_propagate<T, void_t<typename T::propagate_on_container_copy_assignment>>
        : aux::type_is<typename T::propagate_on_container_copy_assignment>
    { /* DUMMY BODY */ };

    template<class T, class = void>
    struct alloc_get_move_propagate: aux::type_is<false_type>
    { /* DUMMY BODY */ };

    template<class T>
    struct alloc_get_move_propagate<T, void_t<typename T::propagate_on_container_move_assignment>>
        : aux::type_is<typename T::propagate_on_container_move_assignment>
    { /* DUMMY BODY */ };

    template<class T, class = void>
    struct alloc_get_swap_propagate: aux::type_is<false_type>
    { /* DUMMY BODY */ };

    template<class T>
    struct alloc_get_swap_propagate<T, void_t<typename T::propagate_on_container_swap>>
        : aux::type_is<typename T::propagate_on_container_swap>
    { /* DUMMY BODY */ };

    template<class T, class = void>
    struct alloc_get_always_equal: aux::type_is<typename is_empty<T>::type>
    { /* DUMMY BODY */ };

    template<class T>
    struct alloc_get_always_equal<T, void_t<typename T::is_always_equal>>
        : aux::type_is<typename T::is_always_equal>
    { /* DUMMY BODY */ };

    template<class Alloc, class T, class = void>
    struct alloc_get_rebind_alloc
    { /* DUMMY BODY */ };

    template<class Alloc, class T>
    struct alloc_get_rebind_alloc<Alloc, T, void_t<typename Alloc::template rebind<T>::other>>
        : aux::type_is<typename Alloc::template rebind<T>::other>
    { /* DUMMY BODY */ };

    template<template <class, class...> class Alloc, class U, class... Args, class T>
    struct alloc_get_rebind_alloc<Alloc<U, Args...>, T>
        : aux::type_is<Alloc<T, Args...>>
    { /* DUMMY BODY */ };

    /**
     * These metafunctions are used to check whether an expression
     * is well-formed for the static functions of allocator_traits:
     */

    template<class Alloc, class Size, class ConstVoidPointer, class = void>
    struct alloc_has_hint_allocate: false_type
    { /* DUMMY BODY */ };

    template<class Alloc, class Size, class ConstVoidPointer>
    struct alloc_has_hint_allocate<
        Alloc, Size, ConstVoidPointer, void_t<
            decltype(declval<Alloc>().alloc(declval<Size>(), declval<ConstVoidPointer>()))
        >
    >: true_type
    { /* DUMMY BODY */ };

    template<class, class Alloc, class T, class... Args>
    struct alloc_has_construct_impl: false_type
    { /* DUMMY BODY */ };

    template<class Alloc, class T, class... Args>
    struct alloc_has_construct_impl<
        void_t<decltype(declval<Alloc>().construct(declval<T*>(), forward<Args>(declval<Args>())...))>,
        Alloc, T, Args...
    >: true_type
    { /* DUMMY BODY */ };

    template<class Alloc, class T, class = void>
    struct alloc_has_destroy: false_type
    { /* DUMMY BODY */ };

    template<class Alloc, class T>
    struct alloc_has_destroy<Alloc, T, void_t<decltype(declval<Alloc>().destroy(declval<T>()))>>
        : true_type
    { /* DUMMY BODY */ };

    template<class Alloc, class T, class... Args>
    struct alloc_has_construct
        : alloc_has_construct_impl<void_t<>, Alloc, T, Args...>
    { /* DUMMY BODY */ };

    template<class Alloc, class = void>
    struct alloc_has_max_size: false_type
    { /* DUMMY BODY */ };

    template<class Alloc>
    struct alloc_has_max_size<Alloc, void_t<decltype(declval<Alloc>().max_size())>>
        : true_type
    { /* DUMMY BODY */ };

    template<class Alloc, class = void>
    struct alloc_has_select: false_type
    { /* DUMMY BODY */ };

    template<class Alloc>
    struct alloc_has_select<
        Alloc, void_t<
            decltype(declval<Alloc>().select_on_container_copy_construction())
        >
    >: true_type
    { /* DUMMY BODY */ };
}

#endif
