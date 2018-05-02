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

#ifndef LIBCPP_RANDOM
#define LIBCPP_RANDOM

#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <internal/builtins.hpp>
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

            static constexpr min()
            {
                return c == 0U ? 1U : 0U;
            }

            static constexpr max()
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
                if (c % modulus_ == 0 && s == 0)
                    state_ = 0;
                else
                    state_ = s;
            }

            template<class Seq>
            void seed(
                enable_if_t<aux::is_seed_sequence_v<Seq, result_type>, Seq&> q
            )
            {
                size_t k = static_cast<size_t>(aux::ceil(aux::log2(modulus_) / 32));
                auto arr = new result_type[k + 3];

                q.generate(arr, arr + k + 3);

                result_type s{};
                for (size_t j = 0; j < k; ++j)
                    s += a[j + 3] * aux::pow2(32U * j);
                s = s % modulus_;

                seed(s);
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
    class mersenne_twister_engine;

    /**
     * 26.5.3.3, class template subtract_with_carry_engine:
     */

    template<class UIntType, size_t w, size_t s, size_t r>
    class subtract_with_carry_engine;

    /**
     * 26.5.4.2, class template discard_block_engine:
     */

    template<class Engine, size_t p, size_t r>
    class discard_block_engine;

    /**
     * 26.5.4.3, class template independent_bits_engine:
     */

    template<class Engine, size_t w, class UIntType>
    class independent_bits_engine;

    /**
     * 26.5.4.4, class template shiffle_order_engine:
     */

    template<class Engine, size_t k>
    class shuffle_order_engine;

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
            hel::srandom(hel::time(nullptr));
        }

        result_type operator()()
        {
            return hel::random();
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
                    vec_.push_back(*first++ % aux::pow2(32));
            }

            template<class RandomAccessGenerator>
            void generate(RandomAccessGenerator first,
                          RandomAccessGenerator last)
            {
                if (first == last)
                    return;

                // TODO: research this
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
    };

    /**
     * 26.5.7.2, function template generate_canonical:
     */

    template<class RealType, size_t bits, class URNG>
    RealType generate_canonical(URNG& g);

    /**
     * 26.5.8.2.1, class template uniform_int_distribution:
     */

    template<class IntType = int>
    class uniform_int_distribution;

    /**
     * 26.5.8.2.2, class template uniform_real_distribution:
     */

    template<class RealType = double>
    class uniform_real_distribution;

    /**
     * 26.5.8.3.1, class bernoulli_distribution:
     */

    class bernoulli_distribution;

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
