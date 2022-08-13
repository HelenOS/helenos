/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mutex>

namespace std
{
    void mutex::lock()
    {
        aux::threading::mutex::lock(mtx_);
    }

    bool mutex::try_lock()
    {
        return aux::threading::mutex::try_lock(mtx_);
    }

    void mutex::unlock()
    {
        aux::threading::mutex::unlock(mtx_);
    }

    mutex::native_handle_type mutex::native_handle()
    {
        return &mtx_;
    }

    recursive_mutex::~recursive_mutex()
    { /* DUMMY BODY */ }

    void recursive_mutex::lock()
    {
        if (owner_ != this_thread::get_id())
        {
            aux::threading::mutex::lock(mtx_);
            owner_ = this_thread::get_id();
            lock_level_ = 1;
        }
        else
            ++lock_level_;
    }

    bool recursive_mutex::try_lock() noexcept
    {
        if (owner_ != this_thread::get_id())
        {
            bool res = aux::threading::mutex::try_lock(mtx_);
            if (res)
            {
                owner_ = this_thread::get_id();
                lock_level_ = 1;
            }

            return res;
        }
        else
            ++lock_level_;

        return true;
    }

    void recursive_mutex::unlock()
    {
        if (owner_ != this_thread::get_id())
            return;
        else if (--lock_level_ == 0)
            aux::threading::mutex::unlock(mtx_);
    }

    recursive_mutex::native_handle_type recursive_mutex::native_handle()
    {
        return &mtx_;
    }

    timed_mutex::timed_mutex() noexcept
        : mtx_{}
    {
        aux::threading::mutex::init(mtx_);
    }

    timed_mutex::~timed_mutex()
    { /* DUMMY BODY */ }

    void timed_mutex::lock()
    {
        aux::threading::mutex::lock(mtx_);
    }

    bool timed_mutex::try_lock()
    {
        return aux::threading::mutex::try_lock(mtx_);
    }

    void timed_mutex::unlock()
    {
        aux::threading::mutex::unlock(mtx_);
    }

    timed_mutex::native_handle_type timed_mutex::native_handle()
    {
        return &mtx_;
    }

    recursive_timed_mutex::~recursive_timed_mutex()
    { /* DUMMY BODY */ }

    void recursive_timed_mutex::lock()
    {
        if (owner_ != this_thread::get_id())
        {
            aux::threading::mutex::lock(mtx_);
            owner_ = this_thread::get_id();
            lock_level_ = 1;
        }
        else
            ++lock_level_;
    }

    bool recursive_timed_mutex::try_lock() noexcept
    {
        if (owner_ != this_thread::get_id())
        {
            bool res = aux::threading::mutex::try_lock(mtx_);
            if (res)
            {
                owner_ = this_thread::get_id();
                lock_level_ = 1;
            }

            return res;
        }
        else
            ++lock_level_;

        return true;
    }

    void recursive_timed_mutex::unlock()
    {
        if (owner_ != this_thread::get_id())
            return;
        else if (--lock_level_ == 0)
            aux::threading::mutex::unlock(mtx_);
    }

    recursive_timed_mutex::native_handle_type recursive_timed_mutex::native_handle()
    {
        return &mtx_;
    }
}
