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

#ifndef LIBCPP_BITS_FUNCTIONAL_REFERENCE_WRAPPER
#define LIBCPP_BITS_FUNCTIONAL_REFERENCE_WRAPPER

#include <__bits/functional/conditional_function_typedefs.hpp>
#include <__bits/functional/invoke.hpp>
#include <type_traits>

namespace std
{
    /**
     * 20.9.4, reference_wrapper:
     */

    template<class T>
    class reference_wrapper
        : public aux::conditional_function_typedefs<remove_cv_t<remove_reference_t<T>>>
    {
        public:
            using type = T;

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
            decltype(auto) operator()(Args&&... args) const
            {
                return aux::INVOKE(*data_, std::forward<Args>(args)...);
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
}

#endif
