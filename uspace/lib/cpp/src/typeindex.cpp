/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
