/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#ifndef LIBCPP_BITS_COMPLEX
#define LIBCPP_BITS_COMPLEX

#include <cassert>
#include <iosfwd>
#include <sstream>

namespace std
{
    template<class T>
    class complex
    {
        public:
            using value_type = T;

            constexpr complex(const value_type& re = value_type{},
                              const value_type& im = value_type{})
                : real_{re}, imag_{im}
            { /* DUMMY BODY */ }

            constexpr complex(const complex& other)
                : real_{other.real_}, imag_{other.imag_}
            { /* DUMMY BODY */ }

            template<class U>
            constexpr complex(const complex<U>& other)
                : real_(other.real()), imag_(other.imag())
            { /* DUMMY BODY */ }

            constexpr value_type real() const
            {
                return real_;
            }

            void real(value_type val)
            {
                real_ = val;
            }

            constexpr value_type imag() const
            {
                return imag_;
            }

            void imag(value_type val)
            {
                imag_ = val;
            }

            complex& operator=(const value_type& val)
            {
                real_ = val;
                imag_ = value_type{};

                return *this;
            }

            complex& operator+=(const value_type& val)
            {
                real_ += val;

                return *this;
            }

            complex& operator-=(const value_type& val)
            {
                real_ -= val;

                return *this;
            }

            complex& operator*=(const value_type& val)
            {
                real_ *= val;
                imag_ *= val;

                return *this;
            }

            complex& operator/=(const value_type& val)
            {
                real_ /= val;
                imag_ /= val;

                return *this;
            }

            complex& operator=(const complex& other)
            {
                real_ = other.real_;
                imag_ = other.imag_;

                return *this;
            }

            template<class U>
            complex& operator=(const complex<U>& rhs)
            {
                real_ = rhs.real_;
                imag_ = rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator+=(const complex<U>& rhs)
            {
                real_ += rhs.real_;
                imag_ += rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator-=(const complex<U>& rhs)
            {
                real_ -= rhs.real_;
                imag_ -= rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator*=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = real_ * rhs.real_ - imag_ * rhs.imag_;
                imag_ = old_real * rhs.imag_ + imag_ * rhs.real_;

                return *this;
            }

            template<class U>
            complex& operator/=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = (real_ * rhs.real_ + imag_ * rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);
                imag_ = (imag_ * rhs.real_ - old_real * rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);

                return *this;
            }

        private:
            value_type real_;
            value_type imag_;
    };

    template<>
    class complex<float>
    {
        public:
            using value_type = float;

            constexpr complex(const value_type& re = value_type{},
                              const value_type& im = value_type{})
                : real_{re}, imag_{im}
            { /* DUMMY BODY */ }

            constexpr complex(const complex& other)
                : real_{other.real_}, imag_{other.imag_}
            { /* DUMMY BODY */ }

            template<class U>
            constexpr complex(const complex<U>& other)
                : real_(other.real()), imag_(other.imag())
            { /* DUMMY BODY */ }

            constexpr value_type real() const
            {
                return real_;
            }

            void real(value_type val)
            {
                real_ = val;
            }

            constexpr value_type imag() const
            {
                return imag_;
            }

            void imag(value_type val)
            {
                imag_ = val;
            }

            complex& operator=(const value_type& val)
            {
                real_ = val;
                imag_ = value_type{};

                return *this;
            }

            complex& operator+=(const value_type& val)
            {
                real_ += val;

                return *this;
            }

            complex& operator-=(const value_type& val)
            {
                real_ -= val;

                return *this;
            }

            complex& operator*=(const value_type& val)
            {
                real_ *= val;
                imag_ *= val;

                return *this;
            }

            complex& operator/=(const value_type& val)
            {
                real_ /= val;
                imag_ /= val;

                return *this;
            }

            complex& operator=(const complex& other)
            {
                real_ = other.real_;
                imag_ = other.imag_;

                return *this;
            }

            template<class U>
            complex& operator=(const complex<U>& rhs)
            {
                real_ = rhs.real_;
                imag_ = rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator+=(const complex<U>& rhs)
            {
                real_ += rhs.real_;
                imag_ += rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator-=(const complex<U>& rhs)
            {
                real_ -= rhs.real_;
                imag_ -= rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator*=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = real_ * rhs.real_ - imag_ * rhs.imag_;
                imag_ = old_real * rhs.imag_ + imag_ * rhs.real_;

                return *this;
            }

            template<class U>
            complex& operator/=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = (real_ * rhs.real_ + imag_ * rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);
                imag_ = (imag_ * rhs.real_ - old_real * rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);

                return *this;
            }

        private:
            value_type real_;
            value_type imag_;
    };

    template<>
    class complex<double>
    {
        public:
            using value_type = double;

            constexpr complex(const value_type& re = value_type{},
                              const value_type& im = value_type{})
                : real_{re}, imag_{im}
            { /* DUMMY BODY */ }

            constexpr complex(const complex& other)
                : real_{other.real_}, imag_{other.imag_}
            { /* DUMMY BODY */ }

            template<class U>
            constexpr complex(const complex<U>& other)
                : real_(other.real()), imag_(other.imag())
            { /* DUMMY BODY */ }

            constexpr value_type real() const
            {
                return real_;
            }

            void real(value_type val)
            {
                real_ = val;
            }

            constexpr value_type imag() const
            {
                return imag_;
            }

            void imag(value_type val)
            {
                imag_ = val;
            }

            complex& operator=(const value_type& val)
            {
                real_ = val;
                imag_ = value_type{};

                return *this;
            }

            complex& operator+=(const value_type& val)
            {
                real_ += val;

                return *this;
            }

            complex& operator-=(const value_type& val)
            {
                real_ -= val;

                return *this;
            }

            complex& operator*=(const value_type& val)
            {
                real_ *= val;
                imag_ *= val;

                return *this;
            }

            complex& operator/=(const value_type& val)
            {
                real_ /= val;
                imag_ /= val;

                return *this;
            }

            complex& operator=(const complex& other)
            {
                real_ = other.real_;
                imag_ = other.imag_;

                return *this;
            }

            template<class U>
            complex& operator=(const complex<U>& rhs)
            {
                real_ = rhs.real_;
                imag_ = rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator+=(const complex<U>& rhs)
            {
                real_ += rhs.real_;
                imag_ += rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator-=(const complex<U>& rhs)
            {
                real_ -= rhs.real_;
                imag_ -= rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator*=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = real_ * rhs.real_ - imag_ * rhs.imag_;
                imag_ = old_real * rhs.imag_ + imag_ * rhs.real_;

                return *this;
            }

            template<class U>
            complex& operator/=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = (real_ * rhs.real_ + imag_ * rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);
                imag_ = (imag_ * rhs.real_ - old_real * rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);

                return *this;
            }

        private:
            value_type real_;
            value_type imag_;
    };

    template<>
    class complex<long double>
    {
        public:
            using value_type = long double;

            constexpr complex(const value_type& re = value_type{},
                              const value_type& im = value_type{})
                : real_{re}, imag_{im}
            { /* DUMMY BODY */ }

            constexpr complex(const complex& other)
                : real_{other.real_}, imag_{other.imag_}
            { /* DUMMY BODY */ }

            template<class U>
            constexpr complex(const complex<U>& other)
                : real_(other.real()), imag_(other.imag())
            { /* DUMMY BODY */ }

            constexpr value_type real() const
            {
                return real_;
            }

            void real(value_type val)
            {
                real_ = val;
            }

            constexpr value_type imag() const
            {
                return imag_;
            }

            void imag(value_type val)
            {
                imag_ = val;
            }

            complex& operator=(const value_type& val)
            {
                real_ = val;
                imag_ = value_type{};

                return *this;
            }

            complex& operator+=(const value_type& val)
            {
                real_ += val;

                return *this;
            }

            complex& operator-=(const value_type& val)
            {
                real_ -= val;

                return *this;
            }

            complex& operator*=(const value_type& val)
            {
                real_ *= val;
                imag_ *= val;

                return *this;
            }

            complex& operator/=(const value_type& val)
            {
                real_ /= val;
                imag_ /= val;

                return *this;
            }

            complex& operator=(const complex& other)
            {
                real_ = other.real_;
                imag_ = other.imag_;

                return *this;
            }

            template<class U>
            complex& operator=(const complex<U>& rhs)
            {
                real_ = rhs.real_;
                imag_ = rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator+=(const complex<U>& rhs)
            {
                real_ += rhs.real_;
                imag_ += rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator-=(const complex<U>& rhs)
            {
                real_ -= rhs.real_;
                imag_ -= rhs.imag_;

                return *this;
            }

            template<class U>
            complex& operator*=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = real_ * rhs.real_ - imag_ * rhs.imag_;
                imag_ = old_real * rhs.imag_ + imag_ * rhs.real_;

                return *this;
            }

            template<class U>
            complex& operator/=(const complex<U>& rhs)
            {
                auto old_real = real_;
                real_ = (real_ * rhs.real_ + imag_ * rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);
                imag_ = (imag_ * rhs.real_ - old_real* rhs.imag_)
                      / (rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_);

                return *this;
            }

        private:
            value_type real_;
            value_type imag_;
    };

    /**
     * 26.4.6, operators:
     */

    template<class T>
    complex<T> operator+(const complex<T>& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} += rhs;
    }

    template<class T>
    complex<T> operator+(const complex<T>& lhs, const T& rhs)
    {
        return complex<T>{lhs} += rhs;
    }

    template<class T>
    complex<T> operator+(const T& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} += rhs;
    }

    template<class T>
    complex<T> operator-(const complex<T>& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} -= rhs;
    }

    template<class T>
    complex<T> operator-(const complex<T>& lhs, const T& rhs)
    {
        return complex<T>{lhs} -= rhs;
    }

    template<class T>
    complex<T> operator-(const T& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} -= rhs;
    }

    template<class T>
    complex<T> operator*(const complex<T>& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} *= rhs;
    }

    template<class T>
    complex<T> operator*(const complex<T>& lhs, const T& rhs)
    {
        return complex<T>{lhs} *= rhs;
    }

    template<class T>
    complex<T> operator*(const T& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} *= rhs;
    }

    template<class T>
    complex<T> operator/(const complex<T>& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} /= rhs;
    }

    template<class T>
    complex<T> operator/(const complex<T>& lhs, const T& rhs)
    {
        return complex<T>{lhs} /= rhs;
    }

    template<class T>
    complex<T> operator/(const T& lhs, const complex<T>& rhs)
    {
        return complex<T>{lhs} /= rhs;
    }

    template<class T>
    complex<T> operator+(const complex<T>& c)
    {
        return complex<T>{c};
    }

    template<class T>
    complex<T> operator-(const complex<T>& c)
    {
        return complex<T>{-c.real(), -c.imag()};
    }

    template<class T>
    constexpr bool operator==(const complex<T>& lhs, const complex<T>& rhs)
    {
        return lhs.real() == rhs.real() && lhs.imag() == rhs.imag();
    }

    template<class T>
    constexpr bool operator==(const complex<T>& lhs, const T& rhs)
    {
        return lhs.real() == rhs && lhs.imag() == T{};
    }

    template<class T>
    constexpr bool operator==(const T& lhs, const complex<T>& rhs)
    {
        return lhs == rhs.real() && T{} == rhs.imag();
    }

    template<class T>
    constexpr bool operator!=(const complex<T>& lhs, const complex<T>& rhs)
    {
        return lhs.real() != rhs.real() || lhs.imag() != rhs.imag();
    }

    template<class T>
    constexpr bool operator!=(const complex<T>& lhs, const T& rhs)
    {
        return lhs.real() != rhs || lhs.imag() != T{};
    }

    template<class T>
    constexpr bool operator!=(const T& lhs, const complex<T>& rhs)
    {
        return lhs != rhs.real() || T{} != rhs.imag();
    }

    template<class T, class Char, class Traits>
    basic_istream<Char, Traits>& operator>>(basic_istream<Char, Traits>& is,
                                            const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return is;
    }

    template<class T, class Char, class Traits>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os,
                                            const complex<T>& c)
    {
        basic_ostringstream<Char, Traits> oss{};
        oss.flags(os.flags());
        oss.imbue(os.getloc());
        oss.precision(os.precision());

        oss << "(" << c.real() << "," << c.imag() << ")";

        return os << oss.str();
    }

    /**
     * 26.4.7, values:
     */

    template<class T>
    constexpr T real(const complex<T>& c)
    {
        return c.real();
    }

    template<class T>
    constexpr T imag(const complex<T>& c)
    {
        return c.imag();
    }

    template<class T>
    T abs(const complex<T>& c)
    {
        return c.real() * c.real() + c.imag() * c.imag();
    }

    template<class T>
    T arg(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    T norm(const complex<T>& c)
    {
        return abs(c) * abs(c);
    }

    template<class T>
    complex<T> conj(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> proj(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> polar(const T&, const T& = T{})
    {
        // TODO: implement
        __unimplemented();
        return complex<T>{};
    }

    /**
     * 26.4.8, transcendentals:
     */

    template<class T>
    complex<T> acos(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> asin(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> atan(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> acosh(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> asinh(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> atanh(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> cos(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> cosh(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> exp(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> log(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> log10(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> pow(const complex<T>& base, const T& exp)
    {
        // TODO: implement
        __unimplemented();
        return base;
    }

    template<class T>
    complex<T> pow(const complex<T>& base, const complex<T>& exp)
    {
        // TODO: implement
        __unimplemented();
        return base;
    }

    template<class T>
    complex<T> pow(const T& base, const complex<T>& exp)
    {
        // TODO: implement
        __unimplemented();
        return complex<T>{base};
    }

    template<class T>
    complex<T> sin(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> sinh(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> sqrt(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> tan(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    template<class T>
    complex<T> tanh(const complex<T>& c)
    {
        // TODO: implement
        __unimplemented();
        return c;
    }

    /**
     * 26.4.10, complex literals:
     */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"
inline namespace literals {
inline namespace complex_literals
{
    constexpr complex<long double> operator ""il(long double val)
    {
        return complex<long double>{0.0L, val};
    }

    constexpr complex<long double> operator ""il(unsigned long long val)
    {
        return complex<long double>{0.0L, static_cast<long double>(val)};
    }

    constexpr complex<double> operator ""i(long double val)
    {
        return complex<double>{0.0, static_cast<double>(val)};
    }

    constexpr complex<double> operator ""i(unsigned long long val)
    {
        return complex<double>{0.0, static_cast<double>(val)};
    }

    constexpr complex<float> operator ""if(long double val)
    {
        return complex<float>{0.0f, static_cast<float>(val)};
    }

    constexpr complex<float> operator ""if(unsigned long long val)
    {
        return complex<float>{0.0f, static_cast<float>(val)};
    }
}}
#pragma GCC diagnostic pop
}

#endif
