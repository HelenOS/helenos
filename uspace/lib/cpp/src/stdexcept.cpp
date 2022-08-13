/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
