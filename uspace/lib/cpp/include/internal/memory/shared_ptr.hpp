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

#ifndef LIBCPP_INTERNAL_MEMORY_SHARED_PTR
#define LIBCPP_INTERNAL_MEMORY_SHARED_PTR

#include <exception>
#include <functional>
#include <type_traits>

namespace std
{
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

            constexpr shared_ptr() noexcept;

            template<class U>
            explicit shared_ptr(U* ptr);

            template<class U, class D>
            shared_ptr(U* ptr, D deleter);

            template<class U, class D, class A>
            shared_ptr(U* ptr, D deleter, A alloc);

            template<class D>
            shared_ptr(nullptr_t, D deleter);

            template<class D, class A>
            shared_ptr(nullptr_t, D deleter, A alloc);

            template<class U>
            shared_ptr(const shared_ptr<U>& other, value_type* ptr);

            shared_ptr(const shared_ptr& other);

            template<class U>
            shared_ptr(const shared_ptr<U>& other);

            shared_ptr(shared_ptr&& other);

            template<class U>
            shared_ptr(shared_ptr<U>&& other);

            template<class U>
            explicit shared_ptr(const weak_ptr<U>& other);

            template<class U, class D>
            shared_ptr(unique_ptr<U, D>&& other);

            constexpr shared_ptr(nullptr_t) noexcept
                : shared_ptr{}
            { /* DUMMY BODY */ }

            /**
             * 20.8.2.2.2, destructor:
             */

            ~shared_ptr();

            /**
             * 20.8.2.2.3, assignment:
             */

            shared_ptr& operator=(const shared_ptr& rhs) noexcept;

            template<class U>
            shared_ptr& operator=(const shared_ptr<U>& rhs) noexcept;

            shared_ptr& operator=(shared_ptr&& rhs) noexcept;

            template<class U>
            shared_ptr& operator=(shared_ptr<U>&& rhs) noexcept;

            template<class U, class D>
            shared_ptr& operator=(unique_ptr<U, D>&& rhs);

            /**
             * 20.8.2.2.4, modifiers:
             */

            void swap(shared_ptr& other) noexcept;

            void reset() noexcept;

            template<class U>
            void reset(U* ptr);

            template<class U, class D>
            void reset(U* ptr, D deleter);

            template<class U, class D, class A>
            void reset(U* ptr, D deleter, A alloc);

            /**
             * 20.8.2.2.5, observers:
             */

            element_type* get() const noexcept
            {
                if (payload_)
                    return payload_->get();
                else
                    return nullptr;
            }

            T& operator*() const noexcept;

            T* operator->() const noexcept
            {
                return get();
            }

            long use_count() const noexcept
            {
                if (payload_)
                    return payload_->refcount();
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
            bool owner_before(const weak_ptr<U>& ptr) const;

        private:
            aux::shared_payload<element_type>* payload_;

            shared_ptr(aux::shared_payload<element_type>* payload)
                : payload_{payload}
            { /* DUMMY BODY */ }

            template<class U, class... Args>
            friend shared_ptr<U> make_shared(Args&&...);

            template<class U, class A, class... Args>
            friend shared_ptr<U> allocate_shared(const A&, Args&&...);
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
            new aux::shared_payload<T>{forward<Args>(args)...}
        };
    }

    template<class T, class A, class... Args>
    shared_ptr<T> allocate_shared(const A& alloc, Args&&... args)
    {
        return shared_ptr<T>{
            new aux::shared_payload<T>{A{alloc}, forward<Args>(args)...}
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
        // TODO: implement this through payload
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
}

#endif
