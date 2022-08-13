/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <locale>

namespace std
{
    locale::facet::facet(size_t refs)
    {
        // TODO: implement
    }

    locale::facet::~facet()
    {
        // TODO: implement
    }

    locale::locale() noexcept
        : name_{}
    { /* DUMMY BODY */ }

    locale::locale(const locale& other) noexcept
        : name_{other.name_}
    { /* DUMMY BODY */ }

    locale::locale(const char* name)
        : name_{name}
    { /* DUMMY BODY */ }

    locale::locale(const string& name)
        : name_{name}
    { /* DUMMY BODY */ }

    locale::locale(const locale& other, const char* name, category cat)
        : name_{name}
    { /* DUMMY BODY */ }

    locale::locale(const locale& other, const string& name, category cat)
        : name_{name}
    { /* DUMMY BODY */ }

    locale::locale(const locale& other, const locale& one, category cat)
        : name_{other.name_}
    { /* DUMMY BODY */ }

    const locale& locale::operator=(const locale& other) noexcept
    {
        name_ = other.name_;

        return *this;
    }

    string locale::name() const
    {
        return name_;
    }

    bool locale::operator==(const locale& other) const
    {
        return (this == &other) || (name_ == other.name_);
    }

    bool locale::operator!=(const locale& other) const
    {
        return !(*this == other);
    }
}
