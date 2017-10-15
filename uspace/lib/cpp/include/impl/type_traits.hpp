/*
 * Copyright (c) 2017 Jaroslav Jindrak
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

#ifndef LIBCPP_TYPE_TRAITS
#define LIBCPP_TYPE_TRAITS

#include <cstdlib>
#include <cstddef>
#include <internal/aux.hpp>

namespace std
{
    /**
     * 20.10.3, helper class:
     */

    template<class T, T v>
    struct integral_constant
    {
        static constexpr T value = v;

        using value_type = T;
        using type       = integral_constant<T, v>;

        constexpr operator value_type() const noexcept
        {
            return value;
        }

        constexpr value_type operator()() const noexcept
        {
            return value;
        }
    };

    using true_type = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;

    /**
     * 20.10.4.1, primary type categories:
     */

    template<class>
    struct remove_cv;

    template<class T>
    using remove_cv_t = typename remove_cv<T>::type;

    template<class>
    struct __is_void: false_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_void<void>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_void: __is_void<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class>
    struct __is_null_pointer: false_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_null_pointer<nullptr_t>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_null_pointer: __is_null_pointer<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct __is_integral: false_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<bool>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<char>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<signed char>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<unsigned char>: true_type
    { /* DUMMY BODY */ };

    /* TODO: Problem - wchar_t is typedef'd to int here.
    template<>
    struct __is_integral<wchar_t>;
    */

    template<>
    struct __is_integral<long>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<unsigned long>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<int>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<unsigned int>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<short>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<unsigned short>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<long long>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_integral<unsigned long long>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_integral: __is_integral<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class>
    struct __is_floating_point: false_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_floating_point<float>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_floating_point<double>: true_type
    { /* DUMMY BODY */ };

    template<>
    struct __is_floating_point<long double>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_floating_point: __is_floating_point<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class>
    struct is_array: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_array<T[]>: true_type
    { /* DUMMY BODY */ };

    template<class>
    struct __is_pointer: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct __is_pointer<T*>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_pointer: __is_pointer<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_lvalue_reference: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_lvalue_reference<T&>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_rvalue_reference: false_type
    { /* DUMMY BODY*/ };

    template<class T>
    struct is_rvalue_reference<T&&>: true_type
    { /* DUMMY BODY*/ };

    template<class>
    struct is_member_pointer;

    template<class>
    struct is_member_function_pointer;

    template<class T>
    struct is_member_object_pointer
        : integral_constant<bool, is_member_pointer<T>::value &&
                            !is_member_function_pointer<T>::value>
    { /* DUMMY BODY */ };

    template<class>
    struct is_function;

    template<class T>
    struct __is_member_function_pointer: false_type
    { /* DUMMY BODY */ };

    template<class T, class U>
    struct __is_member_function_pointer<T U::*>: is_function<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_member_function_pointer: __is_member_function_pointer<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    template<class T>
    struct is_enum; // TODO: No idea how to implement.

    template<class T>
    struct is_union; // TODO: No idea how to implement.

    template<class T>
    struct is_class; // TODO: No idea how to implement.

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

    /**
     * 20.10.4.2, composite type categories:
     */

    template<class T>
    struct is_reference;

    template<class T>
    struct is_arithmetic;

    template<class T>
    struct is_fundamental;

    template<class T>
    struct is_object;

    template<class T>
    struct is_scalar;

    template<class T>
    struct is_compound;

    template<class T>
    struct __is_member_pointer: false_type
    { /* DUMMY BODY */ };

    template<class T, class U>
    struct __is_member_pointer<T U::*>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_member_pointer: __is_member_pointer<remove_cv_t<T>>
    { /* DUMMY BODY */ };

    /**
     * 20.10.4.3, type properties:
     */

    template<class T>
    struct is_const;

    template<class T>
    struct is_volatile;

    template<class T>
    struct is_trivial;

    template<class T>
    struct is_trivially_copyable;

    template<class T>
    struct is_standard_layout;

    template<class T>
    struct is_pod;

    template<class T>
    struct is_literal_type;

    template<class T>
    struct is_empty;

    template<class T>
    struct is_polymorphic;

    template<class T>
    struct is_abstract;

    template<class T>
    struct is_final;

    template<class T>
    struct is_signed;

    template<class T>
    struct is_unsidned;

    template<class T, class... Args>
    struct is_constructible;

    template<class T>
    struct is_default_constructible;

    template<class T>
    struct is_copy_constructible;

    template<class T>
    struct is_move_constructible;

    template<class T, class U>
    struct is_assignable;

    template<class T>
    struct is_copy_assignable;

    template<class T>
    struct is_move_assignable;

    template<class T>
    struct is_destructible;

    template<class T, class... Args>
    struct is_trivially_constructible;

    template<class T>
    struct is_trivially_default_constructible;

    template<class T>
    struct is_trivially_copy_constructible;

    template<class T>
    struct is_trivially_move_constructible;

    template<class T, class U>
    struct is_trivially_assignable;

    template<class T>
    struct is_trivially_copy_assignable;

    template<class T>
    struct is_trivially_move_assignable;

    template<class T>
    struct is_trivially_destructible;

    template<class T, class... Args>
    struct is_nothrow_constructible;

    template<class T>
    struct is_nothrow_default_constructible;

    template<class T>
    struct is_nothrow_copy_constructible;

    template<class T>
    struct is_nothrow_move_constructible;

    template<class T, class U>
    struct is_nothrow_assignable;

    template<class T>
    struct is_nothrow_copy_assignable;

    template<class T>
    struct is_nothrow_move_assignable;

    template<class T>
    struct is_nothrow_destructible;

    template<class T>
    struct has_virtual_destructor;

    /**
     * 20.10.5, type property queries:
     */

    template<class T>
    struct alignment_of;

    template<class>
    struct rank : aux::value_is<size_t, 0u>
    { /* DUMMY BODY */ };

    template<class T, size_t N>
    struct rank<T[N]>: aux::value_is<size_t, 1u + rank<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    struct rank<T[]>: aux::value_is<size_t, 1u + rank<T>::value>
    { /* DUMMY BODY */ };

    template<class T, unsigned I = 0>
    struct extent;

    /**
     * 20.10.6, type relations:
     */

    template<class T, class U>
    struct is_same: false_type
    { /* DUMMY BODY */ };

    template<class T>
    struct is_same<T, T>: true_type
    { /* DUMMY BODY */ };

    template<class Base, class Derived>
    struct is_base_of;

    template<class From, class To>
    struct is_convertible;

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
    struct remove_volatile;

    template<class T>
    struct remove_cv;

    template<class T>
    struct add_const;

    template<class T>
    struct add_volatile;

    template<class T>
    struct add_cv;

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
     * 20.10.7.2, reference modifications:
     */

    template<class T>
    struct remove_reference;

    template<class T>
    struct add_lvalue_reference;

    template<class T>
    struct add_rvalue_reference;

    template<class T>
    using remove_reference_t = typename remove_reference<T>::type;

    template<class T>
    using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

    template<class T>
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

    /**
     * 20.10.7.3, sign modifications:
     */

    template<class T>
    struct make_signed;

    template<class T>
    struct make_unsigned;

    template<class T>
    using make_signed_t = typename make_signed<T>::type;

    template<class T>
    using make_unsigned_t = typename make_signed<T>::type;

    /**
     * 20.10.7.4, array modifications:
     */

    template<class T>
    struct remove_extent;

    template<class T>
    struct remove_all_extents;

    template<class T>
    using remove_extent_t = typename remove_extent<T>::type;

    template<class T>
    using remove_all_extents_t = typename remove_all_extents<T>::type;

    /**
     * 20.10.7.5, pointer modifications:
     */

    template<class T>
    struct remove_pointer;

    template<class T>
    struct add_pointer;

    template<class T>
    using remove_pointer_t = typename remove_pointer<T>::type;

    template<class T>
    using add_pointer_t = typename add_pointer<T>::type;

    /**
     * 20.10.7.6, other transformations:
     */

    // TODO: consult standard on the default value of align
    template<std::size_t Len, std::size_t Align = 0>
    struct aligned_storage;

    template<std::size_t Len, class... Types>
    struct aligned_union;

    template<class T>
    struct decay;

    template<bool, class T = void>
    struct enable_if;

    template<bool, class T, class F>
    struct conditional;

    template<class... T>
    struct common_type;

    template<class... T>
    struct underlying_type;

    template<class>
    class result_of; // not defined

    template<class F, class... ArgTypes>
    class result_of<F(ArgTypes...)>;

    template<std::size_t Len, std::size_t Align = 0>
    using aligned_storage_t = typename aligned_storage<Len, Align>::type;

    template<std::size_t Len, class... Types>
    using aligned_union_t = typename aligned_union<Len, Types...>::type;

    template<class T>
    using decay_t = typename decay<T>::type;

    template<bool b, class T = void>
    using enable_if_t = typename enable_if<b, T>::type;

    template<bool b, class T, class F>
    using conditional_t = typename conditional<b, T, F>::type;

    template<class... T>
    using common_type_t = typename common_type<T...>::type;

    template<class T>
    using underlying_type_t = typename underlying_type<T>::type;

    template<class T>
    using result_of_t = typename result_of<T>::type;

    template<class...>
    using void_t = void;
}

#endif
