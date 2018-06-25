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

#ifndef LIBCPP_BITS_MEMORY_SHARED_PTR
#define LIBCPP_BITS_MEMORY_SHARED_PTR

#include <__bits/functional/arithmetic_operations.hpp>
#include <__bits/functional/hash.hpp>
#include <__bits/memory/allocator_arg.hpp>
#include <__bits/memory/shared_payload.hpp>
#include <__bits/memory/unique_ptr.hpp>
#include <__bits/trycatch.hpp>
#include <exception>
#include <type_traits>

namespace std
{
    template<class T>
    class weak_ptr;

    /**
     * 20.8.2.1, class bad_weak_ptr:
     */

    class bad_weak_ptr: public exception
    {
        public:
            bad_weak_ptr() noexcept = default;

            const char* what() const noexcept override
            {
                return "std::bad_weak_ptr";
            }
    };

    /**
     * 20.8.2.2, class template shared_ptr:
     */

    template<class T>
    class shared_ptr
    {
        public:
            using element_type = T;

            /**
             * 20.8.2.2.1, constructors:
             */

            constexpr shared_ptr() noexcept
                : payload_{}, data_{}
            { /* DUMMY BODY */ }

            template<class U>
            explicit shared_ptr(
                U* ptr,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            )
                : payload_{}, data_{ptr}
            {
                try
                {
                    payload_ = new aux::shared_payload<T>{ptr};
                }
                catch (const bad_alloc&)
                {
                    delete ptr;

                    throw;
                }
            }

            template<class U, class D>
            shared_ptr(
                U* ptr, D deleter,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            )
                : shared_ptr{}
            {
                try
                {
                    payload_ = new aux::shared_payload<T, D>{ptr, deleter};
                }
                catch (const bad_alloc&)
                {
                    deleter(ptr);

                    throw;
                }
            }

            template<class U, class D, class A>
            shared_ptr(
                U* ptr, D deleter, A,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            )
                : shared_ptr{}
            {
                try
                {
                    payload_ = new aux::shared_payload<T, D>{ptr, deleter};
                }
                catch (const bad_alloc&)
                {
                    deleter(ptr);

                    throw;
                }
            }

            template<class D>
            shared_ptr(nullptr_t ptr, D deleter)
                : shared_ptr{}
            { /* DUMMY BODY */ }

            template<class D, class A>
            shared_ptr(nullptr_t, D deleter, A)
                : shared_ptr{}
            { /* DUMMY BODY */ }

            template<class U>
            shared_ptr(
                const shared_ptr<U>& other, element_type* ptr,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            )
                : payload_{other.payload_}, data_{ptr}
            {
                if (payload_)
                    payload_->increment();
            }

            shared_ptr(const shared_ptr& other)
                : payload_{other.payload_}, data_{other.data_}
            {
                if (payload_)
                    payload_->increment();
            }

            template<class U>
            shared_ptr(
                const shared_ptr<U>& other,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            )
                : payload_{other.payload_}, data_{other.data_}
            {
                if (payload_)
                    payload_->increment();
            }

            shared_ptr(shared_ptr&& other)
                : payload_{move(other.payload_)}, data_{move(other.data_)}
            {
                other.payload_ = nullptr;
                other.data_ = nullptr;
            }

            template<class U>
            shared_ptr(
                shared_ptr<U>&& other,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            )
                : payload_{move(other.payload_)}, data_{move(other.data_)}
            {
                other.payload_ = nullptr;
                other.data_ = nullptr;
            }

            template<class U>
            explicit shared_ptr(
                const weak_ptr<U>& other,
                enable_if_t<is_convertible_v<U*, element_type*>>* = nullptr
            )
                : payload_{}, data_{}
            {
                if (other.expired())
                    throw bad_weak_ptr{};

                if (other.payload_)
                {
                    payload_ = other.payload_->lock();
                    data_ = payload_->get();
                }
            }

            template<class U, class D>
            shared_ptr(
                unique_ptr<U, D>&& other,
                enable_if_t<
                    is_convertible_v<
                        typename unique_ptr<U, D>::pointer,
                        element_type*
                    >
                >* = nullptr
            ) // TODO: if D is a reference type, it should be ref(other.get_deleter())
                : shared_ptr{other.release(), other.get_deleter()}
            { /* DUMMY BODY */ }

            constexpr shared_ptr(nullptr_t) noexcept
                : shared_ptr{}
            { /* DUMMY BODY */ }

            /**
             * 20.8.2.2.2, destructor:
             */

            ~shared_ptr()
            {
                remove_payload_();
            }

            /**
             * 20.8.2.2.3, assignment:
             */

            shared_ptr& operator=(const shared_ptr& rhs) noexcept
            {
                if (rhs.payload_)
                    rhs.payload_->increment();

                remove_payload_();

                payload_ = rhs.payload_;
                data_ = rhs.data_;

                return *this;
            }

            template<class U>
            enable_if_t<is_convertible_v<U*, element_type*>, shared_ptr>&
            operator=(const shared_ptr<U>& rhs) noexcept
            {
                if (rhs.payload_)
                    rhs.payload_->increment();

                remove_payload_();

                payload_ = rhs.payload_;
                data_ = rhs.data_;

                return *this;
            }

            shared_ptr& operator=(shared_ptr&& rhs) noexcept
            {
                shared_ptr{move(rhs)}.swap(*this);

                return *this;
            }

            template<class U>
            shared_ptr& operator=(shared_ptr<U>&& rhs) noexcept
            {
                shared_ptr{move(rhs)}.swap(*this);

                return *this;
            }

            template<class U, class D>
            shared_ptr& operator=(unique_ptr<U, D>&& rhs)
            {
                shared_ptr{move(rhs)}.swap(*this);

                return *this;
            }

            /**
             * 20.8.2.2.4, modifiers:
             */

            void swap(shared_ptr& other) noexcept
            {
                std::swap(payload_, other.payload_);
                std::swap(data_, other.data_);
            }

            void reset() noexcept
            {
                shared_ptr{}.swap(*this);
            }

            template<class U>
            void reset(U* ptr)
            {
                shared_ptr{ptr}.swap(*this);
            }

            template<class U, class D>
            void reset(U* ptr, D deleter)
            {
                shared_ptr{ptr, deleter}.swap(*this);
            }

            template<class U, class D, class A>
            void reset(U* ptr, D deleter, A alloc)
            {
                shared_ptr{ptr, deleter, alloc}.swap(*this);
            }

            /**
             * 20.8.2.2.5, observers:
             */

            element_type* get() const noexcept
            {
                return data_;
            }

            enable_if_t<!is_void_v<T>, T&> operator*() const noexcept
            {
                return *data_;
            }

            T* operator->() const noexcept
            {
                return get();
            }

            long use_count() const noexcept
            {
                if (payload_)
                    return payload_->refs();
                else
                    return 0L;
            }

            bool unique() const noexcept
            {
                return use_count() == 1L;
            }

            explicit operator bool() const noexcept
            {
                return get() != nullptr;
            }

            template<class U>
            bool owner_before(const shared_ptr<U>& ptr) const
            {
                return payload_ < ptr.payload_;
            }

            template<class U>
            bool owner_before(const weak_ptr<U>& ptr) const
            {
                return payload_ < ptr.payload_;
            }

        private:
            aux::shared_payload_base<element_type>* payload_;
            element_type* data_;

            shared_ptr(aux::payload_tag_t, aux::shared_payload_base<element_type>* payload)
                : payload_{payload}, data_{payload->get()}
            { /* DUMMY BODY */ }

            void remove_payload_()
            {
                if (payload_)
                {
                    auto res = payload_->decrement();
                    if (res)
                        payload_->destroy();

                    payload_ = nullptr;
                }

                if (data_)
                    data_ = nullptr;
            }

            template<class U, class... Args>
            friend shared_ptr<U> make_shared(Args&&...);

            template<class U, class A, class... Args>
            friend shared_ptr<U> allocate_shared(const A&, Args&&...);

            template<class D, class U>
            D* get_deleter(const shared_ptr<U>&) noexcept;

            template<class U>
            friend class weak_ptr;
    };

    /**
     * 20.8.2.2.6, shared_ptr creation:
     * Note: According to the standard, these two functions
     *       should perform at most one memory allocation
     *       (should, don't have to :P). It might be better
     *       to create payloads that embed the type T to
     *       perform this optimization.
     */

    template<class T, class... Args>
    shared_ptr<T> make_shared(Args&&... args)
    {
        return shared_ptr<T>{
            aux::payload_tag,
            new aux::shared_payload<T>{forward<Args>(args)...}
        };
    }

    template<class T, class A, class... Args>
    shared_ptr<T> allocate_shared(const A& alloc, Args&&... args)
    {
        return shared_ptr<T>{
            aux::payload_tag,
            new aux::shared_payload<T>{allocator_arg, A{alloc}, forward<Args>(args)...}
        };
    }

    /**
     * 20.8.2.2.7, shared_ptr comparisons:
     */

    template<class T, class U>
    bool operator==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    template<class T, class U>
    bool operator!=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    template<class T, class U>
    bool operator<(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept
    {
        return less<common_type_t<T*, U*>>{}(lhs.get(), rhs.get());
    }

    template<class T, class U>
    bool operator>(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept
    {
        return rhs < lhs;
    }

    template<class T, class U>
    bool operator<=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept
    {
        return !(rhs < lhs);
    }

    template<class T, class U>
    bool operator>=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept
    {
        return !(lhs < rhs);
    }

    template<class T>
    bool operator==(const shared_ptr<T>& lhs, nullptr_t) noexcept
    {
        return !lhs;
    }

    template<class T>
    bool operator==(nullptr_t, const shared_ptr<T>& rhs) noexcept
    {
        return !rhs;
    }

    template<class T>
    bool operator!=(const shared_ptr<T>& lhs, nullptr_t) noexcept
    {
        return (bool)lhs;
    }

    template<class T>
    bool operator!=(nullptr_t, const shared_ptr<T>& rhs) noexcept
    {
        return (bool)rhs;
    }

    template<class T>
    bool operator<(const shared_ptr<T>& lhs, nullptr_t) noexcept
    {
        return less<T*>{}(lhs.get(), nullptr);
    }

    template<class T>
    bool operator<(nullptr_t, const shared_ptr<T>& rhs) noexcept
    {
        return less<T*>{}(nullptr, rhs.get());
    }

    template<class T>
    bool operator>(const shared_ptr<T>& lhs, nullptr_t) noexcept
    {
        return nullptr < lhs;
    }

    template<class T>
    bool operator>(nullptr_t, const shared_ptr<T>& rhs) noexcept
    {
        return rhs < nullptr;
    }

    template<class T>
    bool operator<=(const shared_ptr<T>& lhs, nullptr_t) noexcept
    {
        return !(nullptr < lhs);
    }

    template<class T>
    bool operator<=(nullptr_t, const shared_ptr<T>& rhs) noexcept
    {
        return !(rhs < nullptr);
    }

    template<class T>
    bool operator>=(const shared_ptr<T>& lhs, nullptr_t) noexcept
    {
        return !(lhs < nullptr);
    }

    template<class T>
    bool operator>=(nullptr_t, const shared_ptr<T>& rhs) noexcept
    {
        return !(nullptr < rhs);
    }

    /**
     * 20.8.2.2.8, shared_ptr specialized algorithms:
     */

    template<class T>
    void swap(shared_ptr<T>& lhs, shared_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    /**
     * 20.8.2.2.9, shared_ptr casts:
     */

    template<class T, class U>
    shared_ptr<T> static_pointer_cast(const shared_ptr<U>& ptr) noexcept
    {
        if (!ptr)
            return shared_ptr<T>{};

        return shared_ptr<T>{
            ptr, static_cast<T*>(ptr.get())
        };
    }

    template<class T, class U>
    shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& ptr) noexcept
    {
        if (auto res = dynamic_cast<T*>(ptr.get()))
            return shared_ptr<T>{ptr, res};
        else
            return shared_ptr<T>{};
    }

    template<class T, class U>
    shared_ptr<T> const_pointer_cast(const shared_ptr<U>& ptr) noexcept
    {
        if (!ptr)
            return shared_ptr<T>{};

        return shared_ptr<T>{
            ptr, const_cast<T*>(ptr.get())
        };
    }

    /**
     * 20.8.2.2.10, shared_ptr get_deleter:
     */

    template<class D, class T>
    D* get_deleter(const shared_ptr<T>& ptr) noexcept
    {
        if (ptr.payload_)
            return static_cast<D*>(ptr.payload_->deleter());
        else
            return nullptr;
    }

    /**
     * 20.8.2.2.11, shared_ptr I/O:
     */

    template<class Char, class Traits, class T>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os,
                                            const shared_ptr<T>& ptr)
    {
        return os << ptr.get();
    }

    /**
     * 20.8.2.5, class template enable_shared_from_this:
     */

    // TODO: implement

    /**
     * 20.8.2.6, shared_ptr atomic access
     */

    // TODO: implement

    /**
     * 20.8.2.7, smart pointer hash support:
     */

    template<class T>
    struct hash<shared_ptr<T>>
    {
        size_t operator()(const shared_ptr<T>& ptr) const noexcept
        {
            return hash<T*>{}(ptr.get());
        }

        using argument_type = shared_ptr<T>;
        using result_type   = size_t;
    };
}

#endif
