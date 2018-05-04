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

#ifndef LIBCPP_FUNCTIONAL
#define LIBCPP_FUNCTIONAL

#include <limits>
#include <memory>
#include <typeinfo>
#include <type_traits>
#include <utility>

namespace std
{
    namespace aux
    {
        /**
         * 20.9.2, requirements:
         */
        template<class R, class T, class T1, class... Ts>
        decltype(auto) invoke(R T::* f, T1&& t1, Ts&&... args)
        {
            if constexpr (is_member_function_pointer_v<decltype(f)>)
            {
                if constexpr (is_base_of_v<T, remove_reference_t<T1>>)
                    // (1.1)
                    return (t1.*f)(forward<Ts>(args)...);
                else
                    // (1.2)
                    return ((*t1).*f)(forward<Ts>(args)...);
            }
            else if constexpr (is_member_object_pointer_v<decltype(f)> && sizeof...(args) == 0)
            {
                /**
                 * Note: Standard requires to N be equal to 1, but we take t1 directly
                 *       so we need sizeof...(args) to be 0.
                 */
                if constexpr (is_base_of_v<T, remove_reference_t<T1>>)
                    // (1.3)
                    return t1.*f;
                else
                    // (1.4)
                    return (*t1).*f;
            }

            /**
             * Note: If this condition holds this will not be reachable,
             *       but a new addition to the standard (17.7 point (8.1))
             *       prohibits us from simply using false as the condition here,
             *       so we use this because we know it is false here.
             */
            static_assert(is_member_function_pointer_v<decltype(f)>, "invalid invoke");
        }

        template<class F, class... Args>
        decltype(auto) invoke(F&& f, Args&&... args)
        {
            // (1.5)
            return f(forward<Args>(args)...);
        }
    }

    /**
     * 20.9.3, invoke:
     */

    template<class F, class... Args>
    result_of_t<F&&(Args&&...)> invoke(F&& f, Args&&... args)
    {
        return aux::invoke(forward<F>(f)(forward<Args>(args)...));
    }

    /**
     * 20.9.4, reference_wrapper:
     */

    template<class T>
    class reference_wrapper
    {
        public:
            using type = T;
            // TODO: conditional typedefs

            reference_wrapper(type& val) noexcept
                : data_{&val}
            { /* DUMMY BODY */ }

            reference_wrapper(type&&) = delete;

            reference_wrapper(const reference_wrapper& other) noexcept
                : data_{other.data_}
            { /* DUMMY BODY */ }

            reference_wrapper& operator=(const reference_wrapper& other) noexcept
            {
                data_ = other.data_;

                return *this;
            }

            operator type&() const noexcept
            {
                return *data_;
            }

            type& get() const noexcept
            {
                return *data_;
            }

            template<class... Args>
            result_of_t<type&(Args&&...)> operator()(Args&&... args) const
            {
                return invoke(*data_, std::forward<Args>(args)...);
            }

        private:
            type* data_;
    };

    template<class T>
    reference_wrapper<T> ref(T& t) noexcept
    {
        return reference_wrapper<T>{t};
    }

    template<class T>
    reference_wrapper<const T> cref(const T& t) noexcept
    {
        return reference_wrapper<const T>{t};
    }

    template<class T>
    void ref(const T&&) = delete;

    template<class T>
    void cref(const T&&) = delete;

    template<class T>
    reference_wrapper<T> ref(reference_wrapper<T> t) noexcept
    {
        return ref(t.get());
    }

    template<class T>
    reference_wrapper<const T> cref(reference_wrapper<T> t) noexcept
    {
        return cref(t.get());
    }

    /**
     * 20.9.5, arithmetic operations:
     */

    template<class T = void>
    struct plus
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs + rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct minus
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs - rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct multiplies
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs * rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct divides
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs / rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct modulus
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs % rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct negate
    {
        constexpr T operator()(const T& x) const
        {
            return -x;
        }

        using argument_type = T;
        using result_type   = T;
    };

    namespace aux
    {
        /**
         * Used by some functions like std::set::find to determine
         * whether a functor is transparent.
         */
        struct transparent_t;

        template<class T, class = void>
        struct is_transparent: false_type
        { /* DUMMY BODY */ };

        template<class T>
        struct is_transparent<T, void_t<typename T::is_transparent>>
            : true_type
        { /* DUMMY BODY */ };

        template<class T>
        inline constexpr bool is_transparent_v = is_transparent<T>::value;
    }

    template<>
    struct plus<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) + forward<U>(rhs))
        {
            return forward<T>(lhs) + forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct minus<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) - forward<U>(rhs))
        {
            return forward<T>(lhs) - forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct multiplies<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) * forward<U>(rhs))
        {
            return forward<T>(lhs) * forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct divides<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) / forward<U>(rhs))
        {
            return forward<T>(lhs) / forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct modulus<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) % forward<U>(rhs))
        {
            return forward<T>(lhs) % forward<T>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct negate<void>
    {
        template<class T>
        constexpr auto operator()(T&& x) const
            -> decltype(-forward<T>(x))
        {
            return -forward<T>(x);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.6, comparisons:
     */

    template<class T = void>
    struct equal_to
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs == rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct not_equal_to
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs != rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct greater
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs > rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct less
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs < rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct greater_equal
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs >= rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct less_equal
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs <= rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<>
    struct equal_to<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) == forward<U>(rhs))
        {
            return forward<T>(lhs) == forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct not_equal_to<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) != forward<U>(rhs))
        {
            return forward<T>(lhs) != forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct greater<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) > forward<U>(rhs))
        {
            return forward<T>(lhs) > forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct less<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) < forward<U>(rhs))
        {
            return forward<T>(lhs) < forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct greater_equal<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) >= forward<U>(rhs))
        {
            return forward<T>(lhs) >= forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct less_equal<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) <= forward<U>(rhs))
        {
            return forward<T>(lhs) <= forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.7, logical operations:
     */

    template<class T = void>
    struct logical_and
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs && rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct logical_or
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs || rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = bool;
    };

    template<class T = void>
    struct logical_not
    {
        constexpr bool operator()(const T& x) const
        {
            return !x;
        }

        using argument_type = T;
        using result_type   = bool;
    };

    template<>
    struct logical_and<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) && forward<U>(rhs))
        {
            return forward<T>(lhs) && forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct logical_or<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) || forward<U>(rhs))
        {
            return forward<T>(lhs) || forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct logical_not<void>
    {
        template<class T>
        constexpr auto operator()(T&& x) const
            -> decltype(!forward<T>(x))
        {
            return !forward<T>(x);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.8, bitwise operations:
     */

    template<class T = void>
    struct bit_and
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs & rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct bit_or
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs | rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct bit_xor
    {
        constexpr T operator()(const T& lhs, const T& rhs) const
        {
            return lhs ^ rhs;
        }

        using first_argument_type  = T;
        using second_argument_type = T;
        using result_type          = T;
    };

    template<class T = void>
    struct bit_not
    {
        constexpr bool operator()(const T& x) const
        {
            return ~x;
        }

        using argument_type = T;
        using result_type   = T;
    };

    template<>
    struct bit_and<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) & forward<U>(rhs))
        {
            return forward<T>(lhs) & forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct bit_or<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) | forward<U>(rhs))
        {
            return forward<T>(lhs) | forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct bit_xor<void>
    {
        template<class T, class U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(forward<T>(lhs) ^ forward<U>(rhs))
        {
            return forward<T>(lhs) ^ forward<U>(rhs);
        }

        using is_transparent = aux::transparent_t;
    };

    template<>
    struct bit_not<void>
    {
        template<class T>
        constexpr auto operator()(T&& x) const
            -> decltype(~forward<T>(x))
        {
            return ~forward<T>(x);
        }

        using is_transparent = aux::transparent_t;
    };

    /**
     * 20.9.9, negators:
     */

    template<class Predicate>
    class unary_negate
    {
        public:
            using result_type   = bool;
            using argument_type = typename Predicate::argument_type;

            constexpr explicit unary_negate(const Predicate& pred)
                : pred_{pred}
            { /* DUMMY BODY */ }

            constexpr result_type operator()(const argument_type& arg)
            {
                return !pred_(arg);
            }

        private:
            Predicate pred_;
    };

    template<class Predicate>
    constexpr unary_negate<Predicate> not1(const Predicate& pred)
    {
        return unary_negate<Predicate>{pred};
    }

    template<class Predicate>
    class binary_negate
    {
        public:
            using result_type          = bool;
            using first_argument_type  = typename Predicate::first_argument_type;
            using second_argument_type = typename Predicate::second_argument_type;

            constexpr explicit binary_negate(const Predicate& pred)
                : pred_{pred}
            { /* DUMMY BODY */ }

            constexpr result_type operator()(const first_argument_type& arg1,
                                             const second_argument_type& arg2)
            {
                return !pred_(arg1, arg2);
            }

        private:
            Predicate pred_;
    };

    template<class Predicate>
    constexpr binary_negate<Predicate> not2(const Predicate& pred);

    /**
     * 20.9.12, polymorphic function adaptors:
     */

    namespace aux
    {
        // TODO: fix this
        /* template<class, class T, class... Args> */
        /* struct is_callable_impl: false_type */
        /* { /1* DUMMY BODY *1/ }; */

        /* template<class, class R, class... Args> */
        /* struct is_callable_impl< */
        /*     void_t<decltype(aux::invoke(declval<R(Args...)>(), declval<Args>()..., R))>, */
        /*     R, Args... */
        /* > : true_type */
        /* { /1* DUMMY BODY *1/ }; */

        /* template<class T> */
        /* struct is_callable: is_callable_impl<void_t<>, T> */
        /* { /1* DUMMY BODY *1/ }; */

        template<class Callable, class R, class... Args>
        R invoke_callable(Callable* clbl, Args&&... args)
        {
            return (*clbl)(forward<Args>(args)...);
        }

        template<class Callable>
        void copy_callable(Callable* to, Callable* from)
        {
            new(to) Callable{*from};
        }

        template<class Callable>
        void destroy_callable(Callable* clbl)
        {
            if (clbl)
                clbl->~Callable();
        }
    }

    // TODO: implement
    class bad_function_call;

    template<class>
    class function; // undefined

    /**
     * Note: Ideally, this implementation wouldn't
     *       copy the target if it was a pointer to
     *       a function, but for the simplicity of the
     *       implementation, we do copy even in that
     *       case for now. It would be a nice optimization
     *       if this was changed in the future.
     */
    template<class R, class... Args>
    class function<R(Args...)>
    {
        public:
            using result_type = R;
            // TODO: conditional typedefs

            /**
             * 20.9.12.2.1, construct/copy/destroy:
             */

            function() noexcept
                : callable_{}, callable_size_{}, call_{},
                  copy_{}, dest_{}
            { /* DUMMY BODY */ }

            function(nullptr_t) noexcept
                : function{}
            { /* DUMMY BODY */ }

            function(const function& other)
                : callable_{}, callable_size_{other.callable_size_},
                  call_{other.call_}, copy_{other.copy_}, dest_{other.dest_}
            {
                callable_ = new uint8_t[callable_size_];
                (*copy_)(callable_, other.callable_);
            }

            function(function&& other)
                : callable_{other.callable_}, callable_size_{other.callable_size_},
                  call_{other.call_}, copy_{other.copy_}, dest_{other.dest_}
            {
                other.callable_ = nullptr;
                other.callable_size_ = size_t{};
                other.call_ = nullptr;
                other.copy_ = nullptr;
                other.dest_ = nullptr;
            }

            // TODO: shall not participate in overloading unless aux::is_callable<F>
            template<class F>
            function(F f)
                : callable_{}, callable_size_{sizeof(F)},
                  call_{(call_t)aux::invoke_callable<F, R, Args...>},
                  copy_{(copy_t)aux::copy_callable<F>},
                  dest_{(dest_t)aux::destroy_callable<F>}
            {
                callable_ = new uint8_t[callable_size_];
                (*copy_)(callable_, (uint8_t*)&f);
            }

            /**
             * Note: For the moment we're ignoring the allocator
             *       for simplicity of the implementation.
             */

            template<class A>
            function(allocator_arg_t, const A& a) noexcept
                : function{}
            { /* DUMMY BODY */ }

            template<class A>
            function(allocator_arg_t, const A& a, nullptr_t) noexcept
                : function{}
            { /* DUMMY BODY */ }

            template<class A>
            function(allocator_arg_t, const A& a, const function& other)
                : function{other}
            { /* DUMMY BODY */ }

            template<class A>
            function(allocator_arg_t, const A& a, function&& other)
                : function{move(other)}
            { /* DUMMY BODY */ }

            // TODO: shall not participate in overloading unless aux::is_callable<F>
            template<class F, class A>
            function(allocator_arg_t, const A& a, F f)
                : function{f}
            { /* DUMMY BODY */ }

            function& operator=(const function& rhs)
            {
                function{rhs}.swap(*this);

                return *this;
            }

            /**
             * Note: We have to copy call_, copy_
             *       and dest_ because they can be templated
             *       by a type F we don't know.
             */
            function& operator=(function&& rhs)
            {
                clear_();

                callable_ = rhs.callable_;
                callable_size_ = rhs.callable_size_;
                call_ = rhs.call_;
                copy_ = rhs.copy_;
                dest_ = rhs.dest_;

                rhs.callable_ = nullptr;
                rhs.callable_size_ = size_t{};
                rhs.call_ = nullptr;
                rhs.copy_ = nullptr;
                rhs.dest_ = nullptr;

                return *this;
            }

            function& operator=(nullptr_t) noexcept
            {
                clear_();

                return *this;
            }

            // TODO: shall not participate in overloading unless aux::is_callable<F>
            template<class F>
            function& operator=(F&& f)
            {
                callable_size_ = sizeof(F);
                callable_ = new uint8_t[callable_size_];
                call_ = aux::invoke_callable<F, R, Args...>;
                copy_ = aux::copy_callable<F>;
                dest_ = aux::destroy_callable<F>;

                (*copy_)(callable_, (uint8_t*)&f);
            }

            template<class F>
            function& operator=(reference_wrapper<F> ref) noexcept
            {
                return (*this) = ref.get();
            }

            ~function()
            {
                if (callable_)
                {
                    (*dest_)(callable_);
                    delete[] callable_;
                }
            }

            /**
             * 20.9.12.2.2, function modifiers:
             */

            void swap(function& other) noexcept
            {
                std::swap(callable_, other.callable_);
                std::swap(callable_size_, other.callable_size_);
                std::swap(call_, other.call_);
                std::swap(copy_, other.copy_);
                std::swap(dest_, other.dest_);
            }

            template<class F, class A>
            void assign(F&& f, const A& a)
            {
                function{allocator_arg, a, forward<F>(f)}.swap(*this);
            }

            /**
             * 20.9.12.2.3, function capacity:
             */

            explicit operator bool() const noexcept
            {
                return callable_ != nullptr;
            }

            /**
             * 20.9.12.2.4, function invocation:
             */

            result_type operator()(Args... args) const
            {
                // TODO: throw bad_function_call if !callable_ || !call_
                if constexpr (is_same_v<R, void>)
                    (*call_)(callable_, forward<Args>(args)...);
                else
                    return (*call_)(callable_, forward<Args>(args)...);
            }

            /**
             * 20.9.12.2.5, function target access:
             */

            const type_info& target_type() const noexcept
            {
                return typeid(*callable_);
            }

            template<class T>
            T* target() noexcept
            {
                if (target_type() == typeid(T))
                    return (T*)callable_;
                else
                    return nullptr;
            }

            template<class T>
            const T* target() const noexcept
            {
                if (target_type() == typeid(T))
                    return (T*)callable_;
                else
                    return nullptr;
            }

        private:
            using call_t = R(*)(uint8_t*, Args&&...);
            using copy_t = void (*)(uint8_t*, uint8_t*);
            using dest_t = void (*)(uint8_t*);

            uint8_t* callable_;
            size_t callable_size_;
            call_t call_;
            copy_t copy_;
            dest_t dest_;

            void clear_()
            {
                if (callable_)
                {
                    (*dest_)(callable_);
                    delete[] callable_;
                    callable_ = nullptr;
                }
            }
    };

    /**
     * 20.9.12.2.6, null pointer comparisons:
     */

    template<class R, class... Args>
    bool operator==(const function<R(Args...)>& f, nullptr_t) noexcept
    {
        return !f;
    }

    template<class R, class... Args>
    bool operator==(nullptr_t, const function<R(Args...)>& f) noexcept
    {
        return !f;
    }

    template<class R, class... Args>
    bool operator!=(const function<R(Args...)>& f, nullptr_t) noexcept
    {
        return (bool)f;
    }

    template<class R, class... Args>
    bool operator!=(nullptr_t, const function<R(Args...)>& f) noexcept
    {
        return (bool)f;
    }

    /**
     * 20.9.12.2.7, specialized algorithms:
     */

    template<class R, class... Args>
    void swap(function<R(Args...)>& f1, function<R(Args...)>& f2)
    {
        f1.swap(f2);
    }

    template<class R, class... Args, class Alloc>
    struct uses_allocator<function<R(Args...)>, Alloc>
        : true_type
    { /* DUMMY BODY */ };

    /**
     * 20.9.10, bind:
     */

    namespace aux
    {
        template<int N>
        struct placeholder_t
        {
            constexpr placeholder_t() = default;
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
        template<size_t I>
        struct bind_arg_index
        { /* DUMMY BODY */ };

        template<class... Args>
        class bind_bound_args
        {
            public:
                template<class... BoundArgs>
                constexpr bind_bound_args(BoundArgs&&... args)
                    : tpl_{forward<BoundArgs>(args)...}
                { /* DUMMY BODY */ }

                template<size_t I>
                constexpr decltype(auto) operator[](bind_arg_index<I>)
                {
                    return get<I>(tpl_);
                }

            private:
                tuple<Args...> tpl_;
        };

        template<class... Args>
        class bind_arg_filter
        {
            public:
                bind_arg_filter(Args&&... args)
                    : args_{forward<Args>(args)...}
                { /* DUMMY BODY */ }

                // TODO: enable if T != ref_wrapper
                template<class T>
                constexpr decltype(auto) operator[](T&& t)
                { // Since placeholders are constexpr, this is a worse match for them.
                    return forward<T>(t);
                }

                template<int N>
                constexpr decltype(auto) operator[](const placeholder_t<N>)
                {
                    /**
                     * Come on, it's int! Why not use -1 as not placeholder
                     * and start them at 0? -.-
                     */
                    /* return get<is_placeholder_v<decay_t<T>> - 1>(args_); */
                    return get<N - 1>(args_);
                }

                // TODO: overload the operator for reference_wrapper

            private:
                tuple<Args...> args_;
        };

        template<class F, class... Args>
        class bind_t
        {
            // TODO: conditional typedefs
            public:
                template<class... BoundArgs>
                constexpr bind_t(F&& f, BoundArgs&&... args)
                    : func_{forward<F>(f)},
                      bound_args_{forward<BoundArgs>(args)...}
                { /* DUMMY BODY */ }

                template<class... ActualArgs>
                constexpr decltype(auto) operator()(ActualArgs&&... args)
                {
                    return invoke_(
                        make_index_sequence<sizeof...(Args)>{},
                        forward<ActualArgs>(args)...
                    );
                }

            private:
                function<decay_t<F>> func_;
                bind_bound_args<Args...> bound_args_;

                template<size_t... Is, class... ActualArgs>
                constexpr decltype(auto) invoke_(
                    index_sequence<Is...>, ActualArgs&&... args
                )
                {
                    bind_arg_filter<ActualArgs...> filter{forward<ActualArgs>(args)...};

                    return invoke(
                        func_,
                        filter[bound_args_[bind_arg_index<Is>()]]...
                    );
                }
        };
    }

    template<class T>
    struct is_bind_expression: false_type
    { /* DUMMY BODY */ };

    template<class F, class... Args>
    struct is_bind_expression<aux::bind_t<F, Args...>>
        : true_type
    { /* DUMMY BODY */ };

    template<class F, class... Args>
    aux::bind_t<F, Args...> bind(F&& f, Args&&... args)
    {
        return aux::bind_t<F, Args...>{forward<F>(f), forward<Args>(args)...};
    }

    template<class R, class F, class... Args>
    aux::bind_t<F, Args...> bind(F&& f, Args&&... args);

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

    /**
     * 20.9.11, member function adaptors:
     */

    namespace aux
    {
        template<class F>
        class mem_fn_t
        {
            // TODO: conditional typedefs
            public:
                mem_fn_t(F f)
                    : func_{f}
                { /* DUMMY BODY */ }

                template<class... Args>
                decltype(auto) operator()(Args&&... args)
                {
                    return invoke(func_, forward<Args>(args)...);
                }

            private:
                F func_;
        };
    }

    template<class R, class T>
    aux::mem_fn_t<R T::*> mem_fn(R T::* f)
    {
        return aux::mem_fn_t<R T::*>{f};
    }

    /**
     * 20.9.13, hash function primary template:
     */

    namespace aux
    {
        template<class T>
        union converter
        {
            T value;
            uint64_t converted;
        };

        template<class T>
        T hash_(uint64_t x) noexcept
        {
            /**
             * Note: std::hash is used for indexing in
             *       unordered containers, not for cryptography.
             *       Because of this, we decided to simply convert
             *       the value to uin64_t, which will help us
             *       with testing (since in order to create
             *       a collision in a multiset or multimap
             *       we simply need 2 values that congruent
             *       by the size of the table.
             */
            return static_cast<T>(x);
        }

        template<class T>
        size_t hash(T x) noexcept
        {
            static_assert(is_arithmetic_v<T> || is_pointer_v<T>,
                          "invalid type passed to aux::hash");

            converter<T> conv;
            conv.value = x;

            return hash_<size_t>(conv.converted);
        }
    }

    template<class T>
    struct hash
    { /* DUMMY BODY */ };

    template<>
    struct hash<bool>
    {
        size_t operator()(bool x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = bool;
        using result_type   = size_t;
    };

    template<>
    struct hash<char>
    {
        size_t operator()(char x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = char;
        using result_type   = size_t;
    };

    template<>
    struct hash<signed char>
    {
        size_t operator()(signed char x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = signed char;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned char>
    {
        size_t operator()(unsigned char x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned char;
        using result_type   = size_t;
    };

    template<>
    struct hash<char16_t>
    {
        size_t operator()(char16_t x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = char16_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<char32_t>
    {
        size_t operator()(char32_t x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = char32_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<wchar_t>
    {
        size_t operator()(wchar_t x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = wchar_t;
        using result_type   = size_t;
    };

    template<>
    struct hash<short>
    {
        size_t operator()(short x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = short;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned short>
    {
        size_t operator()(unsigned short x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned short;
        using result_type   = size_t;
    };

    template<>
    struct hash<int>
    {
        size_t operator()(int x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = int;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned int>
    {
        size_t operator()(unsigned int x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned int;
        using result_type   = size_t;
    };

    template<>
    struct hash<long>
    {
        size_t operator()(long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = long;
        using result_type   = size_t;
    };

    template<>
    struct hash<long long>
    {
        size_t operator()(long long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = long long;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned long>
    {
        size_t operator()(unsigned long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned long;
        using result_type   = size_t;
    };

    template<>
    struct hash<unsigned long long>
    {
        size_t operator()(unsigned long long x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = unsigned long long;
        using result_type   = size_t;
    };

    template<>
    struct hash<float>
    {
        size_t operator()(float x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = float;
        using result_type   = size_t;
    };

    template<>
    struct hash<double>
    {
        size_t operator()(double x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = double;
        using result_type   = size_t;
    };

    template<>
    struct hash<long double>
    {
        size_t operator()(long double x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = long double;
        using result_type   = size_t;
    };

    template<class T>
    struct hash<T*>
    {
        size_t operator()(T* x) const noexcept
        {
            return aux::hash(x);
        }

        using argument_type = T*;
        using result_type   = size_t;
    };
}

#endif
