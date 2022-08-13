/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
