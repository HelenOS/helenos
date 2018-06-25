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
