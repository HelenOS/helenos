/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
