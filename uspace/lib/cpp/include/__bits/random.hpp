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

#ifndef LIBCPP_BITS_RANDOM
#define LIBCPP_BITS_RANDOM

#include <__bits/builtins.hpp>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <vector>

/**
 * Note: Variables with one or two lettered
 *       names here are named after their counterparts in
 *       the standard. If one needs to understand their meaning,
 *       they should seek the mentioned standard section near
 *       the declaration of these variables.
 * Note: There will be a lot of mathematical expressions in this header.
 *       All of these are taken directly from the standard's requirements
 *       and as such won't be commented here, check the appropriate
 *       sections if you need explanation of these forumulae.
 */

namespace std
{
    namespace aux
    {
        /**
         * This is the minimum requirement imposed by the
         * standard for a type to qualify as a seed sequence
         * in overloading resolutions.
         * (This is because the engines have constructors
         * that accept sequence and seed and without this
         * minimal requirements overload resolution would fail.)
         */
        template<class Sequence, class ResultType>
        struct is_seed_sequence
            : aux::value_is<
            bool, !is_convertible_v<Sequence, ResultType>
        >
        { /* DUMMY BODY */ };

        template<class T, class Engine>
        inline constexpr bool is_seed_sequence_v = is_seed_sequence<T, Engine>::value;
    }

    /**
     * 26.5.3.1, class template linear_congruential_engine:
     */

    template<class UIntType, UIntType a, UIntType c, UIntType m>
    class linear_congruential_engine
    {
        static_assert(m == 0 || (a < m && c < m));

        public:
            using result_type = UIntType;

            static constexpr result_type multiplier = a;
            static constexpr result_type increment = c;
            static constexpr result_type modulus = m;

            static constexpr result_type min()
            {
                return c == 0U ? 1U : 0U;
            }

            static constexpr result_type max()
            {
                return m - 1U;
            }

            static constexpr result_type default_seed = 1U;

            explicit linear_congruential_engine(result_type s = default_seed)
                : state_{}
            {
                seed(s);
            }

            linear_congruential_engine(const linear_congruential_engine& other)
                : state_{other.state_}
            { /* DUMMY BODY */ }

            template<class Seq>
            explicit linear_congruential_engine(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
                : state_{}
            {
                seed(q);
            }

            void seed(result_type s = default_seed)
            {
                if (c % modulus_ == 0 && s % modulus_ == 0)
                    state_ = 1;
                else
                    state_ = s % modulus_;
            }

            template<class Seq>
            void seed(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
            {
                auto k = static_cast<size_t>(aux::ceil(aux::log2(modulus_) / 32));
                auto arr = new result_type[k + 3];

                q.generate(arr, arr + k + 3);

                result_type s{};
                for (size_t j = 0; j < k; ++j)
                    s += arr[j + 3] * aux::pow2(32U * j);
                s = s % modulus_;

                if (c % modulus_ == 0 && s == 0)
                    state_ = 1;
                else
                    state_ = s % modulus_;
                delete[] arr;
            }

            result_type operator()()
            {
                return generate_();
            }

            void discard(unsigned long long z)
            {
                for (unsigned long long i = 0ULL; i < z; ++i)
                    transition_();
            }

            bool operator==(const linear_congruential_engine& rhs) const
            {
                return state_ = rhs.state_;
            }

            bool operator!=(const linear_congruential_engine& rhs) const
            {
                return !(*this == rhs);
            }

            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                auto flags = os.flags();
                os.flags(ios_base::dec | ios_base::left);

                os << state_;

                os.flags(flags);
                return os;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                auto flags = is.flags();
                is.flags(ios_base::dec);

                result_type tmp{};
                if (is >> tmp)
                    state_ = tmp;
                else
                    is.setstate(ios::failbit);

                is.flags(flags);
                return is;
            }

        private:
            result_type state_;

            static constexpr result_type modulus_ =
                (m == 0) ? (numeric_limits<result_type>::max() + 1) : m;

            void transition_()
            {
                state_ = (a * state_ + c) % modulus_;
            }

            result_type generate_()
            {
                transition_();

                return state_;
            }
    };

    /**
     * 26.5.3.2, class template mersenne_twister_engine:
     */

    template<
        class UIntType, size_t w, size_t n, size_t m, size_t r,
        UIntType a, size_t u, UIntType d, size_t s,
        UIntType b, size_t t, UIntType c, size_t l, UIntType f
    >
    class mersenne_twister_engine
    {
        // TODO: fix these
        /* static_assert(0 < m && m <= n); */
        /* static_assert(2 * u < w); */
        /* static_assert(r <= w && u <= w && s <= w && t <= w && l <= w); */
        /* /1* static_assert(w <= numeric_limits<UIntType>::digits); *1/ */
        /* static_assert(a <= (1U << w) - 1U); */
        /* static_assert(b <= (1U << w) - 1U); */
        /* static_assert(c <= (1U << w) - 1U); */
        /* static_assert(d <= (1U << w) - 1U); */
        /* static_assert(f <= (1U << w) - 1U); */

        public:
            using result_type = UIntType;

            static constexpr size_t word_size = w;
            static constexpr size_t state_size = n;
            static constexpr size_t shift_size = m;
            static constexpr size_t mask_bits = r;
            static constexpr UIntType xor_mask = a;

            static constexpr size_t tempering_u = u;
            static constexpr UIntType tempering_d = d;
            static constexpr size_t tempering_s = s;
            static constexpr UIntType tempering_b = b;
            static constexpr size_t tempering_t = t;
            static constexpr UIntType tempering_c = c;
            static constexpr size_t tempering_l = l;

            static constexpr UIntType initialization_multiplier = f;

            static constexpr result_type min()
            {
                return result_type{};
            }

            static constexpr result_type max()
            {
                return static_cast<result_type>(aux::pow2(w)) - 1U;
            }

            static constexpr result_type default_seed = 5489U;

            explicit mersenne_twister_engine(result_type value = default_seed)
                : state_{}, i_{}
            {
                seed(value);
            }

            template<class Seq>
            explicit mersenne_twister_engine(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
                : state_{}, i_{}
            {
                seed(q);
            }

            void seed(result_type value = default_seed)
            {
                state_[idx_(-n)] = value % aux::pow2u(w);;

                for (long long i = 1 - n; i <= -1; ++i)
                {
                    state_[idx_(i)] = (f * (state_[idx_(i - 1)] ^
                                      (state_[idx_(i - 1)] >> (w - 2))) + 1 % n) % aux::pow2u(w);
                }
            }

            template<class Seq>
            void seed(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
            {
                auto k = static_cast<size_t>(w / 32);
                auto arr = new result_type[n * k];
                q.generate(arr, arr + n * k);

                for (long long i = -n; i <= -1; ++i)
                {
                    state_[idx_(i)] = result_type{};
                    for (long long j = 0; j < k; ++j)
                        state_[idx_(i)] += arr[k * (i + n) + j] * aux::pow2(32 * j);
                    state_[idx_(i)] %= aux::pow2(w);
                }

                delete[] arr;
            }

            result_type operator()()
            {
                return generate_();
            }

            void discard(unsigned long long z)
            {
                for (unsigned long long i = 0ULL; i < z; ++i)
                    transition_();
            }

            bool operator==(const mersenne_twister_engine& rhs) const
            {
                if (i_ != rhs.i_)
                    return false;

                for (size_t i = 0; i < n; ++i)
                {
                    if (state_[i] != rhs.state_[i])
                        return false;
                }

                return true;
            }

            bool operator!=(const mersenne_twister_engine& rhs) const
            {
                return !(*this == rhs);
            }

            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                auto flags = os.flags();
                os.flags(ios_base::dec | ios_base::left);

                for (size_t j = n + 1; j > 1; --j)
                {
                    os << state_[idx_(i_ - j - 1)];

                    if (j > 2)
                        os << os.widen(' ');
                }

                os.flags(flags);
                return os;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                auto flags = is.flags();
                is.flags(ios_base::dec);

                for (size_t j = n + 1; j > 1; --j)
                {
                    if (!(is >> state_[idx_(i_ - j - 1)]))
                    {
                        is.setstate(ios::failbit);
                        break;
                    }
                }

                is.flags(flags);
                return is;
            }

        private:
            result_type state_[n];
            size_t i_;

            void transition_()
            {
                auto mask = (result_type{1} << r) - 1;
                auto y = (state_[idx_(i_ - n)] & ~mask) | (state_[idx_(i_ + 1 - n)] & mask);
                auto alpha = a * (y & 1);
                state_[i_] = state_[idx_(i_ + m - n)] ^ (y >> 1) ^ alpha;

                i_ = (i_ + 1) % n;
            }

            result_type generate_()
            {
                auto z1 = state_[i_] ^ ((state_[i_] >> u) & d);
                auto z2 = z1 ^ (lshift_(z1, s) & b);
                auto z3 = z2 ^ (lshift_(z2, t) & c);
                auto z4 = z3 ^ (z3 >> l);

                transition_();

                return z4;
            }

            size_t idx_(size_t idx) const
            {
                return idx % n;
            }

            result_type lshift_(result_type val, size_t count)
            {
                return (val << count) % aux::pow2u(w);
            }
    };

    /**
     * 26.5.3.3, class template subtract_with_carry_engine:
     */

    template<class UIntType, size_t w, size_t s, size_t r>
    class subtract_with_carry_engine
    {
        // TODO: fix these
        /* static_assert(0U < s); */
        /* static_assert(s < r); */
        /* static_assert(0U < w); */
        /* static_assert(w <= numeric_limits<UIntType>::digits); */

        public:
            using result_type = UIntType;

            static constexpr size_t word_size = w;
            static constexpr size_t short_lag = s;
            static constexpr size_t long_lag = r;

            static constexpr result_type min()
            {
                return result_type{};
            }

            static constexpr result_type max()
            {
                return m_ - 1;
            }

            static constexpr result_type default_seed = 19780503U;

            explicit subtract_with_carry_engine(result_type value = default_seed)
                : state_{}, i_{}, carry_{}
            {
                seed(value);
            }

            template<class Seq>
            explicit subtract_with_carry_engine(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
                : state_{}, i_{}, carry_{}
            {
                seed(q);
            }

            void seed(result_type value = default_seed)
            {
                linear_congruential_engine<
                    result_type, 40014U, 0U, 2147483563U
                > e{value == 0U ? default_seed : value};

                auto n = aux::ceil(w / 32.0);
                auto z = new result_type[n];

                for (long long i = -r; i <= -1; ++i)
                {
                    for (size_t i = 0; i < n; ++i)
                        z[i] = e() % aux::pow2u(32);

                    state_[idx_(i)] = result_type{};
                    for (size_t j = 0; j < n; ++j)
                        state_[idx_(i)] += z[j] * aux::pow2u(32 * j);
                    state_[idx_(i)] %= m_;
                }

                if (state_[idx_(-1)] == 0)
                    carry_ = 1;
                else
                    carry_ = 0;

                delete[] z;
            }

            template<class Seq>
            void seed(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
            {
                auto k = aux::ceil(w / 32.0);
                auto arr = new result_type[r * k];

                q.generate(arr, arr + r * k);

                for (long long i = -r; i <= -1; ++i)
                {
                    state_[idx_(i)] = result_type{};
                    for (long long j = 0; j < k; ++j)
                        state_[idx_(i)] += arr[k * (i + r) + j] * aux::pow2(32 * j);
                    state_[idx_(i)] %= m_;
                }

                delete[] arr;

                if (state_[idx_(-1)] == 0)
                    carry_ = 1;
                else
                    carry_ = 0;
            }

            result_type operator()()
            {
                return generate_();
            }

            void discard(unsigned long long z)
            {
                for (unsigned long long i = 0ULL; i < z; ++i)
                    transition_();
            }

            bool operator==(const subtract_with_carry_engine& rhs) const
            {
                if (i_ != rhs.i_)
                    return false;

                for (size_t i = 0; i < r; ++i)
                {
                    if (state_[i] != rhs.state_[i])
                        return false;
                }

                return true;
            }

            bool operator!=(const subtract_with_carry_engine& rhs) const
            {
                return !(*this == rhs);
            }

            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                auto flags = os.flags();
                os.flags(ios_base::dec | ios_base::left);

                for (size_t j = r + 1; j > 1; --j)
                {
                    os << state_[idx_(i_ - j - 1)];
                    os << os.widen(' ');
                }

                os << carry_;

                os.flags(flags);
                return os;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                auto flags = is.flags();
                is.flags(ios_base::dec);

                for (size_t j = r + 1; j > 1; --j)
                {
                    if (!(is >> state_[idx_(i_ - j - 1)]))
                    {
                        is.setstate(ios::failbit);
                        break;
                    }
                }

                if (!(is >> carry_))
                    is.setstate(ios::failbit);

                is.flags(flags);
                return is;
            }

        private:
            result_type state_[r];
            size_t i_;
            uint8_t carry_;

            static constexpr result_type m_ = aux::pow2u(w);

            auto transition_()
            {
                auto y = static_cast<int64_t>(state_[idx_(i_ - s)]) - state_[idx_(i_ - r)] - carry_;
                state_[i_] = y % m_;

                i_ = (i_ + 1) % r;

                return static_cast<result_type>(y % m_);
            }

            result_type generate_()
            {
                return transition_();
            }

            size_t idx_(size_t idx) const
            {
                return idx % r;
            }
    };

    /**
     * 26.5.4.2, class template discard_block_engine:
     */

    template<class Engine, size_t p, size_t r>
    class discard_block_engine
    {
        static_assert(0 < r);
        static_assert(r <= p);

        public:
            using result_type = typename Engine::result_type;

            static constexpr size_t block_size = p;
            static constexpr size_t used_block = r;

            static constexpr result_type min()
            {
                return Engine::min();
            }

            static constexpr result_type max()
            {
                return Engine::max();
            }

            discard_block_engine()
                : engine_{}, n_{}
            { /* DUMMY BODY */ }

            explicit discard_block_engine(const Engine& e)
                : engine_{e}, n_{}
            { /* DUMMY BODY */ }

            explicit discard_block_engine(Engine&& e)
                : engine_{move(e)}, n_{}
            { /* DUMMY BODY */ }

            explicit discard_block_engine(result_type s)
                : engine_{s}, n_{}
            { /* DUMMY BODY */ }

            template<class Seq>
            explicit discard_block_engine(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
                : engine_{q}, n_{}
            { /* DUMMY BODY */ }

            void seed()
            {
                engine_.seed();
            }

            void seed(result_type s)
            {
                engine_.seed(s);
            }

            template<class Seq>
            void seed(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
            {
                engine_.seed(q);
            }

            result_type operator()()
            {
                if (n_ > static_cast<int>(r))
                {
                    auto count = p - r;
                    for (size_t i = 0; i < count; ++i)
                        engine_();
                    n_ = 0;
                }
                ++n_;

                return engine_();
            }

            void discard(unsigned long long z)
            {
                for (unsigned long long i = 0ULL; i < z; ++i)
                    operator()(); // We need to discard our (), not engine's.
            }

            const Engine& base() const noexcept
            {
                return engine_;
            }

            bool operator==(const discard_block_engine& rhs) const
            {
                return engine_ == rhs.engine_ && n_ == rhs.n_;
            }

            bool operator!=(const discard_block_engine& rhs) const
            {
                return !(*this == rhs);
            }

            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                auto flags = os.flags();
                os.flags(ios_base::dec | ios_base::left);

                os << n_ << os.widen(' ') << engine_;

                os.flags(flags);
                return os;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                auto flags = is.flags();
                is.flags(ios_base::dec);

                if (!(is >> n_) || !(is >> engine_))
                    is.setstate(ios::failbit);

                is.flags(flags);
                return is;
            }

        private:
            Engine engine_;
            int n_;
    };

    /**
     * 26.5.4.3, class template independent_bits_engine:
     */

    template<class Engine, size_t w, class UIntType>
    class independent_bits_engine
    {
        static_assert(0U < w);
        /* static_assert(w <= numeric_limits<result_type>::digits); */

        public:
            using result_type = UIntType;

            static constexpr result_type min()
            {
                return result_type{};
            }

            static constexpr result_type max()
            {
                return aux::pow2u(w) - 1;
            }

            independent_bits_engine()
                : engine_{}
            { /* DUMMY BODY */ }

            explicit independent_bits_engine(const Engine& e)
                : engine_{e}
            { /* DUMMY BODY */ }

            explicit independent_bits_engine(Engine&& e)
                : engine_{move(e)}
            { /* DUMMY BODY */ }

            explicit independent_bits_engine(result_type s)
                : engine_{s}
            { /* DUMMY BODY */ }

            template<class Seq>
            explicit independent_bits_engine(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
                : engine_{q}
            { /* DUMMY BODY */ }

            void seed()
            {
                engine_.seed();
            }

            void seed(result_type s)
            {
                engine_.seed(s);
            }

            template<class Seq>
            void seed(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
            {
                engine_.seed(q);
            }

            result_type operator()()
            {
                /* auto r = engine_.max() - engine_.min() + 1; */
                /* auto m = aux::floor(aux::log2(r)); */
                // TODO:

                return engine_();
            }

            void discard(unsigned long long z)
            {
                for (unsigned long long i = 0ULL; i < z; ++i)
                    operator()();
            }

            const Engine& base() const noexcept
            {
                return engine_;
            }

            bool operator==(const independent_bits_engine& rhs) const
            {
                return engine_ == rhs.engine_;
            }

            bool operator!=(const independent_bits_engine& rhs) const
            {
                return !(*this == rhs);
            }

            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                return os << engine_;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                return is >> engine_;
            }

        private:
            Engine engine_;
    };

    /**
     * 26.5.4.4, class template shiffle_order_engine:
     */

    template<class Engine, size_t k>
    class shuffle_order_engine
    {
        static_assert(0U < k);

        public:
            using result_type = typename Engine::result_type;

            static constexpr size_t table_size = k;

            static constexpr result_type min()
            {
                return Engine::min();
            }

            static constexpr result_type max()
            {
                return Engine::max();
            }

            shuffle_order_engine()
                : engine_{}
            { /* DUMMY BODY */ }

            explicit shuffle_order_engine(const Engine& e)
                : engine_{e}
            { /* DUMMY BODY */ }

            explicit shuffle_order_engine(Engine&& e)
                : engine_{move(e)}
            { /* DUMMY BODY */ }

            explicit shuffle_order_engine(result_type s)
                : engine_{s}
            { /* DUMMY BODY */ }

            template<class Seq>
            explicit shuffle_order_engine(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
                : engine_{q}
            { /* DUMMY BODY */ }

            void seed()
            {
                engine_.seed();
            }

            void seed(result_type s)
            {
                engine_.seed(s);
            }

            template<class Seq>
            void seed(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
            {
                engine_.seed(q);
            }

            result_type operator()()
            {
                // TODO:

                return engine_();
            }

            void discard(unsigned long long z)
            {
                for (unsigned long long i = 0ULL; i < z; ++i)
                    operator()();
            }

            const Engine& base() const noexcept
            {
                return engine_;
            }

            bool operator==(const shuffle_order_engine& rhs) const
            {
                return engine_ == rhs.engine_;
            }

            bool operator!=(const shuffle_order_engine& rhs) const
            {
                return !(*this == rhs);
            }

            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                return os << engine_;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                return is >> engine_;
            }

        private:
            Engine engine_;
            result_type y_;
            result_type table_[k];
    };

    /**
     * 26.5.5, engines and engine adaptors with predefined
     * parameters:
     * TODO: check their requirements for testing
     */

    using minstd_rand0  = linear_congruential_engine<uint_fast32_t, 16807, 0, 2147483647>;
    using minstd_rand   = linear_congruential_engine<uint_fast32_t, 48271, 0, 2147483647>;
    using mt19937       = mersenne_twister_engine<
        uint_fast32_t, 32, 624, 397, 31, 0x9908b0df, 11, 0xffffffff, 7,
        0x9d2c5680, 15, 0xefc60000, 18, 1812433253
    >;
    using mt19937_64    = mersenne_twister_engine<
        uint_fast64_t, 64, 312, 156, 31, 0xb5026f5aa96619e9, 29,
        0x5555555555555555, 17, 0x71d67fffeda60000, 37, 0xfff7eee000000000,
        43, 6364136223846793005
    >;
    using ranlux24_base = subtract_with_carry_engine<uint_fast32_t, 24, 10, 24>;
    using ranlux48_base = subtract_with_carry_engine<uint_fast64_t, 48, 5, 12>;
    using ranlux24      = discard_block_engine<ranlux24_base, 223, 23>;
    using ranlux48      = discard_block_engine<ranlux48_base, 389, 11>;
    using knuth_b       = shuffle_order_engine<minstd_rand0, 256>;

    using default_random_engine = minstd_rand0;

    /**
     * 26.5.6, class random_device:
     */

    class random_device
    {
        using result_type = unsigned int;

        static constexpr result_type min()
        {
            return numeric_limits<result_type>::min();
        }

        static constexpr result_type max()
        {
            return numeric_limits<result_type>::max();
        }

        explicit random_device(const string& token = "")
        {
            /**
             * Note: token can be used to choose between
             *       random generators, but HelenOS only
             *       has one :/
             *       Also note that it is implementation
             *       defined how this class generates
             *       random numbers and I decided to use
             *       time seeding with C stdlib random,
             *       - feel free to change it if you know
             *       something better.
             */
            ::srand(::time(nullptr));
        }

        result_type operator()()
        {
            return ::rand();
        }

        double entropy() const noexcept
        {
            return 0.0;
        }

        random_device(const random_device&) = delete;
        random_device& operator=(const random_device&) = delete;
    };

    /**
     * 26.5.7.1, class seed_seq:
     */

    class seed_seq
    {
        public:
            using result_type = uint_least32_t;

            seed_seq()
                : vec_{}
            { /* DUMMY BODY */ }

            template<class T>
            seed_seq(initializer_list<T> init)
                : seed_seq(init.begin(), init.end())
            { /* DUMMY BODY */ }

            template<class InputIterator>
            seed_seq(InputIterator first, InputIterator last)
                : vec_{}
            {
                while (first != last)
                    vec_.push_back((*first++) % aux::pow2u(32));
            }

            template<class RandomAccessGenerator>
            void generate(RandomAccessGenerator first,
                          RandomAccessGenerator last)
            {
                if (first == last)
                    return;

                auto s = vec_.size();
                size_t n = last - first;
                result_type t = (n >= 623) ? 11 : (n >= 68) ? 7 : (n >= 39) ? 5 : (n >= 7) ? 3 : (n - 1) / 2;
                result_type p = (n - t) / 2;
                result_type q = p + t;

                auto current = first;
                while (current != last)
                    *current++ = 0x8b8b8b8b;

                auto m = (s + 1 > n) ? (s + 1) : n;
                decltype(m) k{};
                for (; k < m; ++k)
                {
                    auto r1 = 1664525 * t_(first[k % n] ^ first[(k + p) % n] ^ first[(k - 1) % n]);
                    auto r2 = r1;

                    if (k == 0)
                        r2 += s;
                    else if (k > 0 && k <= s)
                        r2 += (k % n) + vec_[(k - 1) % n];
                    else if (s < k)
                        r2 += (k % n);

                    first[(k + p) % n] += r1;
                    first[(k + q) % n] += r2;
                    first[k % n] = r2;
                }

                for (; k < m + n - 1; ++k)
                {
                    auto r3 = 1566083941 * t_(first[k % n] + first[(k + p) % n] + first[(k - 1) % n]);
                    auto r4 = r3 - (k % n);

                    first[(k + p) % n] ^= r3;
                    first[(k + q) % n] ^= r4;
                    first[k % n] = r4;
                }
            }

            size_t size() const
            {
                return vec_.size();
            }

            template<class OutputIterator>
            void param(OutputIterator dest) const
            {
                for (const auto& x: vec_)
                    *dest++ = x;
            }

            seed_seq(const seed_seq&) = delete;
            seed_seq& operator=(const seed_seq&) = delete;

        private:
            vector<result_type> vec_;

            result_type t_(result_type val) const
            {
                return val ^ (val >> 27);
            }
    };

    /**
     * 26.5.7.2, function template generate_canonical:
     */

    template<class RealType, size_t bits, class URNG>
    RealType generate_canonical(URNG& g)
    {
        auto b = (bits < numeric_limits<RealType>::digits) ? bits : numeric_limits<RealType>::digits;
        RealType r = g.max() - g.min() + 1;
        size_t tmp = aux::ceil(b / aux::log2(r));
        size_t k = (1U < tmp) ? tmp : 1U;

        RealType s{};
        for (size_t i = 0; i < k; ++i)
            s += (g() - g.min()) * aux::pow(r, i);

        return s / aux::pow(r, k);
    }

    /**
     * 26.5.8.2.1, class template uniform_int_distribution:
     */

    template<class IntType = int>
    class uniform_int_distribution
    {
        public:
            using result_type = IntType;
            using param_type  = pair<result_type, result_type>;

            explicit uniform_int_distribution(result_type a = 0,
                                              result_type b = numeric_limits<result_type>::max())
                : a_{a}, b_{b}
            { /* DUMMY BODY */ }

            explicit uniform_int_distribution(const param_type& p)
                : a_{p.first}, b_{p.second}
            { /* DUMMY BODY */ }

            void reset()
            { /* DUMMY BODY */ }

            template<class URNG>
            result_type operator()(URNG& g)
            {
                auto range = b_ - a_ + 1;

                return g() % range + a_;
            }

            template<class URNG>
            result_type operator()(URNG& g, const param_type& p)
            {
                auto range = p.second - p.first + 1;

                return g() % range + p.first;
            }

            result_type a() const
            {
                return a_;
            }

            result_type b() const
            {
                return b_;
            }

            param_type param() const
            {
                return param_type{a_, b_};
            }

            void param(const param_type& p)
            {
                a_ = p.first;
                b_ = p.second;
            }

            result_type min() const
            {
                return a_;
            }

            result_type max() const
            {
                return b_;
            }

            bool operator==(const uniform_int_distribution& rhs) const
            {
                return a_ == rhs.a_ && b_ == rhs.b_;
            }

            bool operator!=(const uniform_int_distribution& rhs) const
            {
                return !(*this == rhs);
            }

            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                auto flags = os.flags();
                os.flags(ios_base::dec | ios_base::left);

                os << a_ << os.widen(' ') << b_;

                os.flags(flags);
                return os;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                auto flags = is.flags();
                is.flags(ios_base::dec);

                if (!(is >> a_) || !(is >> b_))
                    is.setstate(ios::failbit);

                is.flags(flags);
                return is;
            }

        private:
            result_type a_;
            result_type b_;
    };

    /**
     * 26.5.8.2.2, class template uniform_real_distribution:
     */

    template<class RealType = double>
    class uniform_real_distribution
    {
        public:
            using result_type = RealType;
            using param_type  = pair<result_type, result_type>;

            explicit uniform_real_distribution(result_type a = 0.0,
                                               result_type b = 1.0)
                : a_{a}, b_{b}
            { /* DUMMY BODY */ }

            explicit uniform_real_distribution(const param_type& p)
                : a_{p.first}, b_{p.second}
            { /* DUMMY BODY */ }

            void reset()
            { /* DUMMY BODY */ }

            template<class URNG>
            result_type operator()(URNG& g)
            {
                auto range = b_ - a_ + 1;

                return generate_canonical<
                    result_type, numeric_limits<result_type>::digits
                >(g) * range + a_;
            }

            template<class URNG>
            result_type operator()(URNG& g, const param_type& p)
            {
                auto range = p.second - p.first + 1;

                return generate_canonical<
                    result_type, numeric_limits<result_type>::digits
                >(g) * range + p.first;
            }

            result_type a() const
            {
                return a_;
            }

            result_type b() const
            {
                return b_;
            }

            param_type param() const
            {
                return param_type{a_, b_};
            }

            void param(const param_type& p)
            {
                a_ = p.first;
                b_ = p.second;
            }

            result_type min() const
            {
                return a_;
            }

            result_type max() const
            {
                return b_;
            }

            bool operator==(const uniform_real_distribution& rhs) const
            {
                return a_ == rhs.a_ && b_ == rhs.b_;
            }

            bool operator!=(const uniform_real_distribution& rhs) const
            {
                return !(*this == rhs);
            }

            // TODO: ostream/istream operators can't be members
            template<class Char, class Traits>
            basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os) const
            {
                auto flags = os.flags();
                os.flags(ios_base::dec | ios_base::left);

                os << a_ << os.widen(' ') << b_;

                os.flags(flags);
                return os;
            }

            template<class Char, class Traits>
            basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is) const
            {
                auto flags = is.flags();
                is.flags(ios_base::dec);

                if (!(is >> a_) || !(is >> b_))
                    is.setstate(ios::failbit);

                is.flags(flags);
                return is;
            }

        private:
            result_type a_;
            result_type b_;
    };

    /**
     * 26.5.8.3.1, class bernoulli_distribution:
     */

    class bernoulli_distribution
    {
        public:
            using result_type = bool;
            using param_type  = float;

            explicit bernoulli_distribution(double prob = 0.5)
                : prob_{static_cast<float>(prob)}
            { /* DUMMY BODY */ }

            explicit bernoulli_distribution(const param_type& p)
                : prob_{p}
            { /* DUMMY BODY */ }

            void reset()
            { /* DUMMY BODY */ }

            template<class URNG>
            result_type operator()(URNG& g)
            {
                uniform_real_distribution<float> dist{};

                return dist(g) < prob_;
            }

            template<class URNG>
            result_type operator()(const param_type& p)
            {
                uniform_real_distribution<float> dist{};

                return dist(p) < prob_;
            }

            double p() const
            {
                return prob_;
            }

            param_type param() const
            {
                return prob_;
            }

            void param(const param_type& p)
            {
                prob_ = p;
            }

            result_type min() const
            {
                return false;
            }

            result_type max() const
            {
                return true;
            }

        private:
            /**
             * Note: We use float because we do not
             *       have the macro DBL_MANT_DIGITS
             *       for generate_cannonical.
             */
            float prob_;
    };

    // TODO: complete the rest of the distributions

    /**
     * 26.5.8.3.2, class template binomial_distribution:
     */

    template<class IntType = int>
    class binomial_distribution;

    /**
     * 26.5.8.3.3, class template geometric_distribution:
     */

    template<class IntType = int>
    class geometric_distribution;

    /**
     * 26.5.8.3.4, class template negative_binomial_distribution:
     */

    template<class IntType = int>
    class negative_binomial_distribution;

    /**
     * 26.5.8.4.1, class template poisson_distribution:
     */

    template<class IntType = int>
    class poisson_distribution;

    /**
     * 26.5.8.4.2, class template exponential_distribution:
     */

    template<class RealType = double>
    class exponential_distribution;

    /**
     * 26.5.8.4.3, class template gamma_distribution:
     */

    template<class RealType = double>
    class gamma_distribution;

    /**
     * 26.5.8.4.4, class template weibull_distribution:
     */

    template<class RealType = double>
    class weibull_distribution;

    /**
     * 26.5.8.4.5, class template extreme_value_distribution:
     */

    template<class RealType = double>
    class extreme_value_distribution;

    /**
     * 26.5.8.5.1, class template normal_distribution:
     */

    template<class RealType = double>
    class normal_distribution;

    /**
     * 26.5.8.5.2, class template lognormal_distribution:
     */

    template<class RealType = double>
    class lognormal_distribution;

    /**
     * 26.5.8.5.3, class template chi_squared_distribution:
     */

    template<class RealType = double>
    class chi_squared_distribution;

    /**
     * 26.5.8.5.4, class template cauchy_distribution:
     */

    template<class RealType = double>
    class cauchy_distribution;

    /**
     * 26.5.8.5.5, class template fisher_f_distribution:
     */

    template<class RealType = double>
    class fisher_f_distribution;

    /**
     * 26.5.8.5.6, class template student_t_distribution:
     */

    template<class RealType = double>
    class student_t_distribution;

    /**
     * 26.5.8.6.1, class template discrete_distribution:
     */

    template<class IntType = int>
    class discrete_distribution;

    /**
     * 26.5.8.6.2, class template piecewise_constant_distribution:
     */

    template<class RealType = double>
    class piecewise_constant_distribution;

    /**
     * 26.5.8.6.3, class template piecewise_linear_distribution:
     */

    template<class RealType = double>
    class piecewise_linear_distribution;
}

#endif
