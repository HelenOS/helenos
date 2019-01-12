/*
 * Copyright (c) 2019 Jaroslav Jindrak
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
