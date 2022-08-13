/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_FUNCTIONAL
#define LIBCPP_BITS_FUNCTIONAL

#include <__bits/functional/conditional_function_typedefs.hpp>
#include <__bits/functional/invoke.hpp>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace std
{
    /**
     * 20.9.3, invoke:
     */

    template<class F, class... Args>
    decltype(auto) invoke(F&& f, Args&&... args)
    {
        return aux::INVOKE(forward<F>(f),forward<Args>(args)...);
    }

    /**
     * 20.9.11, member function adaptors:
     */

    namespace aux
    {
        template<class F>
        class mem_fn_t
            : public conditional_function_typedefs<remove_cv_t<remove_reference_t<F>>>
        {
            public:
                mem_fn_t(F f)
                    : func_{f}
                { /* DUMMY BODY */ }

                template<class... Args>
                decltype(auto) operator()(Args&&... args)
                {
                    return INVOKE(func_, forward<Args>(args)...);
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
}

#endif
