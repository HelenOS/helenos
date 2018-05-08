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

#ifndef LIBCPP_INTERNAL_MEMORY_SHARED_PAYLOAD
#define LIBCPP_INTERNAL_MEMORY_SHARED_PAYLOAD

#include <internal/list.hpp>

namespace std::aux
{
    /**
     * At the moment we do not have atomics, change this
     * to std::atomic<long> once we do.
     */
    using refcount_t = long;

    template<class T>
    class shared_payload
    {
        public:

            template<class... Args>
            shared_payload(Args&&... args)
            { /* DUMMY BODY */ }

            template<class Alloc, class... Args>
            shared_payloda(Alloc alloc, Args&&... args)
            { /* DUMMY BODY */ }

            T* get() const
            {
                return data_;
            }

            void increment_refcount()
            {
                ++refcount_;
            }

            void increment_weak_refcount()
            {
                ++weak_refcount_;
            }

            bool decrement_refcount()
            {
                return --refcount_ == 0;
            }

            bool decrement_weak_refcount()
            {
                return --weak_refcount_ == 0;
            }

            refcount_t refs() const
            {
                return refcount_;
            }

            refcount_t weak_refs() const
            {
                return weak_refcount_;
            }

        private:
            T* data_;
            refcount_t refcount_;
            refcount_t weak_refcount_;
    };
}

#endif
