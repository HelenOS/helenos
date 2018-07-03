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

#ifndef LIBCPP_BITS_ADT_BITSET
#define LIBCPP_BITS_ADT_BITSET

#include <iosfwd>
#include <string>

namespace std
{
    /**
     * 20.6, class template bitset:
     */

    template<size_t N>
    class bitset
    {
        public:
            class reference
            {
                friend class bitset;

                public:
                    ~reference() noexcept = default;

                    reference& operator=(bool val) noexcept
                    {
                        if (val)
                            data_ |= mask_;
                        else
                            data_ &= ~mask_;
                    }

                    reference& operator=(const reference& other) noexcept
                    {
                        if (other)
                            data_ |= mask_;
                        else
                            data_ &= ~mask_;
                    }

                    bool operator~() const noexcept
                    {
                        return data_ & mask_;
                    }

                    operator bool() const noexcept
                    {
                        return data_ & mask_;
                    }

                    reference& flip() noexcept
                    {
                        data_ ^= mask_;

                        return *this;
                    }

                private:
                    reference() noexcept = delete;

                    using data_type = typename bitset::data_type;

                    data_type& data_;
                    data_type mask_;

                    reference(data_type& data, size_t idx)
                        : data_{data}, mask_{bitset::one_ << idx}
                    { /* DUMMY BODY */ }
            };

            /**
             * 20.6.1, constructors:
             */

            constexpr bitset() noexcept
            {
                init_(zero_);
            }

            constexpr bitset(unsigned long long val) noexcept
            {
                init_(val);
            }

            template<class Char, class Traits, class Allocator>
            explicit bitset(
                const basic_string<Char, Traits, Allocator>& str,
                typename basic_string<Char, Traits, Allocator>::size_type pos = 0,
                typename basic_string<Char, Traits, Allocator>::size_type n =
                    basic_string<Char, Traits, Allocator>::npos,
                Char zero = Char('0'), Char one = Char('1')
            )
            {
                // TODO: throw out_of_range if pos > str.size()

                init_(zero_);
                auto len = n < (str.size() - pos) ? n : (str.size() - pos);
                len = len < N ? len : N;

                for (size_t i = 0, j = pos + len - 1; i < len; ++i, --j)
                {
                    if (Traits::eq(str[j], zero))
                        set(i, false);
                    else if (Traits::eq(str[j], one))
                        set(i, true);
                    // TODO: else throw invalid_argument
                }
            }

            template<class Char>
            explicit bitset(
                const Char* str,
                typename basic_string<Char>::size_type n = basic_string<Char>::npos,
                Char zero = Char('0'), Char one = Char('1')
            )
                : bitset{
                    n == basic_string<Char>::npos ? basic_string<Char>{str} : basic_string<Char>{str, n},
                    0, n, zero, one
                  }
            { /* DUMMY BODY */ }

            /**
             * 20.6.2, bitset operations:
             */

            bitset<N>& operator&=(const bitset<N>& rhs) noexcept
            {
                for (size_t i = 0; i < data_size_; ++i)
                    data_[i] &= rhs.data_[i];

                return *this;
            }

            bitset<N>& operator|=(const bitset<N>& rhs) noexcept
            {
                for (size_t i = 0; i < data_size_; ++i)
                    data_[i] |= rhs.data_[i];

                return *this;
            }

            bitset<N>& operator^=(const bitset<N>& rhs) noexcept
            {
                for (size_t i = 0; i < data_size_; ++i)
                    data_[i] ^= rhs.data_[i];

                return *this;
            }

            bitset<N>& operator<<=(size_t pos) noexcept
            {
                for (size_t i = N; i > 1; --i)
                {
                    if (i < pos)
                        set(i - 1, false);
                    else
                        set(i - 1, test(i - 1 - pos));
                }

                return *this;
            }

            bitset<N>& operator>>=(size_t pos) noexcept
            {
                for (size_t i = 0; i < N; ++i)
                {
                    if (pos >= N - i)
                        set(i, false);
                    else
                        set(i, test(i + pos));
                }

                return *this;
            }

            bitset<N>& set() noexcept
            {
                for (size_t i = 0; i < N; ++i)
                    set(i);

                return *this;
            }

            bitset<N>& set(size_t pos, bool val = true)
            {
                // TODO: throw out_of_range if pos > N
                set_bit_(get_data_idx_(pos), get_bit_idx_(pos), val);

                return *this;
            }

            bitset<N>& reset() noexcept
            {
                for (size_t i = 0; i < N; ++i)
                    reset(i);

                return *this;
            }

            bitset<N>& reset(size_t pos)
            {
                set_bit_(get_data_idx_(pos), get_bit_idx_(pos), false);

                return *this;
            }

            bitset<N> operator~() const noexcept
            {
                bitset<N> res{*this};

                return res.flip();
            }

            bitset<N>& flip() noexcept
            {
                for (size_t i = 0; i < N; ++i)
                    flip(i);

                return *this;
            }

            bitset<N>& flip(size_t pos)
            {
                return set(pos, !test(pos));
            }

            constexpr bool operator[](size_t pos) const
            {
                /**
                 * Note: The standard doesn't mention any exception
                 *       here being thrown at any time, so we shouldn't
                 *       use test().
                 */
                return get_bit_(data_[get_data_idx_(pos)], get_bit_idx_(pos));
            }

            reference operator[](size_t pos)
            {
                return reference{data_[get_data_idx_(pos)], get_bit_idx_(pos)};
            }

            unsigned long to_ulong() const
            {
                // TODO: throw overflow_error if N > bits in ulong
                return static_cast<unsigned long>(data_[0]);
            }

            unsigned long long to_ullong() const
            {
                // TODO: throw overflow_error if N > bits_in_data_type_
                return data_[0];
            }

            template<
                class Char = char,
                class Traits = char_traits<Char>,
                class Allocator = allocator<Char>
            >
            basic_string<Char, Traits, Allocator> to_string(Char zero = Char('0'),
                                                            Char one = Char('1')) const
            {
                basic_string<Char, Traits, Allocator> res{};
                res.reserve(N);
                for (size_t i = N; i > 0; --i)
                {
                    if (test(i - 1))
                        res.push_back(one);
                    else
                        res.push_back(zero);
                }

                return res;
            }

            size_t count() const noexcept
            {
                size_t res{};
                for (size_t i = 0; i < N; ++i)
                {
                    if (test(i))
                        ++res;
                }

                return res;
            }

            constexpr size_t size() const noexcept
            {
                return N;
            }

            bool operator==(const bitset<N>& rhs) const noexcept
            {
                for (size_t i = 0; i < data_size_; ++i)
                {
                    if (data_[i] != rhs.data_[i])
                        return false;
                }

                return true;
            }

            bool operator!=(const bitset<N>& rhs) const noexcept
            {
                return !(*this == rhs);
            }

            bool test(size_t pos) const
            {
                // TODO: throw out of range if pos > N
                return get_bit_(data_[get_data_idx_(pos)], get_bit_idx_(pos));
            }

            bool all() const noexcept
            {
                return count() == size();
            }

            bool any() const noexcept
            {
                return count() != 0;
            }

            bool none() const noexcept
            {
                return count() == 0;
            }

            bitset<N> operator<<(size_t pos) const noexcept
            {
                return bitset<N>{*this} <<= pos;
            }

            bitset<N> operator>>(size_t pos) const noexcept
            {
                return bitset<N>{*this} >>= pos;
            }

        private:
            /**
             * While this might be a bit more wasteful
             * than using unsigned or unsigned long,
             * it will make parts of out code easier
             * to read.
             */
            using data_type = unsigned long long;

            static constexpr size_t bits_in_data_type_ = sizeof(data_type) * 8;
            static constexpr size_t data_size_ = N / bits_in_data_type_ + 1;

            /**
             * These are useful for masks, if we use literals
             * then forgetting to add ULL or changing the data
             * type could result in wrong indices being computed.
             */
            static constexpr data_type zero_ = data_type{0};
            static constexpr data_type one_ = data_type{1};

            data_type data_[data_size_];

            void init_(data_type val)
            {
                data_type mask{~zero_};
                if (N < bits_in_data_type_)
                    mask >>= N;
                data_[0] = val & mask;

                for (size_t i = 1; i < data_size_; ++i)
                    data_[i] = data_type{};
            }

            size_t get_data_idx_(size_t pos) const
            {
                return pos / bits_in_data_type_;
            }

            size_t get_bit_idx_(size_t pos) const
            {
                return pos % bits_in_data_type_;
            }

            bool get_bit_(data_type data, size_t bit_idx) const
            {
                return data & (one_ << bit_idx);
            }

            void set_bit_(size_t data_idx, size_t bit_idx, bool val)
            {
                if (val)
                    data_[data_idx] |= (one_ << bit_idx);
                else
                    data_[data_idx] &= ~(one_ << bit_idx);
            }
    };

    /**
     * 20.6.3 hash support:
     */

    template<class T>
    struct hash;

    template<size_t N>
    struct hash<bitset<N>>
    {
        size_t operator()(const bitset<N>& set) const noexcept
        {
            return hash<unsigned long long>{}(set.to_ullong());
        }

        using argument_type = bitset<N>;
        using result_type   = size_t;
    };

    /**
     * 20.6.4, bitset operators:
     */

    template<size_t N>
    bitset<N> operator&(const bitset<N>& lhs, const bitset<N>& rhs) noexcept
    {
        return bitset<N>{lhs} &= rhs;
    }

    template<size_t N>
    bitset<N> operator|(const bitset<N>& lhs, const bitset<N>& rhs) noexcept
    {
        return bitset<N>{lhs} |= rhs;
    }

    template<size_t N>
    bitset<N> operator^(const bitset<N>& lhs, const bitset<N>& rhs) noexcept
    {
        return bitset<N>{lhs} ^= rhs;
    }

    template<class Char, class Traits, size_t N>
    basic_istream<Char, Traits>&
    operator>>(basic_istream<Char, Traits>& is, bitset<N>& set)
    {
        size_t i{};
        Char c{};
        Char zero{is.widen('0')};
        Char one{is.widen('1')};

        basic_string<Char, Traits> str{};
        while (i < N)
        {
            if (is.eof())
                break;

            is.get(c);

            if (!Traits::eq(c, one) && !Traits::eq(c, zero))
            {
                is.putback(c);
                break;
            }

            str.push_back(c);
            ++i;
        }

        if (i == 0)
            is.setstate(ios_base::failbit);

        set = bitset<N>{str};

        return is;
    }

    template<class Char, class Traits, size_t N>
    basic_ostream<Char, Traits>&
    operator<<(basic_ostream<Char, Traits>& os, const bitset<N>& set)
    {
        return os << set.template to_string<Char, Traits, allocator<Char>>(
            use_facet<ctype<Char>>(os.getloc()).widen('0'),
            use_facet<ctype<Char>>(os.getloc()).widen('1')
        );
    }
}

#endif
