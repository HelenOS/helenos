/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#ifndef LIBCPP_BITS_FUNCTIONAL_BIND
#define LIBCPP_BITS_FUNCTIONAL_BIND

#include <__bits/functional/function.hpp>
#include <__bits/functional/invoke.hpp>
#include <__bits/functional/reference_wrapper.hpp>
#include <cassert>
#include <tuple>
#include <type_traits>
#include <utility>

namespace std
{
    /**
     * 20.9.10, bind:
     */

    namespace aux
    {
        template<int N>
        struct placeholder_t
        {
            constexpr placeholder_t() = default;
            constexpr placeholder_t(const placeholder_t&) = default;
            constexpr placeholder_t(placeholder_t&&) = default;
        };
    }

    template<class T>
    struct is_placeholder: integral_constant<int, 0>
    { /* DUMMY BODY */ };

    template<int N> // Note: const because they are all constexpr.
    struct is_placeholder<const aux::placeholder_t<N>>
        : integral_constant<int, N>
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr int is_placeholder_v = is_placeholder<T>::value;

    namespace aux
    {
        /**
         * Note: Our internal bind return type has an extra type
         *       template parameter and an extra bool template parameter.
         *       We use this for the special version of bind that has
         *       the return type to have a result_type typedef
         *       (i.e. when the bool is true, the extra type parameter
         *       is typedefed as result_type - see the structure below).
         *       This is just a simplification of the implementation
         *       so that we don't need to have two return types for
         *       the two bind functions, because unlike function or
         *       mem_fn, we know exactly when to make the typedef.
         */

        template<class, bool = false>
        struct bind_conditional_result_type
        { /* DUMMY BODY */ };

        template<class R>
        struct bind_conditional_result_type<R, true>
        {
            using result_type = R;
        };

        template<class, bool, class, class...>
        class bind_t;

        /**
         * Filter class that uses its overloaded operator[]
         * to filter our placeholders, reference_wrappers and bind
         * subexpressions and replace them with the correct
         * arguments (extracts references, calls the subexpressions etc).
         */
        template<class... Args>
        class bind_arg_filter
        {
            public:
                bind_arg_filter(Args&&... args)
                    : args_{forward<Args>(args)...}
                { /* DUMMY BODY */ }

                template<class T>
                constexpr decltype(auto) operator[](T&& t)
                {
                    return forward<T>(t);
                }

                template<int N>
                constexpr decltype(auto) operator[](const placeholder_t<N>)
                { // Since placeholders are constexpr, this is the best match for them.
                    /**
                     * Come on, it's int! Why not use -1 as not placeholder
                     * and start them at 0? -.-
                     */
                    return get<N - 1>(args_);
                }

                template<class T>
                constexpr T& operator[](reference_wrapper<T> ref)
                {
                    return ref.get();
                }

                template<class R, bool B, class F, class... BindArgs>
                constexpr decltype(auto) operator[](const bind_t<R, B, F, BindArgs...> b)
                {
                    __unimplemented();
                    return b; // TODO: bind subexpressions
                }


            private:
                tuple<Args...> args_;
        };

        template<class R, bool HasResultType, class F, class... Args>
        class bind_t: public bind_conditional_result_type<R, HasResultType>
        {
            public:
                template<class G, class... BoundArgs>
                constexpr bind_t(G&& g, BoundArgs&&... args)
                    : func_{forward<F>(g)},
                      bound_args_{forward<Args>(args)...}
                { /* DUMMY BODY */ }

                constexpr bind_t(const bind_t& other) = default;
                constexpr bind_t(bind_t&& other) = default;

                template<class... ActualArgs>
                constexpr decltype(auto) operator()(ActualArgs&&... args)
                {
                    return invoke_(
                        make_index_sequence<sizeof...(Args)>{},
                        forward<ActualArgs>(args)...
                    );
                }

            private: // TODO: This has problem with member function pointers.
                function<remove_reference_t<F>> func_;
                tuple<decay_t<Args>...> bound_args_;

                template<size_t... Is, class... ActualArgs>
                constexpr decltype(auto) invoke_(
                    index_sequence<Is...>, ActualArgs&&... args
                )
                {
                    /**
                     * The expression filter[bound_args_[bind_arg_index<Is>()]]...
                     * here expands bind_arg_index to 0, 1, ... sizeof...(ActualArgs) - 1
                     * and then passes this variadic list of indices to the bound_args_
                     * tuple which extracts the bound args from it.
                     * Our filter will then have its operator[] called on each of them
                     * and filter out the placeholders, reference_wrappers etc and changes
                     * them to the actual arguments.
                     */
                    bind_arg_filter<ActualArgs...> filter{forward<ActualArgs>(args)...};

                    return aux::INVOKE(
                        func_,
                        filter[get<Is>(bound_args_)]...
                    );
                }
        };
    }

    template<class T>
    struct is_bind_expression: false_type
    { /* DUMMY BODY */ };

    template<class R, bool B, class F, class... Args>
    struct is_bind_expression<aux::bind_t<R, B, F, Args...>>
        : true_type
    { /* DUMMY BODY */ };

    template<class F, class... Args>
    aux::bind_t<void, false, F, Args...> bind(F&& f, Args&&... args)
    {
        return aux::bind_t<void, false, F, Args...>{forward<F>(f), forward<Args>(args)...};
    }

    template<class R, class F, class... Args>
    aux::bind_t<R, true, F, Args...> bind(F&& f, Args&&... args)
    {
        return aux::bind_t<R, true, F, Args...>{forward<F>(f), forward<Args>(args)...};
    }

    namespace placeholders
    {
        /**
         * Note: The number of placeholders is
         *       implementation defined, we've chosen
         *       8 because it is a nice number
         *       and should be enough for any function
         *       call.
         * Note: According to the C++14 standard, these
         *       are all extern non-const. We decided to use
         *       the C++17 form of them being inline constexpr
         *       because it is more convenient, makes sense
         *       and would eventually need to be upgraded
         *       anyway.
         */
        inline constexpr aux::placeholder_t<1> _1;
        inline constexpr aux::placeholder_t<2> _2;
        inline constexpr aux::placeholder_t<3> _3;
        inline constexpr aux::placeholder_t<4> _4;
        inline constexpr aux::placeholder_t<5> _5;
        inline constexpr aux::placeholder_t<6> _6;
        inline constexpr aux::placeholder_t<7> _7;
        inline constexpr aux::placeholder_t<8> _8;
    }
}

#endif
