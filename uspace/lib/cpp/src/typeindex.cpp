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

#include <typeindex>

namespace std
{
    type_index::type_index(const type_info& rhs) noexcept
        : target_{&rhs}
    { /* DUMMY BODY */ }

    bool type_index::operator==(const type_index& rhs) const noexcept
    {
        return *target_ == *rhs.target_;
    }

    bool type_index::operator!=(const type_index& rhs) const noexcept
    {
        return *target_ != *rhs.target_;
    }

    bool type_index::operator<(const type_index& rhs) const noexcept
    {
        return target_->before(*rhs.target_);
    }

    bool type_index::operator<=(const type_index& rhs) const noexcept
    {
        return !rhs.target_->before(*target_);
    }

    bool type_index::operator>(const type_index& rhs) const noexcept
    {
        return rhs.target_->before(*target_);
    }

    bool type_index::operator>=(const type_index& rhs) const noexcept
    {
        return !target_->before(*rhs.target_);
    }

    size_t type_index::hash_code() const noexcept
    {
        return target_->hash_code();
    }

    const char* type_index::name() const noexcept
    {
        return target_->name();
    }
}
