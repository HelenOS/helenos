/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_AUX
#define LIBCPP_BITS_AUX

namespace std
{
    /**
     * 20.10.3, helper class:
     */

    template<class T, T v>
    struct integral_constant
    {
        static constexpr T value = v;

        using value_type = T;
        using type       = integral_constant<T, v>;

        constexpr operator value_type() const noexcept
        {
            return value;
        }

        constexpr value_type operator()() const noexcept
        {
            return value;
        }
    };

    using true_type = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;
}

namespace std::aux
{
    /**
     * Two very handy templates, this allows us
     * to easily follow the T::type and T::value
     * convention by simply inheriting from specific
     * instantiations of these templates.
     * Examples:
     *  1) We need a struct with int typedef'd to type:
     *
     *      stuct has_type_int: aux::type<int> {};
     *      typename has_type_int::type x = 1; // x is of type int
     *
     *  2) We need a struct with static size_t member called value:
     *
     *      struct has_value_size_t: aux::value<size_t, 1u> {};
     *      std::printf("%u\n", has_value_size_t::value); // prints "1\n"
     */

    template<class T>
    struct type_is
    {
        using type = T;
    };

    // For compatibility with integral_constant.
    template<class T, T v>
    using value_is = std::integral_constant<T, v>;
}

#endif
