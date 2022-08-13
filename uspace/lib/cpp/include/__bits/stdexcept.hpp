/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_STDEXCEPT
#define LIBCPP_BITS_STDEXCEPT

#include <__bits/string/stringfwd.hpp>
#include <__bits/trycatch.hpp>
#include <exception>
#include <iosfwd>

namespace std
{
    class logic_error: public exception
    {
        public:
            explicit logic_error(const string&);
            explicit logic_error(const char*);
            logic_error(const logic_error&) noexcept;
            logic_error& operator=(const logic_error&);
            ~logic_error() override;

            const char* what() const noexcept override;

        protected:
            char* what_;
    };

    class domain_error: public logic_error
    {
        public:
            explicit domain_error(const string&);
            explicit domain_error(const char*);
            domain_error(const domain_error&) noexcept;
    };

    class invalid_argument: public logic_error
    {
        public:
            explicit invalid_argument(const string&);
            explicit invalid_argument(const char*);
            invalid_argument(const invalid_argument&) noexcept;
    };

    class length_error: public logic_error
    {
        public:
            explicit length_error(const string&);
            explicit length_error(const char*);
            length_error(const length_error&) noexcept;
    };

    class out_of_range: public logic_error
    {
        public:
            explicit out_of_range(const string&);
            explicit out_of_range(const char*);
            out_of_range(const out_of_range&) noexcept;
    };

    class runtime_error: public exception
    {
        public:
            explicit runtime_error(const string&);
            explicit runtime_error(const char*);
            runtime_error(const runtime_error&) noexcept;
            runtime_error& operator=(const runtime_error&);
            ~runtime_error() override;

            const char* what() const noexcept override;

        protected:
            char* what_;
    };

    class range_error: public runtime_error
    {
        public:
            explicit range_error(const string&);
            explicit range_error(const char*);
            range_error(const range_error&) noexcept;
    };

    class overflow_error: public runtime_error
    {
        public:
            explicit overflow_error(const string&);
            explicit overflow_error(const char*);
            overflow_error(const overflow_error&) noexcept;
    };

    class underflow_error: public runtime_error
    {
        public:
            explicit underflow_error(const string&);
            explicit underflow_error(const char*);
            underflow_error(const underflow_error&) noexcept;
    };
}

#endif
