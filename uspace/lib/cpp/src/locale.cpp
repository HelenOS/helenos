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
