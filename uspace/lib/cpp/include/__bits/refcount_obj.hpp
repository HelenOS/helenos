/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_REFCOUNT_OBJ
#define LIBCPP_BITS_REFCOUNT_OBJ

namespace std::aux
{
    /**
     * At the moment we do not have atomics, change this
     * to std::atomic<long> once we do.
     */
    using refcount_t = long;

    class refcount_obj
    {
        public:
            refcount_obj() = default;

            void increment() noexcept;
            void increment_weak() noexcept;
            bool decrement() noexcept;
            bool decrement_weak() noexcept;
            refcount_t refs() const noexcept;
            refcount_t weak_refs() const noexcept;
            bool expired() const noexcept;

            virtual ~refcount_obj() = default;
            virtual void destroy() = 0;

        protected:
            /**
             * We're using a trick where refcount_ > 0
             * means weak_refcount_ has 1 added to it,
             * this makes it easier for weak_ptrs that
             * can't decrement the weak_refcount_ to
             * zero with shared_ptrs using this object.
             */
            refcount_t refcount_{1};
            refcount_t weak_refcount_{1};
    };
}

#endif
