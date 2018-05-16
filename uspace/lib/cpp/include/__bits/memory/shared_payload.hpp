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

#ifndef LIBCPP_BITS_MEMORY_SHARED_PAYLOAD
#define LIBCPP_BITS_MEMORY_SHARED_PAYLOAD

#include <cinttypes>
#include <utility>

namespace std
{
    template<class>
    struct default_delete;

    struct allocator_arg_t;
}

namespace std::aux
{
    /**
     * At the moment we do not have atomics, change this
     * to std::atomic<long> once we do.
     */
    using refcount_t = long;

    /**
     * This allows us to construct shared_ptr from
     * a payload pointer in make_shared etc.
     */
    struct payload_tag_t
    { /* DUMMY BODY */ };

    inline constexpr payload_tag_t payload_tag{};

    template<class D, class T>
    void use_payload_deleter(D* deleter, T* data)
    {
        if (deleter)
            (*deleter)(data);
    }

    template<class T>
    class shared_payload_base
    {
        public:
            virtual void destroy() = 0;
            virtual T* get() const noexcept = 0;

            virtual uint8_t* deleter() const noexcept = 0;

            virtual void increment() noexcept = 0;
            virtual void increment_weak() noexcept = 0;
            virtual bool decrement() noexcept = 0;
            virtual bool decrement_weak() noexcept = 0;
            virtual refcount_t refs() const noexcept = 0;
            virtual refcount_t weak_refs() const noexcept = 0;
            virtual bool expired() const noexcept = 0;
            virtual shared_payload_base* lock() noexcept = 0;

            virtual ~shared_payload_base() = default;
    };

    template<class T, class D = default_delete<T>>
    class shared_payload: public shared_payload_base<T>
    {
        public:
            shared_payload(T* ptr, D deleter = D{})
                : data_{ptr}, deleter_{deleter},
                  refcount_{1}, weak_refcount_{1}
            { /* DUMMY BODY */ }

            template<class... Args>
            shared_payload(Args&&... args)
                : data_{new T{forward<Args>(args)...}},
                  deleter_{}, refcount_{1}, weak_refcount_{1}
            { /* DUMMY BODY */ }

            template<class Alloc, class... Args>
            shared_payload(allocator_arg_t, Alloc alloc, Args&&... args)
                : data_{alloc.allocate(1)},
                  deleter_{}, refcount_{1}, weak_refcount_{1}
            {
                alloc.construct(data_, forward<Args>(args)...);
            }

            template<class Alloc, class... Args>
            shared_payload(D deleter, Alloc alloc, Args&&... args)
                : data_{alloc.allocate(1)},
                  deleter_{deleter}, refcount_{1}, weak_refcount_{1}
            {
                alloc.construct(data_, forward<Args>(args)...);
            }

            void destroy() override
            {
                if (refs() == 0)
                {
                    if (data_)
                    {
                        deleter_(data_);
                        data_ = nullptr;
                    }

                    if (weak_refs() == 0)
                        delete this;
                }
            }

            T* get() const noexcept override
            {
                return data_;
            }

            uint8_t* deleter() const noexcept override
            {
                return (uint8_t*)&deleter_;
            }

            void increment() noexcept override
            {
                __atomic_add_fetch(&refcount_, 1, __ATOMIC_ACQ_REL);
            }

            void increment_weak() noexcept override
            {
                __atomic_add_fetch(&weak_refcount_, 1, __ATOMIC_ACQ_REL);
            }

            bool decrement() noexcept override
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

            bool decrement_weak() noexcept override
            {
                return __atomic_sub_fetch(&weak_refcount_, 1, __ATOMIC_ACQ_REL) == 0 && refs() == 0;
            }

            refcount_t refs() const noexcept override
            {
                return __atomic_load_n(&refcount_, __ATOMIC_RELAXED);
            }

            refcount_t weak_refs() const noexcept override
            {
                return __atomic_load_n(&weak_refcount_, __ATOMIC_RELAXED);
            }

            bool expired() const noexcept override
            {
                return refs() == 0;
            }

            shared_payload_base<T>* lock() noexcept override
            {
                refcount_t rfs = refs();
                while (rfs != 0L)
                {
                    if (__atomic_compare_exchange_n(&refcount_, &rfs, rfs + 1,
                                                    true, __ATOMIC_RELAXED,
                                                    __ATOMIC_RELAXED))
                    {
                        return this;
                    }
                }

                return nullptr;
            }

        private:
            T* data_;
            D deleter_;

            /**
             * We're using a trick where refcount_ > 0
             * means weak_refcount_ has 1 added to it,
             * this makes it easier for weak_ptrs that
             * can't decrement the weak_refcount_ to
             * zero with shared_ptrs using this object.
             */
            refcount_t refcount_;
            refcount_t weak_refcount_;
    };
}

#endif
