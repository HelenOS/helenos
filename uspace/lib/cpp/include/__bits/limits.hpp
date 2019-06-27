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

#ifndef LIBCPP_BITS_LIMITS
#define LIBCPP_BITS_LIMITS

#include <cstdint>
#include <climits>

namespace std
{

    /**
     * 18.3.2.5, type float_round_style:
     */

    enum float_round_style
    {
        round_indeterminate       = -1,
        round_toward_zero         =  0,
        round_to_nearest          =  1,
        round_toward_infinity     =  2,
        round_toward_neg_infinity =  3
    };

    /**
     * 18.3.2.6, type float_denorm_style:
     */

    enum float_denorm_style
    {
        denorm_indeterminate = -1,
        denorm_absent        =  0,
        denorm_present       =  1
    };

    /**
     * 18.3.2.3, class template numeric_limits:
     */

    namespace aux
    {
        template<class T>
        class numeric_limits
        {
            public:
                static constexpr bool is_specialized = false;

                static constexpr T min() noexcept
                {
                    return T{};
                }

                static constexpr T max() noexcept
                {
                    return T{};
                }

                static constexpr T lowest() noexcept
                {
                    return T{};
                }

                static constexpr int digits       = 0;
                static constexpr int digits10     = 0;
                static constexpr int max_digits10 = 0;

                static constexpr bool is_signed  = false;
                static constexpr bool is_integer = false;
                static constexpr bool is_exact   = false;

                static constexpr int radix = 0;

                static constexpr T epsilon() noexcept
                {
                    return T{};
                }

                static constexpr T round_error() noexcept
                {
                    return T{};
                }

                static constexpr int min_exponent   = 0;
                static constexpr int min_exponent10 = 0;
                static constexpr int max_exponent   = 0;
                static constexpr int max_exponent10 = 0;

                static constexpr bool has_infinity      = false;
                static constexpr bool has_quiet_NaN     = false;
                static constexpr bool has_signaling_NaN = false;

                static constexpr float_denorm_style has_denorm = denorm_absent;
                static constexpr bool has_denorm_loss          = false;

                static constexpr T infinity() noexcept
                {
                    return T{};
                }

                static constexpr T quiet_NaN() noexcept
                {
                    return T{};
                }

                static constexpr T signaling_NaN() noexcept
                {
                    return T{};
                }

                static constexpr T denorm_min() noexcept
                {
                    return T{};
                }

                static constexpr bool is_iec559  = false;
                static constexpr bool is_bounded = false;
                static constexpr bool is_modulo  = false;

                static constexpr bool traps           = false;
                static constexpr bool tinyness_before = false;

                static constexpr float_round_style round_style = round_toward_zero;
        };
    }

    template<class T>
    class numeric_limits: public aux::numeric_limits<T>
    { /* DUMMY BODY */ };

    template<class T>
    class numeric_limits<const T>: public numeric_limits<T>
    { /* DUMMY BODY */ };

    template<class T>
    class numeric_limits<volatile T>: public numeric_limits<T>
    { /* DUMMY BODY */ };

    template<class T>
    class numeric_limits<const volatile T>: public numeric_limits<T>
    { /* DUMMY BODY */ };

    /**
     * 18.3.2.3, class template numeric_limits:
     */

    template<>
    class numeric_limits<float>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr float min() noexcept
            {
                return 1.17549435e-38f;
            }

            static constexpr float max() noexcept
            {
                return 3.40282347e+38f;
            }

            static constexpr float lowest() noexcept
            {
                return -3.40282347e+38f;
            }

            static constexpr int digits       = 24;
            static constexpr int digits10     =  6;
            static constexpr int max_digits10 =  9;

            static constexpr bool is_signed  = true;
            static constexpr bool is_integer = false;
            static constexpr bool is_exact   = false;

            static constexpr int radix = 2;

            static constexpr float epsilon() noexcept
            {
                return 1.19209290e-07f;
            }

            static constexpr float round_error() noexcept
            {
                return 0.5f;
            }

            static constexpr int min_exponent   = -127;
            static constexpr int min_exponent10 =  -37;
            static constexpr int max_exponent   =  128;
            static constexpr int max_exponent10 =   38;

            static constexpr bool has_infinity      = true;
            static constexpr bool has_quiet_NaN     = true;
            static constexpr bool has_signaling_NaN = true;

            static constexpr float_denorm_style has_denorm = denorm_absent;
            static constexpr bool has_denorm_loss          = false;

            static constexpr float infinity() noexcept
            {
                // TODO: implement
                return 0.f;
            }

            static constexpr float quiet_NaN() noexcept
            {
                // TODO: implement
                return 0.f;
            }

            static constexpr float signaling_NaN() noexcept
            {
                // TODO: implement
                return 0.f;
            }

            static constexpr float denorm_min() noexcept
            {
                return min();
            }

            static constexpr bool is_iec559  = true;
            static constexpr bool is_bounded = true;
            static constexpr bool is_modulo  = false;

            static constexpr bool traps           = true;
            static constexpr bool tinyness_before = true;

            static constexpr float_round_style round_style = round_to_nearest;
    };

    template<>
    class numeric_limits<bool>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr bool min() noexcept
            {
                return false;
            }

            static constexpr bool max() noexcept
            {
                return true;
            }

            static constexpr bool lowest() noexcept
            {
                return false;
            }

            static constexpr int digits       = 1;
            static constexpr int digits10     = 0;
            static constexpr int max_digits10 = 0;

            static constexpr bool is_signed  = false;
            static constexpr bool is_integer = true;
            static constexpr bool is_exact   = true;

            static constexpr int radix = 2;

            static constexpr bool epsilon() noexcept
            {
                return 0;
            }

            static constexpr bool round_error() noexcept
            {
                return 0;
            }

            static constexpr int min_exponent   = 0;
            static constexpr int min_exponent10 = 0;
            static constexpr int max_exponent   = 0;
            static constexpr int max_exponent10 = 0;

            static constexpr bool has_infinity      = false;
            static constexpr bool has_quiet_NaN     = false;
            static constexpr bool has_signaling_NaN = false;

            static constexpr float_denorm_style has_denorm = denorm_absent;
            static constexpr bool has_denorm_loss          = false;

            static constexpr bool infinity() noexcept
            {
                return 0;
            }

            static constexpr bool quiet_NaN() noexcept
            {
                return 0;
            }

            static constexpr bool signaling_NaN() noexcept
            {
                return 0;
            }

            static constexpr bool denorm_min() noexcept
            {
                return 0;
            }

            static constexpr bool is_iec559  = false;
            static constexpr bool is_bounded = true;
            static constexpr bool is_modulo  = false;

            static constexpr bool traps           = false;
            static constexpr bool tinyness_before = false;

            static constexpr float_round_style round_style = round_toward_zero;
    };

    /**
     * Note: Because of the limited state of limit.h, we define only
     *       the most basic properties of the most basic types (that are likely
     *       to be used in a program that is being ported to HelenOS).
     */

    template<>
    class numeric_limits<char>: public aux::numeric_limits<char>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(char) * 8;

            static constexpr char max()
            {
                return CHAR_MAX;
            }

            static constexpr char min()
            {
                return CHAR_MIN;
            }

            static constexpr char lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<signed char>: public aux::numeric_limits<signed char>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(signed char) * 8 - 1;

            static constexpr signed char max()
            {
                return SCHAR_MAX;
            }

            static constexpr signed char min()
            {
                return SCHAR_MIN;
            }

            static constexpr signed char lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<short>: public aux::numeric_limits<short>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(short) * 8 - 1;

            static constexpr short max()
            {
                return SHRT_MAX;
            }

            static constexpr short min()
            {
                return SHRT_MIN;
            }

            static constexpr short lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<int>: public aux::numeric_limits<int>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(int) * 8 - 1;

            static constexpr int max()
            {
                return INT_MAX;
            }

            static constexpr int min()
            {
                return INT_MIN;
            }

            static constexpr int lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<long>: public aux::numeric_limits<long>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(long) * 8 - 1;

            static constexpr long max()
            {
                return LONG_MAX;
            }

            static constexpr long min()
            {
                return LONG_MIN;
            }

            static constexpr long lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<long long>: public aux::numeric_limits<long long>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(long long) * 8 - 1;

            static constexpr long long max()
            {
                return LLONG_MAX;
            }

            static constexpr long long min()
            {
                return LLONG_MIN;
            }

            static constexpr long long lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<unsigned char>: public aux::numeric_limits<unsigned char>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(unsigned char) * 8;

            static constexpr unsigned char max()
            {
                return SCHAR_MAX;
            }

            static constexpr unsigned char min()
            {
                return SCHAR_MIN;
            }

            static constexpr unsigned char lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<unsigned short>: public aux::numeric_limits<unsigned short>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(unsigned short) * 8;

            static constexpr unsigned short max()
            {
                return USHRT_MAX;
            }

            static constexpr unsigned short min()
            {
                return 0;
            }

            static constexpr unsigned short lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<unsigned int>: public aux::numeric_limits<unsigned int>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(unsigned int) * 8;

            static constexpr unsigned int max()
            {
                return UINT_MAX;
            }

            static constexpr unsigned int min()
            {
                return 0;
            }

            static constexpr unsigned int lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<unsigned long>: public aux::numeric_limits<unsigned long>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(unsigned long) * 8;

            static constexpr unsigned long max()
            {
                return ULONG_MAX;
            }

            static constexpr unsigned long min()
            {
                return 0;
            }

            static constexpr unsigned long lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<unsigned long long>: public aux::numeric_limits<unsigned long long>
    {
        public:
            static constexpr bool is_specialized = true;

            static constexpr int digits = sizeof(unsigned long long) * 8;

            static constexpr unsigned long long max()
            {
                return ULLONG_MAX;
            }

            static constexpr unsigned long long min()
            {
                return 0;
            }

            static constexpr unsigned long long lowest()
            {
                return min();
            }
    };

    template<>
    class numeric_limits<double>: public aux::numeric_limits<double>
    {
        public:
        // TODO: implement
            static constexpr int digits = sizeof(short) * 8 - 1;
    };

    template<>
    class numeric_limits<long double>: public aux::numeric_limits<long double>
    {
        public:
        // TODO: implement
    };
}

#endif
