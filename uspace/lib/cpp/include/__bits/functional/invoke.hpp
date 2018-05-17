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

#ifndef LIBCPP_BITS_FUNCTIONAL_INVOKE
#define LIBCPP_BITS_FUNCTIONAL_INVOKE

#include <__bits/utility/declval.hpp>
#include <__bits/utility/forward_move.hpp>

namespace std
{
    template<class>
    struct is_member_function_pointer;

    template<class, class>
    struct is_base_of;

    template<class>
    struct is_member_object_pointer;
}

namespace std::aux
{
    /**
     * 20.9.2, requirements:
     */

    template<class R, class T, class T1, class... Ts>
    decltype(auto) INVOKE(R T::* f, T1&& t1, Ts&&... args)
    {
        if constexpr (is_member_function_pointer<decltype(f)>::value)
        {
            if constexpr (is_base_of<T, remove_reference_t<T1>>::value)
                // (1.1)
                return (t1.*f)(forward<Ts>(args)...);
            else
                // (1.2)
                return ((*t1).*f)(forward<Ts>(args)...);
        }
        else if constexpr (is_member_object_pointer<decltype(f)>::value && sizeof...(args) == 0)
        {
            /**
             * Note: Standard requires to N be equal to 1, but we take t1 directly
             *       so we need sizeof...(args) to be 0.
             */
            if constexpr (is_base_of<T, remove_reference_t<T1>>::value)
                // (1.3)
                return t1.*f;
            else
                // (1.4)
                return (*t1).*f;
        }
        else

        /**
         * Note: If this condition holds this will not be reachable,
         *       but a new addition to the standard (17.7 point (8.1))
         *       prohibits us from simply using false as the condition here,
         *       so we use this because we know it is false here.
         */
        static_assert(is_member_function_pointer<decltype(f)>::value, "invalid invoke");
    }

    template<class F, class... Args>
    decltype(auto) INVOKE(F&& f, Args&&... args)
    {
        // (1.5)
        return f(forward<Args>(args)...);
    }
}

#endif
