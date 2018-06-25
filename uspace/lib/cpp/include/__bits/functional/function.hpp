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

#ifndef LIBCPP_BITS_FUNCTIONAL_FUNCTION
#define LIBCPP_BITS_FUNCTIONAL_FUNCTION

#include <__bits/functional/conditional_function_typedefs.hpp>
#include <__bits/functional/reference_wrapper.hpp>
#include <__bits/memory/allocator_arg.hpp>
#include <__bits/memory/allocator_traits.hpp>
#include <typeinfo>
#include <type_traits>
#include <utility>

namespace std
{
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
        : public aux::conditional_function_typedefs<Args...>
    {
        public:
            using result_type = R;

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
}

#endif
