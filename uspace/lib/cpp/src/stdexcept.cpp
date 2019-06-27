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

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <str.h>

namespace std
{
    logic_error::logic_error(const string& what)
        : what_{::helenos::str_dup(what.c_str())}
    { /* DUMMY BODY */ }

    logic_error::logic_error(const char* what)
        : what_{::helenos::str_dup(what)}
    { /* DUMMY BODY */ }

    logic_error::logic_error(const logic_error& other) noexcept
        : exception{other}, what_{::helenos::str_dup(other.what_)}
    { /* DUMMY BODY */ }

    logic_error& logic_error::operator=(const logic_error& other)
    {
        if (what_)
            free(what_);
        what_ = ::helenos::str_dup(other.what_);

        return *this;
    }

    logic_error::~logic_error()
    {
        free(what_);
    }

    const char* logic_error::what() const noexcept
    {
        return what_;
    }

    domain_error::domain_error(const string& what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    domain_error::domain_error(const char* what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    domain_error::domain_error(const domain_error& other) noexcept
        : logic_error{other}
    { /* DUMMY BODY */ }

    invalid_argument::invalid_argument(const string& what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    invalid_argument::invalid_argument(const char* what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    invalid_argument::invalid_argument(const invalid_argument& other) noexcept
        : logic_error{other}
    { /* DUMMY BODY */ }

    length_error::length_error(const string& what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    length_error::length_error(const char* what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    length_error::length_error(const length_error& other) noexcept
        : logic_error{other}
    { /* DUMMY BODY */ }

    out_of_range::out_of_range(const string& what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    out_of_range::out_of_range(const char* what)
        : logic_error{what}
    { /* DUMMY BODY */ }

    out_of_range::out_of_range(const out_of_range& other) noexcept
        : logic_error{other}
    { /* DUMMY BODY */ }

    runtime_error::runtime_error(const string& what)
        : what_{::helenos::str_dup(what.c_str())}
    { /* DUMMY BODY */ }

    runtime_error::runtime_error(const char* what)
        : what_{::helenos::str_dup(what)}
    { /* DUMMY BODY */ }

    runtime_error::runtime_error(const runtime_error& other) noexcept
        : exception{other}, what_{::helenos::str_dup(other.what_)}
    { /* DUMMY BODY */ }

    runtime_error& runtime_error::operator=(const runtime_error& other)
    {
        if (what_)
            free(what_);
        what_ = ::helenos::str_dup(other.what_);

        return *this;
    }

    runtime_error::~runtime_error()
    {
        free(what_);
    }

    const char* runtime_error::what() const noexcept
    {
        return what_;
    }

    range_error::range_error(const string& what)
        : runtime_error{what}
    { /* DUMMY BODY */ }

    range_error::range_error(const char* what)
        : runtime_error{what}
    { /* DUMMY BODY */ }

    range_error::range_error(const range_error& other) noexcept
        : runtime_error{other}
    { /* DUMMY BODY */ }

    overflow_error::overflow_error(const string& what)
        : runtime_error{what}
    { /* DUMMY BODY */ }

    overflow_error::overflow_error(const char* what)
        : runtime_error{what}
    { /* DUMMY BODY */ }

    overflow_error::overflow_error(const overflow_error& other) noexcept
        : runtime_error{other}
    { /* DUMMY BODY */ }

    underflow_error::underflow_error(const string& what)
        : runtime_error{what}
    { /* DUMMY BODY */ }

    underflow_error::underflow_error(const char* what)
        : runtime_error{what}
    { /* DUMMY BODY */ }

    underflow_error::underflow_error(const underflow_error& other) noexcept
        : runtime_error{other}
    { /* DUMMY BODY */ }
}
