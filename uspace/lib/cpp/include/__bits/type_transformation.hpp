/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_TYPE_TRANSFORMATION
#define LIBCPP_BITS_TYPE_TRANSFORMATION

#include <__bits/aux.hpp>
#include <type_traits>

namespace std
{
    template<class>
    class reference_wrapper;
}

namespace std::aux
{
    /**
     * Used by tuple to decay reference wrappers to references
     * in make_tuple.
     */

    template<class T>
    struct remove_reference_wrapper: type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_reference_wrapper<reference_wrapper<T>>: type_is<T&>
    { /* DUMMY BODY */ };

    template<class T>
    using remove_reference_wrapper_t = typename remove_reference_wrapper<T>::type;

    template<class T>
    using transform_tuple_types_t = remove_reference_wrapper_t<decay_t<T>>;
}

#endif
