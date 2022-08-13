/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_MEMORY_SHARED_PAYLOAD
#define LIBCPP_BITS_MEMORY_SHARED_PAYLOAD

#include <__bits/refcount_obj.hpp>
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
    class shared_payload_base: public aux::refcount_obj
    {
        public:
            virtual T* get() const noexcept = 0;

            virtual uint8_t* deleter() const noexcept = 0;

            virtual shared_payload_base* lock() noexcept = 0;

            virtual ~shared_payload_base() = default;
    };

    template<class T, class D = default_delete<T>>
    class shared_payload: public shared_payload_base<T>
    {
        public:
            shared_payload(T* ptr, D deleter = D{})
                : data_{ptr}, deleter_{deleter}
            { /* DUMMY BODY */ }

            template<class... Args>
            shared_payload(Args&&... args)
                : data_{new T{forward<Args>(args)...}},
                  deleter_{}
            { /* DUMMY BODY */ }

            template<class Alloc, class... Args>
            shared_payload(allocator_arg_t, Alloc alloc, Args&&... args)
                : data_{alloc.allocate(1)},
                  deleter_{}
            {
                alloc.construct(data_, forward<Args>(args)...);
            }

            template<class Alloc, class... Args>
            shared_payload(D deleter, Alloc alloc, Args&&... args)
                : data_{alloc.allocate(1)},
                  deleter_{deleter}
            {
                alloc.construct(data_, forward<Args>(args)...);
            }

            void destroy() override
            {
                if (this->refs() == 0)
                {
                    if (data_)
                    {
                        deleter_(data_);
                        data_ = nullptr;
                    }

                    if (this->weak_refs() == 0)
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

            shared_payload_base<T>* lock() noexcept override
            {
                refcount_t rfs = this->refs();
                while (rfs != 0L)
                {
                    if (__atomic_compare_exchange_n(&this->refcount_, &rfs, rfs + 1,
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
    };
}

#endif
