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

#ifndef LIBCPP_BITS_CHRONO
#define LIBCPP_BITS_CHRONO

#include <ctime>
#include <limits>
#include <ratio>
#include <type_traits>

namespace std::chrono
{
    /**
     * 20.12.5, class template duration:
     */

    // Forward declarations.
    template<class Rep, class Period = ratio<1>>
    class duration;

    template<class ToDuration, class Rep, class Period>
    constexpr ToDuration duration_cast(const duration<Rep, Period>& dur);

    template<class Rep>
    struct duration_values;

    template<class Rep, class Period>
    class duration
    {
        public:
            using rep    = Rep;
            using period = Period;

            /**
             * 20.12.5.1, construct/copy/destroy:
             */

            constexpr duration() = default;

            // TODO: check remarks to these two constructors, need is_convertible
            template<class Rep2>
            constexpr explicit duration(const Rep2& r)
                : rep_{r}
            { /* DUMMY BODY */ }

            template<class Rep2, class Period2>
            constexpr duration(const duration<Rep2, Period2>& other)
                : rep_{duration_cast<duration>(other).count()}
            { /* DUMMY BODY */ }

            ~duration() = default;

            duration(const duration&) = default;

            duration& operator=(const duration&) = default;

            /**
             * 20.12.5.2, observer:
             */

            constexpr rep count() const
            {
                return rep_;
            }

            /**
             * 20.12.5.3, arithmetic:
             */

            constexpr duration operator+() const
            {
                return *this;
            }

            constexpr duration operator-() const
            {
                return duration{-rep_};
            }

            duration& operator++()
            {
                ++rep_;

                return *this;
            }

            duration operator++(int)
            {
                return duration{rep_++};
            }

            duration& operator--()
            {
                --rep_;

                *this;
            }

            duration operator--(int)
            {
                return duration{rep_--};
            }

            duration& operator+=(const duration& rhs)
            {
                rep_ += rhs.count();

                return *this;
            }

            duration& operator-=(const duration& rhs)
            {
                rep_ -= rhs.count();

                return *this;
            }

            duration& operator*=(const rep& rhs)
            {
                rep_ *= rhs;

                return *this;
            }

            duration& operator/=(const rep& rhs)
            {
                rep_ /= rhs;

                return *this;
            }

            duration& operator%=(const rep& rhs)
            {
                rep_ %= rhs;

                return *this;
            }

            duration& operator%=(const duration& rhs)
            {
                rep_ %= rhs.count();

                return *this;
            }

            /**
             * 20.12.5.4, special values:
             */

            static constexpr duration zero()
            {
                return duration{duration_values<rep>::zero()};
            }

            static constexpr duration min()
            {
                return duration{duration_values<rep>::min()};
            }

            static constexpr duration max()
            {
                return duration{duration_values<rep>::max()};
            }

        private:
            rep rep_;
    };

    /**
     * 20.12.6, class template time_point:
     */

    template<class Clock, class Duration = typename Clock::duration>
    class time_point
    {
        public:
            using clock    = Clock;
            using duration = Duration;
            using rep      = typename duration::rep;
            using period   = typename duration::period;

            /**
             * 20.12.6.1, construct:
             */

            constexpr time_point()
                : duration_{duration::zero()}
            { /* DUMMY BODY */ }

            constexpr explicit time_point(const duration& d)
                : duration_{d}
            { /* DUMMY BODY */ }

            // TODO: see remark to this constuctor
            template<class Duration2>
            constexpr time_point(const time_point<clock, Duration2>& other)
                : duration_{static_cast<duration>(other.time_since_epoch())}
            { /* DUMMY BODY */ }

            /**
             * 20.12.6.2, observer:
             */

            constexpr duration time_since_epoch() const
            {
                return duration_;
            }

            /**
             * 20.12.6.3, arithmetic:
             */

            time_point& operator+=(const duration& rhs)
            {
                duration_ += rhs;

                return *this;
            }

            time_point& operator-=(const duration& rhs)
            {
                duration_ -= rhs;

                return *this;
            }

            /**
             * 20.12.6.4, special values:
             */

            static constexpr time_point min()
            {
                return time_point{duration::min()};
            }

            static constexpr time_point max()
            {
                return time_point{duration::max()};
            }

        private:
            duration duration_;
    };
}

namespace std
{
    /**
     * 20.12.4.3, common_type specializations:
     */

    template<class Rep1, class Period1, class Rep2, class Period2>
    struct common_type<chrono::duration<Rep1, Period1>, chrono::duration<Rep2, Period2>>
    {
        using type = chrono::duration<
            common_type_t<Rep1, Rep2>,
            ratio<aux::gcd_v<Period1::num, Period2::num>, aux::lcm_v<Period1::den, Period2::den>>
        >;
    };

    template<class Clock, class Duration1, class Duration2>
    struct common_type<chrono::time_point<Clock, Duration1>, chrono::time_point<Clock, Duration2>>
    {
        using type = chrono::time_point<Clock, common_type_t<Duration1, Duration2>>;
    };
}

namespace std::chrono
{
    /**
     * 20.12.4, customization traits:
     */

    template<class Rep>
    struct treat_as_floating_point: is_floating_point<Rep>
    { /* DUMMY BODY */ };

    template<class Rep>
    struct duration_values
    {
        static constexpr Rep zero()
        {
            // Note: Using Rep(0) instead of Rep{} is intentional, do not change.
            return Rep(0);
        }

        static constexpr Rep min()
        {
            return numeric_limits<Rep>::lowest();
        }

        static constexpr Rep max()
        {
            return numeric_limits<Rep>::max();
        }
    };

    /**
     * 20.12.5.5, duration arithmetic:
     */

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
    operator+(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
    {
        using CD = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
        return CD(CD(lhs).count() + CD(rhs).count());
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
    operator-(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
    {
        using CD = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
        return CD(CD(lhs).count() - CD(rhs).count());
    }

    // TODO: This shall not participate in overloading if Rep2 is not implicitly
    //       convertible to CR(Rep1, Rep2) -> CR(A, B) defined in standard as
    //       common_type_t<A, B>.
    template<class Rep1, class Period, class Rep2>
    constexpr duration<common_type_t<Rep1, Rep2>, Period>
    operator*(const duration<Rep1, Period>& dur, const Rep2& rep)
    {
        using CD = duration<common_type_t<Rep1, Rep2>, Period>;
        return CD(CD(dur).count() * rep);
    }

    // TODO: This shall not participate in overloading if Rep2 is not implicitly
    //       convertible to CR(Rep1, Rep2) -> CR(A, B) defined in standard as
    //       common_type_t<A, B>.
    template<class Rep1, class Period, class Rep2>
    constexpr duration<common_type_t<Rep1, Rep2>, Period>
    operator*(const Rep2& rep, const duration<Rep1, Period>& dur)
    {
        return dur * rep;
    }

    // TODO: This shall not participate in overloading if Rep2 is not implicitly
    //       convertible to CR(Rep1, Rep2) -> CR(A, B) defined in standard as
    //       common_type_t<A, B>.
    template<class Rep1, class Period, class Rep2>
    constexpr duration<common_type_t<Rep1, Rep2>, Period>
    operator/(const duration<Rep1, Period>& dur, const Rep2& rep)
    {
        using CD = duration<common_type_t<Rep1, Rep2>, Period>;
        return CD(CD(dur).count() / rep);
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr common_type_t<Rep1, Rep2>
    operator/(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
    {
        using CD = common_type_t<Rep1, Rep2>;
        return CD(lhs).count() / CD(rhs).count();
    }

    // TODO: This shall not participate in overloading if Rep2 is not implicitly
    //       convertible to CR(Rep1, Rep2) -> CR(A, B) defined in standard as
    //       common_type_t<A, B>.
    template<class Rep1, class Period, class Rep2>
    constexpr duration<common_type_t<Rep1, Rep2>, Period>
    operator%(const duration<Rep1, Period>& dur, const Rep2& rep)
    {
        using CD = duration<common_type_t<Rep1, Rep2>, Period>;
        return CD(CD(dur).count() / rep);
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
    operator%(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
    {
        using CD = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
        return CD(CD(lhs).count() % CD(rhs).count());
    }

    /**
     * 20.12.5.6, duration comparisons:
     */

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr bool operator==(const duration<Rep1, Period1>& lhs,
                              const duration<Rep2, Period2>& rhs)
    {
        using CT = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
        return CT(lhs).count() == CT(rhs).count();
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr bool operator!=(const duration<Rep1, Period1>& lhs,
                              const duration<Rep2, Period2>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr bool operator<(const duration<Rep1, Period1>& lhs,
                             const duration<Rep2, Period2>& rhs)
    {
        using CT = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
        return CT(lhs).count() < CT(rhs).count();
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr bool operator<=(const duration<Rep1, Period1>& lhs,
                              const duration<Rep2, Period2>& rhs)
    {
        return !(rhs < lhs);
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr bool operator>(const duration<Rep1, Period1>& lhs,
                             const duration<Rep2, Period2>& rhs)
    {
        return rhs < lhs;
    }

    template<class Rep1, class Period1, class Rep2, class Period2>
    constexpr bool operator>=(const duration<Rep1, Period1>& lhs,
                              const duration<Rep2, Period2>& rhs)
    {
        return !(lhs < rhs);
    }

    /**
     * 20.12.5.7, duration cast:
     */

    // TODO: This function should not participate in overloading
    //       unless ToDuration is an instantiation of duration.
    template<class ToDuration, class Rep, class Period>
    constexpr ToDuration duration_cast(const duration<Rep, Period>& dur)
    {
        using CF = ratio_divide<Period, typename ToDuration::period>;
        using CR = typename common_type<typename ToDuration::rep, Rep, intmax_t>::type;

        using to_rep = typename ToDuration::rep;

        if constexpr (CF::num == 1 && CF::den == 1)
            return ToDuration(static_cast<to_rep>(dur.count()));
        else if constexpr (CF::num != 1 && CF::den == 1)
        {
            return ToDuration(
                static_cast<to_rep>(
                    static_cast<CR>(dur.count()) * static_cast<CR>(CF::num)
                )
            );
        }
        else if constexpr (CF::num == 1 && CF::den != 1)
        {
            return ToDuration(
                static_cast<to_rep>(
                    static_cast<CR>(dur.count()) / static_cast<CR>(CF::den)
                )
            );
        }
        else
        {
            return ToDuration(
                static_cast<to_rep>(
                    static_cast<CR>(dur.count()) * static_cast<CR>(CF::num)
                    / static_cast<CR>(CF::den)
                )
            );
        }
    }

    // convenience typedefs
    using nanoseconds  = duration<int64_t, nano>;
    using microseconds = duration<int64_t, micro>;
    using milliseconds = duration<int64_t, milli>;
    using seconds      = duration<int64_t>;
    using minutes      = duration<int32_t, ratio<60>>;
    using hours        = duration<int32_t, ratio<3600>>;

    /**
     * 20.12.6.5, time_point arithmetic:
     */

    template<class Clock, class Duration1, class Rep2, class Period2>
    constexpr time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2>>>
    operator+(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs)
    {
        using CT = time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2>>>;
        return CT(lhs.time_since_epoch() + rhs);
    }

    template<class Rep1, class Period1, class Clock, class Duration2>
    constexpr time_point<Clock, common_type_t<duration<Rep1, Period1>, Duration2>>
    operator+(const duration<Rep1, Period1>& lhs, const time_point<Clock, Duration2>& rhs)
    {
        return rhs + lhs;
    }

    template<class Clock, class Duration1, class Rep2, class Period2>
    constexpr time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2>>>
    operator-(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs)
    {
        return lhs + (-rhs);
    }

    template<class Clock, class Duration1, class Duration2>
    constexpr common_type_t<Duration1, Duration2>
    operator-(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)
    {
        return lhs.time_since_epoch() - rhs.time_since_epoch();
    }

    /**
     * 20.12.6.6, time_point comparisons:
     */

    template<class Clock, class Duration1, class Duration2>
    constexpr bool operator==(const time_point<Clock, Duration1>& lhs,
                              const time_point<Clock, Duration2>& rhs)
    {
        return lhs.time_since_epoch() == rhs.time_since_epoch();
    }

    template<class Clock, class Duration1, class Duration2>
    constexpr bool operator!=(const time_point<Clock, Duration1>& lhs,
                              const time_point<Clock, Duration2>& rhs)
    {
        return !(lhs == rhs);
    }

    template<class Clock, class Duration1, class Duration2>
    constexpr bool operator<(const time_point<Clock, Duration1>& lhs,
                             const time_point<Clock, Duration2>& rhs)
    {
        return lhs.time_since_epoch() < rhs.time_since_epoch();
    }

    template<class Clock, class Duration1, class Duration2>
    constexpr bool operator<=(const time_point<Clock, Duration1>& lhs,
                              const time_point<Clock, Duration2>& rhs)
    {
        return !(rhs < lhs);
    }

    template<class Clock, class Duration1, class Duration2>
    constexpr bool operator>(const time_point<Clock, Duration1>& lhs,
                             const time_point<Clock, Duration2>& rhs)
    {
        return rhs < lhs;
    }

    template<class Clock, class Duration1, class Duration2>
    constexpr bool operator>=(const time_point<Clock, Duration1>& lhs,
                              const time_point<Clock, Duration2>& rhs)
    {
        return !(lhs < rhs);
    }

    /**
     * 20.12.6.7, time_point cast:
     */

    // TODO: This function should not participate in overloading
    //       unless ToDuration is an instantiation of duration.
    template<class ToDuration, class Clock, class Duration>
    constexpr time_point<Clock, ToDuration>
    time_point_cast(const time_point<Clock, Duration>& tp)
    {
        return time_point<Clock, ToDuration>(
            duration_cast<ToDuration>(tp.time_since_epoch())
        );
    }

    /**
     * 20.12.7, clocks:
     */

    class system_clock
    {
        public:
            using rep        = int64_t;
            using period     = micro;
            using duration   = chrono::duration<rep, period>;
            using time_point = chrono::time_point<system_clock>;

            // TODO: is it steady?
            static constexpr bool is_steady = true;

            static time_point now()
            {
                ::std::timespec ts{};
                ::helenos::getrealtime(&ts);

                rep time = NSEC2USEC(ts.tv_nsec);
                time += (ts.tv_sec * 1'000'000ul);

                return time_point{duration{time - epoch_usecs}};
            }

            static time_t to_time_t(const time_point& tp)
            {
                /* auto dur = tp.time_since_epoch(); */
                /* auto time_t_dur = duration_cast<chrono::duration<time_t, seconds>>(dur); */

                /* return time_t{time_t_dur.count()}; */
                return time_t{};
            }

            static time_point from_time_t(time_t tt)
            {
                /* auto time_t_dur = chrono::duration<time_t, seconds>{tt}; */
                /* auto dur = duration_cast<duration>(time_t_dur); */

                /* return time_point{duration_cast<duration>(chrono::duration<time_t, seconds>{tt})}; */
                return time_point{};
            }

        private:
            static constexpr rep epoch_usecs{11644473600ul * 1'000'000ul};
    };

    class steady_clock
    {
        public:
            using rep        = int64_t;
            using period     = micro;
            using duration   = chrono::duration<rep, period>;
            using time_point = chrono::time_point<steady_clock>;

            static constexpr bool is_steady = true;

            static time_point now()
            {
                ::std::timespec ts{};
                ::helenos::getuptime(&ts);

                rep time = NSEC2USEC(ts.tv_nsec);
                time += (ts.tv_sec * 1'000'000ul);

                return time_point{duration{time}};
            }
    };

    using high_resolution_clock = system_clock;
}

namespace std
{
    inline namespace literals
    {
        inline namespace chrono_literals
        {
            /**
             * 20.125.8, suffixes for duration literals:
             */

            /**
             * Note: According to the standard, literal suffixes that do not
             *       start with an underscore are reserved for future standardization,
             *       but since we are implementing the standard, we're going to ignore it.
             *       This should work (according to their documentation) work for clang,
             *       but that should be tested.
             */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"

            constexpr chrono::hours operator ""h(unsigned long long hrs)
            {
                return chrono::hours{static_cast<chrono::hours::rep>(hrs)};
            }

            constexpr chrono::duration<long double, ratio<3600>> operator ""h(long double hrs)
            {
                return chrono::duration<long double, ratio<3600>>{hrs};
            }

            constexpr chrono::minutes operator ""m(unsigned long long mins)
            {
                return chrono::minutes{static_cast<chrono::minutes::rep>(mins)};
            }

            constexpr chrono::duration<long double, ratio<60>> operator ""m(long double mins)
            {
                return chrono::duration<long double, ratio<60>>{mins};
            }

            constexpr chrono::seconds operator ""s(unsigned long long secs)
            {
                return chrono::seconds{static_cast<chrono::seconds::rep>(secs)};
            }

            constexpr chrono::duration<long double, ratio<1>> operator ""s(long double secs)
            {
                return chrono::duration<long double, ratio<1>>{secs};
            }

            constexpr chrono::milliseconds operator ""ms(unsigned long long msecs)
            {
                return chrono::milliseconds{static_cast<chrono::milliseconds::rep>(msecs)};
            }

            constexpr chrono::duration<long double, milli> operator ""ms(long double msecs)
            {
                return chrono::duration<long double, milli>{msecs};
            }

            constexpr chrono::microseconds operator ""us(unsigned long long usecs)
            {
                return chrono::microseconds{static_cast<chrono::microseconds::rep>(usecs)};
            }

            constexpr chrono::duration<long double, micro> operator ""us(long double usecs)
            {
                return chrono::duration<long double, micro>{usecs};
            }

            constexpr chrono::nanoseconds operator ""ns(unsigned long long nsecs)
            {
                return chrono::nanoseconds{static_cast<chrono::nanoseconds::rep>(nsecs)};
            }

            constexpr chrono::duration<long double, nano> operator ""ns(long double nsecs)
            {
                return chrono::duration<long double, nano>{nsecs};
            }

#pragma GCC diagnostic pop
        }
    }

    namespace chrono
    {
        using namespace literals::chrono_literals;
    }
}

#endif
