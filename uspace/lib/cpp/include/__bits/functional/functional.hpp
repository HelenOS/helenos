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
