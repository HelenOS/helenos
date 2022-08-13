/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <shared_mutex>

namespace std
{
    shared_timed_mutex::shared_timed_mutex() noexcept
        : mtx_{}
    {
        aux::threading::shared_mutex::init(mtx_);
    }

    shared_timed_mutex::~shared_timed_mutex()
    { /* DUMMY BODY */ }

    void shared_timed_mutex::lock()
    {
        aux::threading::shared_mutex::lock(mtx_);
    }

    bool shared_timed_mutex::try_lock()
    {
        return aux::threading::shared_mutex::try_lock(mtx_);
    }

    void shared_timed_mutex::unlock()
    {
        aux::threading::shared_mutex::unlock(mtx_);
    }

    void shared_timed_mutex::lock_shared()
    {
        aux::threading::shared_mutex::lock_shared(mtx_);
    }

    bool shared_timed_mutex::try_lock_shared()
    {
        return aux::threading::shared_mutex::try_lock_shared(mtx_);
    }

    void shared_timed_mutex::unlock_shared()
    {
        aux::threading::shared_mutex::unlock_shared(mtx_);
    }

    shared_timed_mutex::native_handle_type shared_timed_mutex::native_handle()
    {
        return &mtx_;
    }
}
