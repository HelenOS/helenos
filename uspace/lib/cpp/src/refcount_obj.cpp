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

#include <__bits/refcount_obj.hpp>

namespace std::aux
{
    void refcount_obj::increment()
    {
        __atomic_add_fetch(&refcount_, 1, __ATOMIC_ACQ_REL);
    }

    void refcount_obj::increment_weak()
    {
        __atomic_add_fetch(&weak_refcount_, 1, __ATOMIC_ACQ_REL);
    }

    bool refcount_obj::decrement()
    {
        if (__atomic_sub_fetch(&refcount_, 1, __ATOMIC_ACQ_REL) == 0)
        {
            /**
             * First call to destroy() will delete the held object,
             * so it doesn't matter what the weak_refcount_ is,
             * but we added one and we need to remove it now.
             */
            decrement_weak();

            return true;
        }
        else
            return false;
    }

    bool refcount_obj::decrement_weak()
    {
        return __atomic_sub_fetch(&weak_refcount_, 1, __ATOMIC_ACQ_REL) == 0 && refs() == 0;
    }

    refcount_t refcount_obj::refs() const
    {
        return __atomic_load_n(&refcount_, __ATOMIC_RELAXED);
    }

    refcount_t refcount_obj::weak_refs() const
    {
        return __atomic_load_n(&weak_refcount_, __ATOMIC_RELAXED);
    }

    bool refcount_obj::expired() const
    {
        return refs() == 0;
    }
}
