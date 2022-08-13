/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_MEMORY_WEAK_PTR
#define LIBCPP_BITS_MEMORY_WEAK_PTR

#include <__bits/memory/shared_payload.hpp>
#include <type_traits>
#include <utility>

namespace std
{
    template<class T>
    class shared_ptr;

    /**
     * 20.8.2.3, class template weak_ptr:
     */

    template<class T>
    class weak_ptr
    {
        public:
            using element_type = T;

            /**
             * 20.8.2.3.1, constructors:
             */

            constexpr weak_ptr() noexcept
                : payload_{}
            { /* DUMMY BODY */ }

            template<class U>
            weak_ptr(
                const shared_ptr<U>& other,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            ) noexcept
                : payload_{other.payload_}
            {
                if (payload_)
                    payload_->increment_weak();
            }

            weak_ptr(const weak_ptr& other) noexcept
                : payload_{other.payload_}
            {
                if (payload_)
                    payload_->increment_weak();
            }

            template<class U>
            weak_ptr(
                const weak_ptr<U>& other,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            ) noexcept
                : payload_{other.payload_}
            {
                if (payload_)
                    payload_->increment_weak();
            }

            weak_ptr(weak_ptr&& other) noexcept
                : payload_{other.payload_}
            {
                other.payload_ = nullptr;
            }

            template<class U>
            weak_ptr(
                weak_ptr<U>&& other,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            ) noexcept
                : payload_{other.payload_}
            {
                other.payload_ = nullptr;
            }

            /**
             * 20.8.2.3.2, destructor:
             */

            ~weak_ptr()
            {
                remove_payload_();
            }

            /**
             * 20.8.2.3.3, assignment:
             */

            weak_ptr& operator=(const weak_ptr& rhs) noexcept
            {
                remove_payload_();

                payload_ = rhs.payload_;
                if (payload_)
                    payload_->increment_weak();

                return *this;
            }

            template<class U>
            weak_ptr& operator=(const weak_ptr<U>& rhs) noexcept
            {
                weak_ptr{rhs}.swap(*this);

                return *this;
            }

            template<class U>
            weak_ptr& operator=(const shared_ptr<U>& rhs) noexcept
            {
                weak_ptr{rhs}.swap(*this);

                return *this;
            }

            weak_ptr& operator=(weak_ptr&& rhs) noexcept
            {
                weak_ptr{move(rhs)}.swap(*this);

                return *this;
            }

            template<class U>
            weak_ptr& operator=(
                weak_ptr<U>&& rhs
            ) noexcept
            {
                weak_ptr{move(rhs)}.swap(*this);

                return *this;
            }

            /**
             * 20.8.2.3.4, modifiers:
             */

            void swap(weak_ptr& other) noexcept
            {
                std::swap(payload_, other.payload_);
            }

            void reset() noexcept
            {
                weak_ptr{}.swap(*this);
            }

            /**
             * 20.8.2.3.5, observers:
             */

            long use_count() const noexcept
            {
                if (payload_)
                    return payload_->refs();
                else
                    return 0L;
            }

            bool expired() const noexcept
            {
                return use_count() == 0L;
            }

            shared_ptr<T> lock() const noexcept
            {
                return shared_ptr{aux::payload_tag, payload_->lock()};
            }

            template<class U>
            bool owner_before(const shared_ptr<U>& other) const
            {
                return payload_ < other.payload_;
            }

            template<class U>
            bool owner_before(const weak_ptr<U>& other) const
            {
                return payload_ < other.payload_;
            }

        private:
            aux::shared_payload_base<T>* payload_;

            void remove_payload_()
            {
                if (payload_ && payload_->decrement_weak())
                    payload_->destroy();
                payload_ = nullptr;
            }

            template<class U>
            friend class shared_ptr;
    };

    /**
     * 20.8.2.3.6, specialized algorithms:
     */

    template<class T>
    void swap(weak_ptr<T>& lhs, weak_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}

#endif
