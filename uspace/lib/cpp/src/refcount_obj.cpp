/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
