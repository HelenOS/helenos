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

#ifndef LIBCPP_BITS_MEMORY_UNIQUE_PTR
#define LIBCPP_BITS_MEMORY_UNIQUE_PTR

#include <__bits/aux.hpp>
#include <__bits/functional/hash.hpp>
#include <type_traits>
#include <utility>

namespace std
{
    /**
     * 20.8, smart pointers:
     */

    template<class T>
    struct default_delete
    {
        default_delete() noexcept = default;

        template<class U, class = enable_if_t<is_convertible_v<U*, T*>, void>>
        default_delete(const default_delete<U>&) noexcept
        { /* DUMMY BODY */ }

        template<class U>
        void operator()(U* ptr)
        {
            delete ptr;
        }
    };

    template<class T>
    struct default_delete<T[]>
    {
        default_delete() noexcept = default;

        template<class U, class = enable_if_t<is_convertible_v<U(*)[], T(*)[]>, void>>
        default_delete(const default_delete<U[]>&) noexcept
        { /* DUMMY BODY */ }

        template<class U, class = enable_if_t<is_convertible_v<U(*)[], T(*)[]>, void>>
        void operator()(U* ptr)
        {
            delete[] ptr;
        }
    };

    template<class T, class D = default_delete<T>>
    class unique_ptr;

    namespace aux
    {
        template<class P, class D, class = void>
        struct get_unique_pointer: type_is<typename P::element_type*>
        { /* DUMMY BODY */ };

        template<class P, class D>
        struct get_unique_pointer<P, D, void_t<typename remove_reference_t<D>::pointer>>
            : type_is<typename remove_reference_t<D>::pointer>
        { /* DUMMY BODY */ };
    }

    template<class T, class D>
    class unique_ptr
    {
        public:
            using element_type = T;
            using deleter_type = D;
            using pointer      = typename aux::get_unique_pointer<unique_ptr<T, D>, D>::type;

            /**
             * 20.8.1.2.1, constructors:
             */

            constexpr unique_ptr() noexcept
                : ptr_{}, deleter_{}
            { /* DUMMY BODY */ }

            explicit unique_ptr(pointer ptr) noexcept
                : ptr_{ptr}, deleter_{}
            { /* DUMMY BODY */ }

            unique_ptr(pointer ptr, /* TODO */ int d) noexcept;

            unique_ptr(pointer ptr, /* TODO */ char d) noexcept;

            unique_ptr(unique_ptr&& other)
                : ptr_{move(other.ptr_)}, deleter_{forward<deleter_type>(other.deleter_)}
            {
                other.ptr_ = nullptr;
            }

            constexpr unique_ptr(nullptr_t)
                : unique_ptr{}
            { /* DUMMY BODY */ }

            template<
                class U, class E,
                class = enable_if_t<
                    is_convertible_v<
                        typename unique_ptr<U, E>::pointer,
                        pointer
                    >, void
                >
            >
            unique_ptr(unique_ptr<U, E>&& other) noexcept
                : ptr_{move(other.ptr_)}, deleter_{forward<D>(other.deleter_)}
            {
                other.ptr_ = nullptr;
            }

            /**
             * 20.8.1.2.2, destructor:
             */

            ~unique_ptr()
            {
                if (ptr_)
                    deleter_(ptr_);
            }

            /**
             * 20.8.1.2.3, assignment:
             */

            unique_ptr& operator=(unique_ptr&& rhs) noexcept
            {
                reset(rhs.release());
                deleter_ = forward<deleter_type>(rhs.get_deleter());

                return *this;
            }

            template<
                class U, class E,
                class = enable_if_t<
                    is_convertible_v<
                        typename unique_ptr<U, E>::pointer,
                        pointer
                    > && !is_array_v<U>, void
                >
            >
            unique_ptr& operator=(unique_ptr<U, E>&& rhs) noexcept
            {
                reset(rhs.release());
                deleter_ = forward<E>(rhs.get_deleter());

                return *this;
            }

            unique_ptr& operator=(nullptr_t) noexcept
            {
                reset();

                return *this;
            }

            /**
             * 20.8.1.2.4, observers:
             */

            add_lvalue_reference_t<element_type> operator*() const
            {
                return *ptr_;
            }

            pointer operator->() const noexcept
            {
                return ptr_;
            }

            pointer get() const noexcept
            {
                return ptr_;
            }

            deleter_type& get_deleter() noexcept
            {
                return deleter_;
            }

            const deleter_type& get_deleter() const noexcept
            {
                return deleter_;
            }

            explicit operator bool() const noexcept
            {
                return ptr_ != nullptr;
            }

            /**
             * 20.8.1.2.5, modifiers:
             */

            pointer release() noexcept
            {
                auto ret = ptr_;
                ptr_ = nullptr;

                return ret;
            }

            void reset(pointer ptr = pointer{}) noexcept
            {
                /**
                 * Note: Order is significant, deleter may delete
                 *       *this.
                 */
                auto old = ptr_;
                ptr_ = ptr;

                if (old)
                    deleter_(old);
            }

            void swap(unique_ptr& other) noexcept
            {
                std::swap(ptr_, other.ptr_);
                std::swap(deleter_, other.deleter_);
            }

            unique_ptr(const unique_ptr&) = delete;
            unique_ptr& operator=(const unique_ptr&) = delete;

        private:
            pointer ptr_;
            deleter_type deleter_;
    };

    namespace aux
    {
        template<class From, class To>
        struct is_convertible_array: is_convertible<From(*)[], To(*)[]>
        { /* DUMMY BODY */ };

        template<class From, class To>
        inline constexpr bool is_convertible_array_v = is_convertible_array<From, To>::value;

        template<class T, class D, class U, class E>
        struct compatible_ptrs: integral_constant<
              bool,
              is_array_v<U> && is_same_v<
                typename unique_ptr<T, D>::pointer,
                typename unique_ptr<T, D>::element_type*
              > && is_same_v<
                typename unique_ptr<U, E>::pointer,
                typename unique_ptr<U, E>::element_type*
              > && is_convertible_array_v<
                typename unique_ptr<T, D>::element_type,
                typename unique_ptr<U, E>::element_type
              > && ((is_reference_v<D> && is_same_v<D, E>) ||
                   (!is_reference_v<D> && is_convertible_v<E, D>))
        >
        { /* DUMMY BODY */ };

        template<class T, class D, class U, class E>
        inline constexpr bool compatible_ptrs_v = compatible_ptrs<T, D, U, E>::value;
    }

    template<class T, class D>
    class unique_ptr<T[], D>
    {
        public:
            using element_type = T;
            using deleter_type = D;
            using pointer      = typename aux::get_unique_pointer<unique_ptr<T[], D>, D>::type;

            /**
             * 20.8.1.3.1, constructors:
             */

            constexpr unique_ptr() noexcept
                : ptr_{}, deleter_{}
            { /* DUMMY BODY */ }

            template<
                class U,
                class = enable_if_t<
                    is_same_v<U, T> || aux::is_convertible_array_v<U, T>, void
                >
            >
            explicit unique_ptr(U ptr) noexcept
                : ptr_{ptr}, deleter_{}
            { /* DUMMY BODY */ }

            template<
                class U, class E,
                class = enable_if_t<aux::compatible_ptrs_v<T, D, U, E>, void>
            >
            unique_ptr(U ptr, /* TODO */ int d) noexcept;

            template<
                class U, class E,
                class = enable_if_t<aux::compatible_ptrs_v<T, D, U, E>, void>
            >
            unique_ptr(U ptr, /* TODO */ char d) noexcept;

            unique_ptr(unique_ptr&& other) noexcept
                : ptr_{move(other.ptr_)}, deleter_{forward<deleter_type>(other.deleter_)}
            {
                other.ptr_ = nullptr;
            }

            template<
                class U, class E,
                class = enable_if_t<
                    is_same_v<U, pointer> ||
                    (is_same_v<pointer, element_type*> && is_pointer_v<U> &&
                     aux::is_convertible_array_v<remove_pointer_t<U>, element_type>),
                    void
                >
            >
            unique_ptr(unique_ptr<U, E>&& other) noexcept
                : ptr_{move(other.ptr_)}, deleter_{forward<D>(other.deleter_)}
            {
                other.ptr_ = nullptr;
            }

            constexpr unique_ptr(nullptr_t) noexcept
                : unique_ptr{}
            { /* DUMMY BODY */ }

            ~unique_ptr()
            {
                if (ptr_)
                    deleter_(ptr_);
            }

            /**
             * 20.8.1.3.2, assignment:
             */

            unique_ptr& operator=(unique_ptr&& rhs) noexcept
            {
                reset(rhs.release());
                deleter_ = forward<deleter_type>(rhs.get_deleter());

                return *this;
            }

            template<
                class U, class E,
                class = enable_if_t<aux::compatible_ptrs_v<T, D, U, E>, void>
            >
            unique_ptr& operator=(unique_ptr<U, E>&& rhs) noexcept
            {
                reset(rhs.release());
                deleter_ = forward<E>(rhs.get_deleter());

                return *this;
            }

            unique_ptr& operator=(nullptr_t) noexcept
            {
                reset();

                return *this;
            }

            /**
             * 20.8.1.3.3, observers:
             */

            element_type& operator[](size_t idx) const
            {
                return ptr_[idx];
            }

            pointer get() const noexcept
            {
                return ptr_;
            }

            deleter_type& get_deleter() noexcept
            {
                return deleter_;
            }

            const deleter_type& get_deleter() const noexcept
            {
                return deleter_;
            }

            explicit operator bool() const noexcept
            {
                return ptr_ != nullptr;
            }

            /**
             * 20.8.1.3.4, modifiers:
             */

            pointer release() noexcept
            {
                auto ret = ptr_;
                ptr_ = nullptr;

                return ret;
            }

            template<
                class U,
                class = enable_if_t<
                    is_same_v<U, pointer> ||
                    (is_same_v<pointer, element_type*> && is_pointer_v<U> &&
                     aux::is_convertible_array_v<remove_pointer_t<U>, element_type>),
                    void
                >
            >
            void reset(U ptr) noexcept
            {
                /**
                 * Note: Order is significant, deleter may delete
                 *       *this.
                 */
                auto old = ptr_;
                ptr_ = ptr;

                if (old)
                    deleter_(old);
            }

            void reset(nullptr_t = nullptr) noexcept
            {
                reset(pointer{});
            }

            void swap(unique_ptr& other) noexcept
            {
                std::swap(ptr_, other.ptr_);
                std::swap(deleter_, other.deleter_);
            }

            unique_ptr(const unique_ptr&) = delete;
            unique_ptr& operator=(const unique_ptr&) = delete;

        private:
            pointer ptr_;
            deleter_type deleter_;
    };

    namespace aux
    {
        template<class T>
        struct is_unbound_array: false_type
        { /* DUMMY BODY */ };

        template<class T>
        struct is_unbound_array<T[]>: true_type
        { /* DUMMY BODY */ };

        template<class T>
        struct is_bound_array: false_type
        { /* DUMMY BODY */ };

        template<class T, size_t N>
        struct is_bound_array<T[N]>: true_type
        { /* DUMMY BODY */ };
    }

    template<
        class T, class... Args,
        class = enable_if_t<!is_array_v<T>, void>
    >
    unique_ptr<T> make_unique(Args&&... args)
    {
        return unique_ptr<T>(new T(forward<Args>(args)...));
    }

    template<
        class T, class = enable_if_t<aux::is_unbound_array<T>::value, void>
    >
    unique_ptr<T> make_unique(size_t n)
    {
        return unique_ptr<T>(new remove_extent_t<T>[n]());
    }

    template<
        class T, class... Args,
        class = enable_if_t<aux::is_bound_array<T>::value, void>
    >
    void make_unique(Args&&...) = delete;

    template<class T, class D>
    void swap(unique_ptr<T, D>& lhs, unique_ptr<T, D>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template<class T1, class D1, class T2, class D2>
    bool operator==(const unique_ptr<T1, D1>& lhs,
                    const unique_ptr<T2, D2>& rhs)
    {
        return lhs.get() == rhs.get();
    }

    template<class T1, class D1, class T2, class D2>
    bool operator!=(const unique_ptr<T1, D1>& lhs,
                    const unique_ptr<T2, D2>& rhs)
    {
        return lhs.get() != rhs.get();
    }

    template<class T1, class D1, class T2, class D2>
    bool operator<(const unique_ptr<T1, D1>& lhs,
                   const unique_ptr<T2, D2>& rhs)
    {
        return lhs.get() < rhs.get();
    }

    template<class T1, class D1, class T2, class D2>
    bool operator<=(const unique_ptr<T1, D1>& lhs,
                    const unique_ptr<T2, D2>& rhs)
    {
        return !(rhs < lhs);
    }

    template<class T1, class D1, class T2, class D2>
    bool operator>(const unique_ptr<T1, D1>& lhs,
                   const unique_ptr<T2, D2>& rhs)
    {
        return rhs < lhs;
    }

    template<class T1, class D1, class T2, class D2>
    bool operator>=(const unique_ptr<T1, D1>& lhs,
                    const unique_ptr<T2, D2>& rhs)
    {
        return !(lhs < rhs);
    }

    template<class T, class D>
    bool operator==(const unique_ptr<T, D>& ptr, nullptr_t) noexcept
    {
        return !ptr;
    }

    template<class T, class D>
    bool operator==(nullptr_t, const unique_ptr<T, D>& ptr) noexcept
    {
        return !ptr;
    }

    template<class T, class D>
    bool operator!=(const unique_ptr<T, D>& ptr, nullptr_t) noexcept
    {
        return static_cast<bool>(ptr);
    }

    template<class T, class D>
    bool operator!=(nullptr_t, const unique_ptr<T, D>& ptr) noexcept
    {
        return static_cast<bool>(ptr);
    }

    template<class T, class D>
    bool operator<(const unique_ptr<T, D>& ptr, nullptr_t)
    {
        return ptr.get() < nullptr;
    }

    template<class T, class D>
    bool operator<(nullptr_t, const unique_ptr<T, D>& ptr)
    {
        return nullptr < ptr.get();
    }

    template<class T, class D>
    bool operator<=(const unique_ptr<T, D>& ptr, nullptr_t)
    {
        return !(nullptr < ptr);
    }

    template<class T, class D>
    bool operator<=(nullptr_t, const unique_ptr<T, D>& ptr)
    {
        return !(ptr < nullptr);
    }

    template<class T, class D>
    bool operator>(const unique_ptr<T, D>& ptr, nullptr_t)
    {
        return nullptr < ptr;
    }

    template<class T, class D>
    bool operator>(nullptr_t, const unique_ptr<T, D>& ptr)
    {
        return ptr < nullptr;
    }

    template<class T, class D>
    bool operator>=(const unique_ptr<T, D>& ptr, nullptr_t)
    {
        return !(ptr < nullptr);
    }

    template<class T, class D>
    bool operator>=(nullptr_t, const unique_ptr<T, D>& ptr)
    {
        return !(nullptr < ptr);
    }

    /**
     * 20.8.2.7, smart pointer hash support:
     */

    template<class T, class D>
    struct hash<unique_ptr<T, D>>
    {
        size_t operator()(const unique_ptr<T, D>& ptr) const noexcept
        {
            return hash<typename unique_ptr<T, D>::pointer>{}(ptr.get());
        }

        using argument_type = unique_ptr<T, D>;
        using result_type   = size_t;
    };
}

#endif
