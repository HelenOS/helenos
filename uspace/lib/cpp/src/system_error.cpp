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

#include <cstring>
#include <functional>
#include <string>
#include <system_error>

namespace std
{
    error_category::~error_category()
    { /* DUMMY BODY */ }

    error_condition error_category::default_error_condition(int code) const noexcept
    {
        return error_condition{code, *this};
    }

    bool error_category::equivalent(int code, const error_condition& cond) const noexcept
    {
        return default_error_condition(code) == cond;
    }

    bool error_category::equivalent(const error_code& ec, int cond) const noexcept
    {
        return *this == ec.category() && ec.value() == cond;
    }

    bool error_category::operator==(const error_category& rhs) const noexcept
    {
        return this == &rhs;
    }

    bool error_category::operator!=(const error_category& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    bool error_category::operator<(const error_category& rhs) const noexcept
    {
        return less<const error_category*>{}(this, &rhs);
    }

    namespace aux
    {
        class generic_category_t: public error_category
        {
            public:
                generic_category_t()
                    : error_category{}
                { /* DUMMY BODY */ }

                ~generic_category_t() = default;

                const char* name() const noexcept override
                {
                    return "generic";
                }

                string message(int ev) const override
                {
                    return "ev: " + std::to_string(ev);
                }
        };

        class system_category_t: public error_category
        {
            public:
                system_category_t()
                    : error_category{}
                { /* DUMMY BODY */ }

                ~system_category_t() = default;

                const char* name() const noexcept override
                {
                    return "system";
                }

                string message(int ev) const override
                {
                    return "ev: " + std::to_string(ev);
                }
        };
    }

    const error_category& generic_category() noexcept
    {
        static aux::generic_category_t instance{};

        return instance;
    }

    const error_category& system_category() noexcept
    {
        static aux::system_category_t instance{};

        return instance;
    }

    error_code::error_code() noexcept
        : val_{}, cat_{&system_category()}
    { /* DUMMY BODY */ }

    error_code::error_code(int val, const error_category& cat) noexcept
        : val_{val}, cat_{&cat}
    { /* DUMMY BODY */ }

    void error_code::assign(int val, const error_category& cat) noexcept
    {
        val_ = val;
        cat_ = &cat;
    }

    void error_code::clear() noexcept
    {
        val_ = 0;
        cat_ = &system_category();
    }

    int error_code::value() const noexcept
    {
        return val_;
    }

    const error_category& error_code::category() const noexcept
    {
        return *cat_;
    }

    error_condition error_code::default_error_condition() const noexcept
    {
        return cat_->default_error_condition(val_);
    }

    string error_code::message() const
    {
        return cat_->message(val_);
    }

    error_code make_error_code(errc e) noexcept
    {
        return error_code{static_cast<int>(e), generic_category()};
    }

    bool operator<(const error_code& lhs, const error_code& rhs) noexcept
    {
        return lhs.category() < rhs.category() ||
            (lhs.category() == rhs.category() && lhs.value() < rhs.value());
    }

    error_condition::error_condition() noexcept
        : val_{}, cat_{&generic_category()}
    { /* DUMMY BODY */ }

    error_condition::error_condition(int val, const error_category& cat) noexcept
        : val_{val}, cat_{&cat}
    { /* DUMMY BODY */ }

    void error_condition::assign(int val, const error_category& cat) noexcept
    {
        val_ = val;
        cat_ = &cat;
    }

    void error_condition::clear() noexcept
    {
        val_ = 0;
        cat_ = &generic_category();
    }

    int error_condition::value() const noexcept
    {
        return val_;
    }

    const error_category& error_condition::category() const noexcept
    {
        return *cat_;
    }

    string error_condition::message() const
    {
        return cat_->message(val_);
    }

    error_condition make_error_condition(errc e) noexcept
    {
        return error_condition{static_cast<int>(e), generic_category()};
    }

    bool operator<(const error_condition& lhs, const error_condition& rhs) noexcept
    {
        return lhs.category() < rhs.category() ||
            (lhs.category() == rhs.category() && lhs.value() < rhs.value());
    }

    bool operator==(const error_code& lhs, const error_code& rhs) noexcept
    {
        return lhs.category() == rhs.category() && lhs.value() == rhs.value();
    }

    bool operator==(const error_code& lhs, const error_condition& rhs) noexcept
    {
        return lhs.category().equivalent(lhs.value(), rhs)
            || rhs.category().equivalent(lhs, rhs.value());
    }

    bool operator==(const error_condition& lhs, const error_code& rhs) noexcept
    {
        return rhs.category().equivalent(rhs.value(), lhs)
            || lhs.category().equivalent(rhs, lhs.value());
    }

    bool operator==(const error_condition& lhs, const error_condition& rhs) noexcept
    {
        return lhs.category() == rhs.category() && lhs.value() == rhs.value();
    }

    bool operator!=(const error_code& lhs, const error_code& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    bool operator!=(const error_code& lhs, const error_condition& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    bool operator!=(const error_condition& lhs, const error_code& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    bool operator!=(const error_condition& lhs, const error_condition& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    system_error::system_error(error_code ec, const string& what_arg)
        : runtime_error{what_arg.c_str()}, code_{ec}
    { /* DUMMY BODY */ }

    system_error::system_error(error_code ec, const char* what_arg)
        : runtime_error{what_arg}, code_{ec}
    { /* DUMMY BODY */ }

    system_error::system_error(error_code ec)
        : runtime_error{"system_error"}, code_{ec}
    { /* DUMMY BODY */ }

    system_error::system_error(int code, const error_category& cat,
                               const string& what_arg)
        : runtime_error{what_arg.c_str()}, code_{code, cat}
    { /* DUMMY BODY */ }

    system_error::system_error(int code, const error_category& cat,
                               const char* what_arg)
        : runtime_error{what_arg}, code_{code, cat}
    { /* DUMMY BODY */ }

    system_error::system_error(int code, const error_category& cat)
        : runtime_error{"system_error"}, code_{code, cat}
    { /* DUMMY BODY */ }

    const error_code& system_error::code() const noexcept
    {
        return code_;
    }
}
