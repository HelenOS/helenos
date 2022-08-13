/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cassert>
#include <condition_variable>

namespace std
{
    condition_variable::condition_variable()
        : cv_{}
    {
        aux::threading::condvar::init(cv_);
    }

    condition_variable::~condition_variable()
    { /* DUMMY BODY */ }

    void condition_variable::notify_one() noexcept
    {
        aux::threading::condvar::signal(cv_);
    }

    void condition_variable::notify_all() noexcept
    {
        aux::threading::condvar::broadcast(cv_);
    }

    void condition_variable::wait(unique_lock<mutex>& lock)
    {
        if (lock.owns_lock())
            aux::threading::condvar::wait(cv_, *lock.mutex()->native_handle());
    }

    condition_variable::native_handle_type condition_variable::native_handle()
    {
        return &cv_;
    }

    condition_variable_any::condition_variable_any()
        : cv_{}
    {
        aux::threading::condvar::init(cv_);
    }

    condition_variable_any::~condition_variable_any()
    { /* DUMMY BODY */ }

    void condition_variable_any::notify_one() noexcept
    {
        aux::threading::condvar::signal(cv_);
    }

    void condition_variable_any::notify_all() noexcept
    {
        aux::threading::condvar::broadcast(cv_);
    }

    condition_variable_any::native_handle_type condition_variable_any::native_handle()
    {
        return &cv_;
    }

    void notify_all_at_thread_exit(condition_variable&, unique_lock<mutex>&)
    {
        // TODO: implement
        __unimplemented();
    }
}
