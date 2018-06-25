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

#ifndef LIBCPP_BITS_TYPE_TRAITS
#define LIBCPP_BITS_TYPE_TRAITS

#include <__bits/aux.hpp>
#include <__bits/type_traits/references.hpp>
#include <cstdlib>
#include <cstddef>
#include <cstdint>

namespace std
{
    template<class T>
    struct add_rvalue_reference;

    template<class T>
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

    template<class T>
    add_rvalue_reference_t<T> declval() noexcept;

    template<class...>
    using void_t = void;

    template<class>
    struct remove_cv;

    template<class T>
    using remove_cv_t = typename remove_cv<T>::type;

    namespace aux
    {
        template<class T, class U>
        struct is_same: false_type
        { /* DUMMY BODY */ };

        template<class T>
        struct is_same<T, T>: true_type
        { /* DUMMY BODY */ };

        template<class T, class... Ts>
        struct is_one_of;

        template<class T>
        struct is_one_of<T>: false_type
        { /* DUMMY BODY */ };

        template<class T, class... Ts>
        struct is_one_of<T, T, Ts...>: true_type
        { /* DUMMY BODY */ };

        template<class T, class U, class... Ts>
        struct is_one_of<T, U, Ts...>: is_one_of<T, Ts...>
        { /* DUMMY BODY */ };
    }

    /**
     * 20.10.4.1, primary type categories:
     */

    template<class T>
    struct is_void: aux::is_same<remove_cv_t<T>, void>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_void_v = is_void<T>::value;

    template<class T>
    struct is_null_pointer: aux::is_same<remove_cv_t<T>, nullptr_t>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_null_pointer_v = is_null_pointer<T>::value;

    template<class T>
    struct is_integral: aux::is_one_of<remove_cv_t<T>,
            bool, char, unsigned char, signed char,
            long, unsigned long, int, unsigned int, short,
            unsigned short, long long, unsigned long long,
            char16_t, char32_t, wchar_t>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_floating_point
        : aux::is_one_of<remove_cv_t<T>, float, double, long double>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_floating_point_v = is_floating_point<T>::value;

    template<class>
    struct is_array: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_array<T[]>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_array_v = is_array<T>::value;

    namespace aux
    {
        template<class>
        struct is_pointer: false_type
        { /* DUMMY BODY */ };

        template<class T>
        struct is_pointer<T*>: true_type
        { /* DUMMY BODY */ };
    }

    template<class T>
    struct is_pointer: aux::is_pointer<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_pointer_v = is_pointer<T>::value;

    template<class T>
    struct is_lvalue_reference: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_lvalue_reference<T&>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

    template<class T>
    struct is_rvalue_reference: false_type
    { /* DUMMY BODY*/ };

    template<class T>
    struct is_rvalue_reference<T&&>: true_type
    { /* DUMMY BODY*/ };

    template<class T>
    inline constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;

    template<class>
    struct is_member_pointer;

    template<class>
    struct is_member_function_pointer;

    template<class T>
    struct is_member_object_pointer
        : integral_constant<bool, is_member_pointer<T>::value &&
                            !is_member_function_pointer<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_member_object_pointer_v = is_member_object_pointer<T>::value;

    template<class>
    struct is_function;

    namespace aux
    {
        template<class T>
        struct is_member_function_pointer: false_type
        { /* DUMMY BODY */ };

        template<class T, class U>
        struct is_member_function_pointer<T U::*>: is_function<T>
        { /* DUMMY BODY */ };
    }

    template<class T>
    struct is_member_function_pointer: aux::is_member_function_pointer<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_member_function_pointer_v = is_member_function_pointer<T>::value;

    template<class T>
    struct is_enum: aux::value_is<bool, __is_enum(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_enum_v = is_enum<T>::value;

    template<class T>
    struct is_union: aux::value_is<bool, __is_union(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_union_v = is_union<T>::value;

    template<class T>
    struct is_class: aux::value_is<bool, __is_class(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_class_v = is_class<T>::value;

    /**
     * Note: is_function taken from possile implementation on cppreference.com
     */
    template<class>
    struct is_function: false_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...)>: true_type
    { /* DUMMY BODY */ };

    // Note: That hexadot covers variadic functions like printf.
    template<class Ret, class... Args>
    struct is_function<Ret(Args......)>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) volatile>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const volatile>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) volatile>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const volatile>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) volatile &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const volatile &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) volatile &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const volatile &>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) volatile &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const volatile &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) volatile &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const volatile &&>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) volatile noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const volatile noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) volatile noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const volatile noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) volatile & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const volatile & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) volatile & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const volatile & noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) volatile && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args...) const volatile && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) volatile && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class Ret, class... Args>
    struct is_function<Ret(Args......) const volatile && noexcept>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_function_v = is_function<T>::value;

    /**
     * 20.10.4.2, composite type categories:
     */

    template<class T>
    struct is_reference: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_reference<T&>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_reference<T&&>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_reference_v = is_reference<T>::value;

    template<class T>
    struct is_arithmetic: aux::value_is<
        bool,
        is_integral<T>::value || is_floating_point<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

    template<class T>
    struct is_fundamental;

    template<class T>
    struct is_object;

    template<class T>
    struct is_scalar;

    template<class T>
    struct is_compound;

    namespace aux
    {
        template<class T>
        struct is_member_pointer: false_type
        { /* DUMMY BODY */ };

        template<class T, class U>
        struct is_member_pointer<T U::*>: true_type
        { /* DUMMY BODY */ };
    }

    template<class T>
    struct is_member_pointer: aux::is_member_pointer<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_member_pointer_v = is_member_pointer<T>::value;

    /**
     * 20.10.4.3, type properties:
     */

    template<class T>
    struct is_const: false_type
    { /* DUMMY BODY */};

    template<class T>
    struct is_const<T const>: true_type
    { /* DUMMY BODY */};

    template<class T>
    inline constexpr bool is_const_v = is_const<T>::value;

    template<class T>
    struct is_volatile: false_type
    { /* DUMMY BODY */};

    template<class T>
    struct is_volatile<T volatile>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_volatile_v = is_volatile<T>::value;

    /**
     * 2 forward declarations.
     */

    template<class T>
    struct is_trivially_copyable;

    template<class T>
    struct is_trivially_default_constructible;

    template<class T>
    struct is_trivial: aux::value_is<
        bool,
        is_trivially_copyable<T>::value &&
        is_trivially_default_constructible<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_trivial_v = is_trivial<T>::value;

    template<class T>
    struct is_trivially_copyable: aux::value_is<bool, __has_trivial_copy(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;

    template<class T>
    struct is_standard_layout: aux::value_is<bool, __is_standard_layout(T)>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_pod: aux::value_is<bool, __is_pod(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_pod_v = is_pod<T>::value;

    template<class T>
    struct is_literal_type: aux::value_is<bool, __is_literal_type(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_literal_type_v = is_literal_type<T>::value;

    template<class T>
    struct is_empty: aux::value_is<bool, __is_empty(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_empty_v = is_empty<T>::value;

    template<class T>
    struct is_polymorphic: aux::value_is<bool, __is_polymorphic(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_polymorphic_v = is_polymorphic<T>::value;

    template<class T>
    struct is_abstract: aux::value_is<bool, __is_abstract(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_abstract_v = is_abstract<T>::value;

    template<class T>
    struct is_final: aux::value_is<bool, __is_final(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_final_v = is_final<T>::value;

    namespace aux
    {
        /**
         * Note: We cannot simply use !is_signed to define
         *       is_unsigned because non-arithmetic types
         *       are neither signer nor unsigned.
         */
        template<class T, bool = is_arithmetic<T>::value>
        struct is_signed: value_is<bool, T(-1) < T(0)>
        { /* DUMMY BODY */ };

        template<class T>
        struct is_signed<T, false>: false_type
        { /* DUMMY BODY */ };

        template<class T, bool = is_arithmetic<T>::value>
        struct is_unsigned: value_is<bool, T(0) < T(-1)>
        { /* DUMMY BODY */ };

        template<class T>
        struct is_unsigned<T, false>: false_type
        { /* DUMMY BODY */ };
    }

    template<class T>
    struct is_signed: aux::is_signed<T>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_signed_v = is_signed<T>::value;

    template<class T>
    struct is_unsigned: aux::is_unsigned<T>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_unsigned_v = is_unsigned<T>::value;

    namespace aux
    {
        template<class, class T, class... Args>
        struct is_constructible: false_type
        { /* DUMMY BODY */ };

        template<class T, class... Args>
        struct is_constructible<
            void_t<decltype(T(declval<Args>()...))>,
            T, Args...
        >
            : true_type
        { /* DUMMY BODY */ };
    }

    template<class T, class... Args>
    struct is_constructible: aux::is_constructible<void_t<>, T, Args...>
    { /* DUMMY BODY */ };

    template<class T, class... Args>
    inline constexpr bool is_constructible_v = is_constructible<T, Args...>::value;

    template<class T>
    struct is_default_constructible
        : is_constructible<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_copy_constructible
        : is_constructible<T, add_lvalue_reference<const T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_move_constructible
        : is_constructible<T, add_rvalue_reference<T>>
    { /* DUMMY BODY */ };

    template<class T, class U, class = void>
    struct is_assignable: false_type
    { /* DUMMY BODY */ };

    template<class T, class U>
    struct is_assignable<T, U, void_t<decltype(declval<T>() = declval<U>())>>
        : true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_copy_assignable
        : is_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_move_assignable
        : is_assignable<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>>
    { /* DUMMY BODY */ };

    template<class, class = void>
    struct is_destructible: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_destructible<T, void_t<decltype(declval<T&>().~T())>>
        : true_type
    { /* DUMMY BODY */ };

    template<class T, class... Args>
    struct is_trivially_constructible: aux::value_is<bool, __has_trivial_constructor(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_trivially_constructible_v = is_trivially_constructible<T>::value;

    template<class T>
    struct is_trivially_default_constructible
        : is_trivially_constructible<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_trivially_copy_constructible: aux::value_is<bool, __has_trivial_copy(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_trivially_copy_constructible_v = is_trivially_copy_constructible<T>::value;

    template<class T>
    struct is_trivially_move_constructible
        : is_trivially_constructible<T, add_rvalue_reference_t<T>>
    { /* DUMMY BODY */ };

    template<class T, class U>
    struct is_trivially_assignable: aux::value_is<bool, __has_trivial_assign(T) && is_assignable<T, U>::value>
    { /* DUMMY BODY */ };

    template<class T, class U>
    inline constexpr bool is_trivially_assignable_v = is_trivially_assignable<T, U>::value;

    template<class T>
    struct is_trivially_copy_assignable
        : is_trivially_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_trivially_move_assignable
        : is_trivially_assignable<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_trivially_destructible: aux::value_is<bool, __has_trivial_destructor(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;

    template<class T, class... Args>
    struct is_nothrow_constructible: aux::value_is<bool, __has_nothrow_constructor(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_nothrow_constructible_v = is_nothrow_constructible<T>::value;

    template<class T>
    struct is_nothrow_default_constructible
        : is_nothrow_constructible<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_nothrow_copy_constructible: aux::value_is<bool, __has_nothrow_copy(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_nothrow_copy_constructible_v = is_nothrow_copy_constructible<T>::value;

    template<class T>
    struct is_nothrow_move_constructible
        : is_nothrow_constructible<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>>
    { /* DUMMY BODY */ };

    template<class T, class U>
    struct is_nothrow_assignable: aux::value_is<bool, __has_nothrow_assign(T)>
    { /* DUMMY BODY */ };

    template<class T, class U>
    inline constexpr bool is_nothrow_assignable_v = is_nothrow_assignable<T, U>::value;

    template<class T>
    struct is_nothrow_copy_assignable
        : is_nothrow_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_nothrow_move_assignable
        : is_nothrow_assignable<add_lvalue_reference_t<T>, add_rvalue_reference_t<T>>
    { /* DUMMY BODY */ };

    template<class, class = void>
    struct is_nothrow_destructible: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_nothrow_destructible<T, void_t<decltype(declval<T&>().~T())>>
        : aux::value_is<bool, noexcept(declval<T&>().~T())>
    { /* DUMMY BODY */ };

    template<class T>
    struct has_virtual_destructor: aux::value_is<bool, __has_virtual_destructor(T)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool has_virtual_destructor_v = has_virtual_destructor<T>::value;

    /**
     * 20.10.5, type property queries:
     */

    template<class T>
    struct alignment_of: aux::value_is<std::size_t, alignof(T)>
    { /* DUMMY BODY */ };

    template<class>
    struct rank : aux::value_is<size_t, 0u>
    { /* DUMMY BODY */ };

    template<class T, size_t N>
    struct rank<T[N]>: aux::value_is<size_t, 1u + rank<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    struct rank<T[]>: aux::value_is<size_t, 1u + rank<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr size_t rank_v = rank<T>::value;

    template<class T, unsigned I = 0U>
    struct extent: aux::value_is<size_t, 0U>
    { /* DUMMY BODY */ };

    template<class T>
    struct extent<T[], 0U>: aux::value_is<size_t, 0U>
    { /* DUMMY BODY */ };

    template<class T, unsigned I>
    struct extent<T[], I>: extent<T, I - 1>
    { /* DUMMY BODY */ };

    template<class T, size_t N>
    struct extent<T[N], 0U>: aux::value_is<size_t, 0U>
    { /* DUMMY BODY */ };

    template<class T, unsigned I, size_t N>
    struct extent<T[N], I>: extent<T, I - 1>
    { /* DUMMY BODY */ };

    /**
     * 20.10.6, type relations:
     */

    template<class T, class U>
    struct is_same: aux::is_same<T, U>
    { /* DUMMY BODY */ };

    template<class T, class U>
    inline constexpr bool is_same_v = is_same<T, U>::value;

    template<class Base, class Derived>
    struct is_base_of: aux::value_is<bool, __is_base_of(Base, Derived)>
    { /* DUMMY BODY */ };

    template<class Base, class Derived>
    inline constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;

    template<class From, class To, class = void>
    struct is_convertible: false_type
    { /* DUMMY BODY */ };

    template<class From, class To>
    struct is_convertible<From, To, void_t<decltype((To)declval<From>())>>
        : true_type
    { /* DUMMY BODY */ };

    template<class From, class To>
    inline constexpr bool is_convertible_v = is_convertible<From, To>::value;

    /**
     * 20.10.7.1, const-volatile modifications:
     */

    template<class T>
    struct remove_const: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_const<T const>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_volatile: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_volatile<T volatile>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_cv: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_cv<T const>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_cv<T volatile>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_cv<T const volatile>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct add_const: aux::type_is<T const>
    { /* DUMMY BODY */ };

    template<class T>
    struct add_volatile: aux::type_is<T volatile>
    { /* DUMMY BODY */ };

    template<class T>
    struct add_cv: aux::type_is<T const volatile>
    { /* DUMMY BODY */ };

    template<class T>
    using remove_const_t = typename remove_const<T>::type;

    template<class T>
    using remove_volatile_t = typename remove_volatile<T>::type;

    template<class T>
    using remove_cv_t = typename remove_cv<T>::type;

    template<class T>
    using add_const_t = typename add_const<T>::type;

    template<class T>
    using add_volatile_t = typename add_volatile<T>::type;

    template<class T>
    using add_cv_t = typename add_cv<T>::type;

    /**
     * 20.10.7.3, sign modifications:
     * Note: These are fairly naive implementations that
     *       are meant to keep our code going (i.e. they work
     *       for the most common types, but I'm not sure
     *       if there are any intricacies required by
     *       the standard).
     */

    template<class T>
    struct make_signed: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<>
    struct make_signed<char>: aux::type_is<signed char>
    { /* DUMMY BODY */ };

    template<>
    struct make_signed<unsigned char>: aux::type_is<signed char>
    { /* DUMMY BODY */ };

    template<>
    struct make_signed<unsigned short>: aux::type_is<short>
    { /* DUMMY BODY */ };

    template<>
    struct make_signed<unsigned int>: aux::type_is<int>
    { /* DUMMY BODY */ };

    template<>
    struct make_signed<unsigned long>: aux::type_is<long>
    { /* DUMMY BODY */ };

    template<>
    struct make_signed<unsigned long long>: aux::type_is<long long>
    { /* DUMMY BODY */ };

    template<class T>
    struct make_unsigned: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<>
    struct make_unsigned<char>: aux::type_is<unsigned char>
    { /* DUMMY BODY */ };

    template<>
    struct make_unsigned<signed char>: aux::type_is<unsigned char>
    { /* DUMMY BODY */ };

    template<>
    struct make_unsigned<short>: aux::type_is<unsigned short>
    { /* DUMMY BODY */ };

    template<>
    struct make_unsigned<int>: aux::type_is<unsigned int>
    { /* DUMMY BODY */ };

    template<>
    struct make_unsigned<long>: aux::type_is<unsigned long>
    { /* DUMMY BODY */ };

    template<>
    struct make_unsigned<long long>: aux::type_is<unsigned long long>
    { /* DUMMY BODY */ };

    template<class T>
    using make_signed_t = typename make_signed<T>::type;

    template<class T>
    using make_unsigned_t = typename make_signed<T>::type;

    /**
     * 20.10.7.4, array modifications:
     */

    template<class T>
    struct remove_extent: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_extent<T[]>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T, size_t N>
    struct remove_extent<T[N]>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_all_extents: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_all_extents<T[]>: remove_all_extents<T>
    { /* DUMMY BODY */ };

    template<class T, size_t N>
    struct remove_all_extents<T[N]>: remove_all_extents<T>
    { /* DUMMY BODY */ };

    template<class T>
    using remove_extent_t = typename remove_extent<T>::type;

    template<class T>
    using remove_all_extents_t = typename remove_all_extents<T>::type;

    /**
     * 20.10.7.5, pointer modifications:
     */

    template<class T>
    struct remove_pointer: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_pointer<T*>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_pointer<T* const>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_pointer<T* volatile>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct remove_pointer<T* const volatile>: aux::type_is<T>
    { /* DUMMY BODY */ };

    namespace aux
    {
        template<class T>
        struct add_pointer_to_function: type_is<T>
        { /* DUMMY BODY */ };

        template<class T, class... Args>
        struct add_pointer_to_function<T(Args...)>
            : type_is<T(*)(Args...)>
        { /* DUMMY BODY */ };

        template<class T, bool = false>
        struct add_pointer_cond: aux::type_is<remove_reference_t<T>*>
        { /* DUMMY BODY */ };

        template<class T>
        struct add_pointer_cond<T, true>
            : aux::add_pointer_to_function<T>
        { /* DUMMY BODY */ };
    }

    template<class T>
    struct add_pointer: aux::add_pointer_cond<T, is_function_v<T>>
    { /* DUMMY BODY */ };

    template<class T>
    using remove_pointer_t = typename remove_pointer<T>::type;

    template<class T>
    using add_pointer_t = typename add_pointer<T>::type;

    /**
     * 20.10.7.6, other transformations:
     */

    namespace aux
    {
        template<size_t Len, size_t Align>
        struct aligned_t
        {
            alignas(Align) uint8_t storage[Len];
        };

        template<class T>
        constexpr T max_of_cont(T cont)
        {
            if (cont.size() == 0U)
                return T{};

            auto it = cont.begin();
            auto res = *it;

            while (++it != cont.end())
            {
                if (*it > res)
                    res = *it;
            }

            return res;
        }
    }

    // TODO: consult standard on the default value of align
    template<std::size_t Len, std::size_t Align = 0>
    struct aligned_storage: aux::type_is<aux::aligned_t<Len, Align>>
    { /* DUMMY BODY */ };

    template<std::size_t Len, class... Types>
    struct aligned_union: aux::type_is<
        aux::aligned_t<
            aux::max_of_cont({Len, alignof(Types)...}),
            aux::max_of_cont({alignof(Types)...})
        >
    >
    { /* DUMMY BODY */ };

	// TODO: this is very basic implementation for chrono, fix!
    /* template<class T> */
    /* struct decay: aux::type_is<remove_cv_t<remove_reference_t<T>>> */
	/* { /1* DUMMY BODY *1/ }; */

    template<bool, class, class>
    struct conditional;

    template<class T>
    struct decay: aux::type_is<
        typename conditional<
            is_array_v<remove_reference_t<T>>,
            remove_extent_t<remove_reference_t<T>>*,
            typename conditional<
                is_function_v<remove_reference_t<T>>,
                add_pointer_t<remove_reference_t<T>>,
                remove_cv_t<remove_reference_t<T>>
            >::type
        >::type
    >
    { /* DUMMY BODY */ };

	template<class T>
    using decay_t = typename decay<T>::type;

    template<bool, class T = void>
    struct enable_if
    { /* DUMMY BODY */ };

    template<class T>
    struct enable_if<true, T>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<bool, class T, class F>
    struct conditional: aux::type_is<F>
    { /* DUMMY BODY */ };

    template<class T, class F>
    struct conditional<true, T, F>: aux::type_is<T>
    { /* DUMMY BODY */ };

    template<class... T>
    struct common_type
    { /* DUMMY BODY */ };

    template<class... T>
    using common_type_t = typename common_type<T...>::type;

    template<class T>
    struct common_type<T>: common_type<T, T>
    { /* DUMMY BODY */ };

    // To avoid circular dependency with <utility>.
    template<class T>
    add_rvalue_reference_t<T> declval() noexcept;

    template<class T1, class T2>
    struct common_type<T1, T2>: aux::type_is<decay_t<decltype(false ? declval<T1>() : declval<T2>())>>
    { /* DUMMY BODY */ };

    template<class T1, class T2, class... Ts>
    struct common_type<T1, T2, Ts...>
        : aux::type_is<common_type_t<common_type_t<T1, T2>, Ts...>>
    { /* DUMMY BODY */ };

    template<class... T>
    struct underlying_type;

    template<std::size_t Len, std::size_t Align = 0>
    using aligned_storage_t = typename aligned_storage<Len, Align>::type;

    template<std::size_t Len, class... Types>
    using aligned_union_t = typename aligned_union<Len, Types...>::type;

    template<bool b, class T = void>
    using enable_if_t = typename enable_if<b, T>::type;

    template<bool b, class T, class F>
    using conditional_t = typename conditional<b, T, F>::type;

    template<class T>
    using underlying_type_t = typename underlying_type<T>::type;
}

#endif
