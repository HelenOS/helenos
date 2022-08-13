/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ITERATOR_HELPERS
#define LIBCPP_BITS_ITERATOR_HELPERS

#include <__bits/aux.hpp>

namespace std::aux
{
    /**
     * Used for our custom iterators where we know
     * that their references/const_references and
     * pointers/const_pointers differ only in constness.
     */

    template<class T>
    struct get_non_const_ref
        : type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct get_non_const_ref<const T&>
        : type_is<T&>
    { /* DUMMY BODY */ };

    template<class T>
    using get_non_const_ref_t = typename get_non_const_ref<T>::type;

    template<class T>
    struct get_non_const_ptr
        : type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct get_non_const_ptr<const T*>
        : type_is<T*>
    { /* DUMMY BODY */ };

    template<class T>
    using get_non_const_ptr_t = typename get_non_const_ptr<T>::type;
}

#endif
