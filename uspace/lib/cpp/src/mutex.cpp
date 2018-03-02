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

#include <mutex>

namespace std
{
    mutex::mutex()
        : mtx_{}
    {
        fibril_mutex_initialize(&mtx_);
    }

    mutex::~mutex()
    {
        if (fibril_mutex_is_locked(&mtx_))
        {
            /**
             * According to the standard, this is
             * undefined behavior, we could unlock the
             * mutex, but that could cause issues if we
             * are not the current owner.
             */
            // fibril_mutex_unlock(&mtx_);
        }
    }

    void mutex::lock()
    {
        fibril_mutex_lock(&mtx_);
    }

    bool mutex::try_lock()
    {
        return fibril_mutex_trylock(&mtx_);
    }

    void mutex::unlock()
    {
        fibril_mutex_unlock(&mtx_);
    }

    mutex::native_handle_type mutex::native_handle()
    {
        return mtx_;
    }

    recursive_mutex::recursive_mutex()
        : mtx_{}, lock_level_{}, owner_{}
    {
        fibril_mutex_initialize(&mtx_);
    }

    recursive_mutex::~recursive_mutex()
    { /* DUMMY BODY */ }

    void recursive_mutex::lock()
    {
        if (owner_ != this_thread::get_id())
        {
            fibril_mutex_lock(&mtx_);
            owner_ = this_thread::get_id();
            lock_level_ = 1;
        }
        else
            ++lock_level_;
    }

    bool recursive_mutex::try_lock()
    {
        if (owner_ != this_thread::get_id())
        {
            bool res = fibril_mutex_trylock(&mtx_);
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
            fibril_mutex_unlock(&mtx_);
    }

    recursive_mutex::native_handle_type recursive_mutex::native_handle()
    {
        return mtx_;
    }
}
