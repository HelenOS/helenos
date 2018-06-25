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
