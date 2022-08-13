/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_TYPE_TRAITS_REFERENCES
#define LIBCPP_BITS_TYPE_TRAITS_REFERENCES

#include <__bits/aux.hpp>

namespace std
{
    /**
     * 20.10.7.2, reference modifications:
     */

    template<class T>
    struct remove_reference: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_reference<T&>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_reference<T&&>: aux::type_is<T>
    { /* DUMMY BODY */ };

    // TODO: is this good?
    template<class T>
    struct add_lvalue_reference: aux::type_is<T&>
    { /* DUMMY BODY */ };

    // TODO: Special case when T is not referencable!
    template<class T>
    struct add_rvalue_reference: aux::type_is<T&&>
    { /* DUMMY BODY */ };

    template<class T>
    struct add_rvalue_reference<T&>: aux::type_is<T&>
    { /* DUMMY BODY */ };

    template<class T>
    using remove_reference_t = typename remove_reference<T>::type;

    template<class T>
    using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

    template<class T>
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;
}

#endif
