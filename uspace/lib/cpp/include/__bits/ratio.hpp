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

#ifndef LIBCPP_BITS_RATIO
#define LIBCPP_BITS_RATIO

#include <cstdint>
#include <type_traits>

namespace std
{
    namespace aux
    {
        template<intmax_t A, intmax_t B>
        struct gcd
        {
            static constexpr intmax_t value = gcd<B, A % B>::value;
        };

        template<intmax_t A>
        struct gcd<A, 0>
        {
            static constexpr intmax_t value = A;
        };

        template<intmax_t A, intmax_t B>
        inline constexpr intmax_t gcd_v = gcd<A, B>::value;

        template<intmax_t A>
        struct abs
        {
            static constexpr intmax_t value = (A > 0 ? A : -A);
        };

        template<intmax_t A>
        inline constexpr intmax_t abs_v = abs<A>::value;

        template<intmax_t A>
        struct sign
        {
            static constexpr intmax_t value = (A == 0 ? 0: (A > 0 ? 1 : -1));
        };

        template<intmax_t A>
        inline constexpr intmax_t sign_v = sign<A>::value;

        // Not used here, but in <chrono>, better to keep them together.
        template<intmax_t A, intmax_t B>
        struct lcm
        {
            static constexpr intmax_t value = abs_v<A * B> / gcd_v<A, B>;
        };

        template<intmax_t A, intmax_t B>
        inline constexpr intmax_t lcm_v = lcm<A, B>::value;
    }

    /**
     * 20.11.3, class template ratio:
     */

    template<intmax_t N, intmax_t D = 1>
    class ratio
    {
        public:
            static_assert(D != 0, "ratio with denominator == 0");

            static constexpr intmax_t num = aux::sign_v<N> * aux::sign_v<D>
                                            * aux::abs_v<N> / aux::gcd_v<N, D>;

            static constexpr intmax_t den = aux::abs_v<D> / aux::gcd_v<N, D>;

            using type = ratio<num, den>;
    };

    /**
     * 20.11.4, ratio arithmetic:
     */

    template<class R1, class R2>
    using ratio_add = typename ratio<
        R1::num * R2::den + R2::num * R1::den,
        R1::den * R2::den
    >::type;

    template<class R1, class R2>
    using ratio_subtract = typename ratio<
        R1::num * R2::den - R2::num * R1::den,
        R1::den * R2::den
    >::type;

    template<class R1, class R2>
    using ratio_multiply = typename ratio<
        R1::num * R2::num,
        R1::den * R2::den
    >::type;

    template<class R1, class R2>
    using ratio_divide = typename ratio<
        R1::num * R2::den,
        R1::den * R2::num
    >::type;

    /**
     * 20.11.5, ratio comparison:
     */

    template<class R1, class R2>
    struct ratio_equal: integral_constant<
        bool, (R1::num == R2::num) && (R1::den == R2::den)
    >
    { /* DUMMY BODY */ };

    template<class R1, class R2>
    inline constexpr bool ratio_equal_v = ratio_equal<R1, R2>::value;

    template<class R1, class R2>
    struct ratio_not_equal: integral_constant<bool, !ratio_equal<R1, R2>::value>
    { /* DUMMY BODY */ };

    template<class R1, class R2>
    inline constexpr bool ratio_not_equal_v = ratio_not_equal<R1, R2>::value;

    template<class R1, class R2>
    struct ratio_less: integral_constant<
        bool, R1::num * R2::den < R2::num * R1::den
    >
    { /* DUMMY BODY */ };

    template<class R1, class R2>
    inline constexpr bool ratio_less_v = ratio_less<R1, R2>::value;

    template<class R1, class R2>
    struct ratio_less_equal: integral_constant<bool, !ratio_less<R2, R1>::value>
    { /* DUMMY BODY */ };

    template<class R1, class R2>
    inline constexpr bool ratio_less_equal_v = ratio_less_equal<R1, R2>::value;

    template<class R1, class R2>
    struct ratio_greater: integral_constant<bool, ratio_less<R2, R1>::value>
    { /* DUMMY BODY */ };

    template<class R1, class R2>
    inline constexpr bool ratio_greater_v = ratio_greater<R1, R2>::value;

    template<class R1, class R2>
    struct ratio_greater_equal: integral_constant<bool, !ratio_less<R1, R2>::value>
    { /* DUMMY BODY */ };

    template<class R1, class R2>
    inline constexpr bool ratio_greater_equal_v = ratio_greater_equal<R1, R2>::value;

    /**
     * 20.11.6, convenience SI typedefs:
     */

    // TODO: yocto/zepto and yotta/zetta should not be defined if intmax_t is too small

    /* using yocto = ratio<1, 1'000'000'000'000'000'000'000'000>; */
    /* using zepto = ratio<1,     1'000'000'000'000'000'000'000>; */
    using atto  = ratio<1,         1'000'000'000'000'000'000>;
    using femto = ratio<1,             1'000'000'000'000'000>;
    using pico  = ratio<1,                 1'000'000'000'000>;
    using nano  = ratio<1,                     1'000'000'000>;
    using micro = ratio<1,                         1'000'000>;
    using milli = ratio<1,                             1'000>;
    using centi = ratio<1,                               100>;
    using deci  = ratio<1,                                10>;
    using deca  = ratio<                               10, 1>;
    using hecto = ratio<                              100, 1>;
    using kilo  = ratio<                            1'000, 1>;
    using mega  = ratio<                        1'000'000, 1>;
    using giga  = ratio<                    1'000'000'000, 1>;
    using tera  = ratio<                1'000'000'000'000, 1>;
    using peta  = ratio<            1'000'000'000'000'000, 1>;
    using exa   = ratio<        1'000'000'000'000'000'000, 1>;
    /* using zetta = ratio<    1'000'000'000'000'000'000'000, 1>; */
    /* using yotta = ratio<1'000'000'000'000'000'000'000'000, 1>; */
}

#endif
