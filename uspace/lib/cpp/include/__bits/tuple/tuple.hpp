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

#ifndef LIBCPP_BITS_TUPLE
#define LIBCPP_BITS_TUPLE

#include <__bits/aux.hpp>
#include <__bits/tuple/tuple_cat.hpp>
#include <__bits/tuple/tuple_ops.hpp>
#include <__bits/type_transformation.hpp>
#include <functional>
#include <type_traits>
#include <utility>

namespace std
{
    template<class... Ts>
    class tuple;

    /**
     * 20.4.2.4, tuple creation functions:
     */

    namespace aux
    {
        struct ignore_t
        {
            template<class T>
            const ignore_t& operator=(const T&) const
            {
                return *this;
            }
        };
    }

    inline constexpr aux::ignore_t ignore;

    template<class... Ts> // TODO: test the reference_wrapper version once we got reference_wrapper
    constexpr auto make_tuple(Ts&&... ts)
    {
        return tuple<aux::transform_tuple_types_t<Ts>...>(forward<Ts>(ts)...);
    }

    template<class... Ts>
    constexpr tuple<Ts&&...> forward_as_tuple(Ts&&... ts) noexcept
    {
        return tuple<Ts&&...>(forward<Ts>(ts)...);
    }

    template<class... Ts>
    constexpr tuple<Ts&...> tie(Ts&... ts) noexcept
    {
        return tuple<Ts&...>(ts...);
    }

    template<class... Tuples>
    constexpr aux::tuple_cat_type_t<Tuples...> tuple_cat(Tuples&&... tpls)
    { // TODO: currently does not work because of index mismatch
        return aux::tuple_cat(
            forward<Tuples>(tpls)...,
            make_index_sequence<sizeof...(Tuples)>{},
            aux::generate_indices_t<Tuples...>{}
        );
    }

    /**
     * 20.4.2.5, tuple helper classes:
     */

    template<class T>
    class tuple_size; // undefined

    template<class T>
    class tuple_size<const T>
        : public integral_constant<size_t, tuple_size<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    class tuple_size<volatile T>
        : public integral_constant<size_t, tuple_size<T>::value>
    { /* DUMMY BODY */ };

    template<class T>
    class tuple_size<const volatile T>
        : public integral_constant<size_t, tuple_size<T>::value>
    { /* DUMMY BODY */ };

    template<class... Ts>
    class tuple_size<tuple<Ts...>>
        : public integral_constant<size_t, sizeof...(Ts)>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr size_t tuple_size_v = tuple_size<T>::value;

    template<size_t I, class T>
    class tuple_element; // undefined

    template<size_t I, class T>
    class tuple_element<I, const T>
    {
        using type = add_const_t<typename tuple_element<I, T>::type>;
    };

    template<size_t I, class T>
    class tuple_element<I, volatile T>
    {
        using type = add_volatile_t<typename tuple_element<I, T>::type>;
    };

    template<size_t I, class T>
    class tuple_element<I, const volatile T>
    {
        using type = add_cv_t<typename tuple_element<I, T>::type>;
    };

    namespace aux
    {
        template<size_t I, class T, class... Ts>
        struct type_at: type_at<I - 1, Ts...>
        { /* DUMMY BODY */ };

        template<class T, class... Ts>
        struct type_at<0, T, Ts...>
        {
            using type = T;
        };

        template<size_t I, class... Ts>
        using type_at_t = typename type_at<I, Ts...>::type;
    }

    template<size_t I, class... Ts>
    class tuple_element<I, tuple<Ts...>>
    {
        public:
            using type = aux::type_at_t<I, Ts...>;
    };

    template<size_t I, class T>
    using tuple_element_t = typename tuple_element<I, T>::type;

    namespace aux
    {
        template<size_t I, class T>
        struct tuple_element_wrapper
        {
            constexpr tuple_element_wrapper() = default;

            constexpr explicit tuple_element_wrapper(T&& val)
                : value{forward<T>(val)}
            { /* DUMMY BODY */ }

            template<
                class U,
                class = enable_if_t<
                    is_convertible_v<U, T> && !is_same_v<U, T>,
                    void
                >
            >
            constexpr explicit tuple_element_wrapper(U&& val)
                : value(forward<U>(val))
            { /* DUMMY BODY */ }

            T value;
        };

        template<class, class...>
        class tuple_impl; // undefined

        template<size_t... Is, class... Ts>
        class tuple_impl<index_sequence<Is...>, Ts...>: public tuple_element_wrapper<Is, Ts>...
        {
            public:
                constexpr tuple_impl()
                    : tuple_element_wrapper<Is, Ts>{}...
                { /* DUMMY BODY */ }

                template<class... Us>
                constexpr explicit tuple_impl(Us&&... us)
                    : tuple_element_wrapper<Is, Ts>(forward<Us>(us))...
                { /* DUMMY BODY */ }

                template<class... Us>
                constexpr tuple_impl(const tuple<Us...>& tpl)
                    : tuple_impl{tpl, make_index_sequence<sizeof...(Us)>{}}
                { /* DUMMY BODY */ }

                template<class... Us>
                constexpr tuple_impl(tuple<Us...>&& tpl)
                    : tuple_impl{move<tuple<Us...>>(tpl), make_index_sequence<sizeof...(Us)>{}}
                { /* DUMMY BODY */ }

                template<class... Us, size_t... Iss>
                constexpr tuple_impl(const tuple<Us...>& tpl, index_sequence<Iss...>)
                    : tuple_impl{get<Iss>(tpl)...}
                { /* DUMMY BODY */ }

                template<class... Us, size_t... Iss>
                constexpr tuple_impl(tuple<Us...>&& tpl, index_sequence<Iss...>)
                    : tuple_impl{get<Iss>(move(tpl))...}
                { /* DUMMY BODY */ }
        };

        template<class T, class... Ts>
        struct tuple_noexcept_swap
        {
            static constexpr bool value = noexcept(std::swap(declval<T&>(), declval<T&>()))
                && tuple_noexcept_swap<Ts...>::value;
        };

        template<class T>
        struct tuple_noexcept_swap<T>
        {
            static constexpr bool value = noexcept(std::swap(declval<T&>(), declval<T&>()));
        };

        template<class T, class... Ts>
        struct tuple_noexcept_assignment
        {
            static constexpr bool value = is_nothrow_move_assignable<T>::value
                && tuple_noexcept_assignment<Ts...>::value;
        };

        template<class T>
        struct tuple_noexcept_assignment<T>
        {
            static constexpr bool value = is_nothrow_move_assignable<T>::value;
        };
    }

    /**
     * 20.4.2.6, element access:
     */

    template<size_t I, class... Ts>
    constexpr tuple_element_t<I, tuple<Ts...>>& get(tuple<Ts...>& tpl) noexcept
    {
        aux::tuple_element_wrapper<I, tuple_element_t<I, tuple<Ts...>>>& wrapper = tpl;

        return wrapper.value;
    }

    template<size_t I, class... Ts>
    constexpr tuple_element_t<I, tuple<Ts...>>&& get(tuple<Ts...>&& tpl) noexcept
    {
        return forward<typename tuple_element<I, tuple<Ts...>>::type&&>(get<I>(tpl));
    }

    template<size_t I, class... Ts>
    constexpr const tuple_element_t<I, tuple<Ts...>>& get(const tuple<Ts...>& tpl) noexcept
    {
        const aux::tuple_element_wrapper<I, tuple_element_t<I, tuple<Ts...>>>& wrapper = tpl;

        return wrapper.value;
    }

    namespace aux
    {
        template<size_t I, class U, class T, class... Ts>
        struct index_of_type: index_of_type<I + 1, U, Ts...>
        { /* DUMMY BODY */ };

        template<size_t I, class T, class... Ts>
        struct index_of_type<I, T, T, Ts...>: std::integral_constant<std::size_t, I>
        { /* DUMMY BODY */ };
    }

    template<class T, class... Ts>
    constexpr T& get(tuple<Ts...>& tpl) noexcept
    {
        return get<aux::index_of_type<0, T, Ts...>::value>(tpl);
    }

    template<class T, class... Ts>
    constexpr T&& get(tuple<Ts...>&& tpl) noexcept
    {
        return get<aux::index_of_type<0, T, Ts...>::value>(forward<tuple<Ts...>>(tpl));
    }

    template<class T, class... Ts>
    constexpr const T& get(const tuple<Ts...>& tpl) noexcept
    {
        return get<aux::index_of_type<0, T, Ts...>::value>(tpl);
    }

    /**
     * 20.4.2, class template tuple:
     */

    template<class... Ts>
    class tuple: public aux::tuple_impl<make_index_sequence<sizeof...(Ts)>, Ts...>
    {
        using base_t = aux::tuple_impl<make_index_sequence<sizeof...(Ts)>, Ts...>;

        public:

            /**
             * 20.4.2.1, tuple construction:
             */

            constexpr tuple()
                : base_t{}
            { /* DUMMY BODY */ }

            constexpr explicit tuple(
                const Ts&... ts, enable_if_t<sizeof...(Ts) != 0>* = nullptr)
                : base_t(ts...)
            { /* DUMMY BODY */ }

            template<class... Us> // TODO: is_convertible == true for all Us to all Ts
            constexpr explicit tuple(Us&&... us, enable_if_t<sizeof...(Us) == sizeof...(Ts)>* = nullptr)
                : base_t(forward<Us>(us)...)
            { /* DUMMY BODY */ }

            tuple(const tuple&) = default;
            tuple(tuple&&) = default;

            template<class... Us>
            constexpr tuple(const tuple<Us...>& tpl, enable_if_t<sizeof...(Us) == sizeof...(Ts)>* = nullptr)
                : base_t(tpl)
            { /* DUMMY BODY */ }

            template<class... Us>
            constexpr tuple(tuple<Us...>&& tpl, enable_if_t<sizeof...(Us) == sizeof...(Ts)>* = nullptr)
                : base_t(move(tpl))
            { /* DUMMY BODY */ }

            // TODO: pair related construction and assignment needs convertibility, not size
            template<class U1, class U2>
            constexpr tuple(const pair<U1, U2>& p)
                : base_t{}
            {
                get<0>(*this) = p.first;
                get<1>(*this) = p.second;
            }

            template<class U1, class U2>
            constexpr tuple(pair<U1, U2>&& p)
                : base_t{}
            {
                get<0>(*this) = forward<U1>(p.first);
                get<1>(*this) = forward<U2>(p.second);
            }

            // TODO: allocator-extended constructors

            /**
             * 20.4.2.2, tuple assignment:
             */

            tuple& operator=(const tuple& other)
            {
                aux::tuple_ops<0, sizeof...(Ts) - 1>::assign_copy(*this, other);

                return *this;
            }

            tuple& operator=(tuple&& other) noexcept(aux::tuple_noexcept_assignment<Ts...>::value)
            {
                aux::tuple_ops<0, sizeof...(Ts) - 1>::assign_move(*this, move(other));

                return *this;
            }

            template<class... Us>
            tuple& operator=(const tuple<Us...>& other)
            {
                aux::tuple_ops<0, sizeof...(Ts) - 1>::assign_copy(*this, other);

                return *this;
            }

            template<class... Us>
            tuple& operator=(tuple<Us...>&& other)
            {
                aux::tuple_ops<0, sizeof...(Ts) - 1>::assign_move(*this, move(other));

                return *this;
            }

            template<class U1, class U2>
            tuple& operator=(const pair<U1, U2>& p)
            {
                get<0>(*this) = p.first;
                get<1>(*this) = p.second;

                return *this;
            }

            template<class U1, class U2>
            tuple& operator=(pair<U1, U2>&& p)
            {
                get<0>(*this) = forward<U1>(p.first);
                get<1>(*this) = forward<U2>(p.second);

                return *this;
            }

            /**
             * 20.4.2.3, tuple swap:
             */

            void swap(tuple& other) noexcept(aux::tuple_noexcept_swap<Ts...>::value)
            {
                aux::tuple_ops<0, sizeof...(Ts) - 1>::swap(*this, other);
            }
    };

    template<>
    class tuple<>
    {
        /**
         * In some cases, e.g. for async() calls
         * without argument to the functors, we need
         * zero length tuples, which are provided by
         * the following specialization.
         * (Without it we get a resolution conflict between
         * the default constructor and the Ts... constructor
         * in the original tuple.)
         */

        tuple() = default;

        void swap(tuple&) noexcept
        { /* DUMMY BODY */ }
    };

    /**
     * 20.4.2.7, relational operators:
     */

    template<class... Ts, class... Us>
    constexpr bool operator==(const tuple<Ts...>& lhs, const tuple<Us...> rhs)
    {
        if constexpr (sizeof...(Ts) == 0)
            return true;
        else
            return aux::tuple_ops<0, sizeof...(Ts) - 1>::eq(lhs, rhs);
    }

    template<class... Ts, class... Us>
    constexpr bool operator<(const tuple<Ts...>& lhs, const tuple<Us...> rhs)
    {
        if constexpr (sizeof...(Ts) == 0)
            return false;
        else
            return aux::tuple_ops<0, sizeof...(Ts) - 1>::lt(lhs, rhs);
    }

    template<class... Ts, class... Us>
    constexpr bool operator!=(const tuple<Ts...>& lhs, const tuple<Us...> rhs)
    {
        return !(lhs == rhs);
    }

    template<class... Ts, class... Us>
    constexpr bool operator>(const tuple<Ts...>& lhs, const tuple<Us...> rhs)
    {
        return rhs < lhs;
    }

    template<class... Ts, class... Us>
    constexpr bool operator<=(const tuple<Ts...>& lhs, const tuple<Us...> rhs)
    {
        return !(rhs < lhs);
    }

    template<class... Ts, class... Us>
    constexpr bool operator>=(const tuple<Ts...>& lhs, const tuple<Us...> rhs)
    {
        return !(lhs < rhs);
    }

    /**
     * 20.4.2.8, allocator-related traits:
     */

    template<class... Ts, class Alloc>
    struct uses_allocator<tuple<Ts...>, Alloc>: true_type
    { /* DUMMY BODY */ };

    /**
     * 20.4.2.9, specialized algorithms:
     */

    template<class... Ts>
    void swap(tuple<Ts...>& lhs, tuple<Ts...>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif
