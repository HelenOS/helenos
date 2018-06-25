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

#ifndef LIBCPP_BITS_MEMORY_ALLOCATOR_TRAITS
#define LIBCPP_BITS_MEMORY_ALLOCATOR_TRAITS

#include <__bits/aux.hpp>
#include <__bits/memory/addressof.hpp>
#include <__bits/memory/pointer_traits.hpp>
#include <__bits/memory/type_getters.hpp>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace std
{
    /**
     * 20.7.7, uses_allocator:
     */

    namespace aux
    {
        template<class T, class = void>
        struct has_allocator_type: false_type
        { /* DUMMY BODY */ };

        template<class T>
        struct has_allocator_type<T, void_t<typename T::allocator_type>>
            : true_type
        { /* DUMMY BODY */ };
    }

    template<class T, class Alloc>
    struct uses_allocator
        : aux::value_is<
        bool, aux::has_allocator_type<T>::value && is_convertible_v<
            Alloc, typename T::allocator_type
        >
    >
    { /* DUMMY BODY */ };

    /**
     * 20.7.8, allocator traits:
     */

    template<class Alloc>
    struct allocator_traits
    {
        using allocator_type = Alloc;

        using value_type         = typename Alloc::value_type;
        using pointer            = typename aux::alloc_get_pointer<Alloc>::type;
        using const_pointer      = typename aux::alloc_get_const_pointer<Alloc, pointer>::type;
        using void_pointer       = typename aux::alloc_get_void_pointer<Alloc, pointer>::type;
        using const_void_pointer = typename aux::alloc_get_const_void_pointer<Alloc, pointer>::type;
        using difference_type    = typename aux::alloc_get_difference_type<Alloc, pointer>::type;
        using size_type          = typename aux::alloc_get_size_type<Alloc, difference_type>::type;

        using propagate_on_container_copy_assignment = typename aux::alloc_get_copy_propagate<Alloc>::type;
        using propagate_on_container_move_assignment = typename aux::alloc_get_move_propagate<Alloc>::type;
        using propagate_on_container_swap            = typename aux::alloc_get_swap_propagate<Alloc>::type;
        using is_always_equal                        = typename aux::alloc_get_always_equal<Alloc>::type;

        template<class T>
        using rebind_alloc = typename aux::alloc_get_rebind_alloc<Alloc, T>;

        template<class T>
        using rebind_traits = allocator_traits<rebind_alloc<T>>;

        static pointer allocate(Alloc& alloc, size_type n)
        {
            return alloc.allocate(n);
        }

        static pointer allocate(Alloc& alloc, size_type n, const_void_pointer hint)
        {
            if constexpr (aux::alloc_has_hint_allocate<Alloc, size_type, const_void_pointer>::value)
                return alloc.allocate(n, hint);
            else
                return alloc.allocate(n);
        }

        static void deallocate(Alloc& alloc, pointer ptr, size_type n)
        {
            alloc.deallocate(ptr, n);
        }

        template<class T, class... Args>
        static void construct(Alloc& alloc, T* ptr, Args&&... args)
        {
            if constexpr (aux::alloc_has_construct<Alloc, T, Args...>::value)
                alloc.construct(ptr, forward<Args>(args)...);
            else
                ::new(static_cast<void*>(ptr)) T(forward<Args>(args)...);
        }

        template<class T>
        static void destroy(Alloc& alloc, T* ptr)
        {
            if constexpr (aux::alloc_has_destroy<Alloc, T>::value)
                alloc.destroy(ptr);
            else
                ptr->~T();
        }

        static size_type max_size(const Alloc& alloc) noexcept
        {
            if constexpr (aux::alloc_has_max_size<Alloc>::value)
                return alloc.max_size();
            else
                return numeric_limits<size_type>::max();
        }

        static Alloc select_on_container_copy_construction(const Alloc& alloc)
        {
            if constexpr (aux::alloc_has_select<Alloc>::value)
                return alloc.select_on_container_copy_construction();
            else
                return alloc;
        }
    };

    /**
     * 20.7.9, the default allocator
     */

    template<class T>
    class allocator;

    template<>
    class allocator<void>
    {
        public:
            using pointer       = void*;
            using const_pointer = const void*;
            using value_type    = void;

            template<class U>
            struct rebind
            {
                using other = allocator<U>;
            };
    };

    template<class T>
    T* addressof(T& x) noexcept;

    template<class T>
    class allocator
    {
        public:
            using size_type       = size_t;
            using difference_type = ptrdiff_t;
            using pointer         = T*;
            using const_pointer   = const T*;
            using reference       = T&;
            using const_reference = const T&;
            using value_type      = T;

            template<class U>
            struct rebind
            {
                using other = allocator<U>;
            };

            using propagate_on_container_move_assignment = true_type;
            using is_always_equal                        = true_type;

            allocator() noexcept = default;

            allocator(const allocator&) noexcept = default;

            template<class U>
            allocator(const allocator<U>&) noexcept
            { /* DUMMY BODY */ }

            ~allocator() = default;

            pointer address(reference x) const noexcept
            {
                return addressof(x);
            }

            const_pointer address(const_reference x) const noexcept
            {
                return addressof(x);
            }

            pointer allocate(size_type n, allocator<void>::const_pointer = 0)
            {
                return static_cast<pointer>(::operator new(n * sizeof(value_type)));
            }

            void deallocate(pointer ptr, size_type n)
            {
                ::operator delete(ptr, n);
            }

            size_type max_size() const noexcept
            {
                return numeric_limits<size_type>::max();
            }

            template<class U, class... Args>
            void construct(U* ptr, Args&&... args)
            {
                ::new((void*)ptr) U(forward<Args>(args)...);
            }

            template<class U>
            void destroy(U* ptr)
            {
                ptr->~U();
            }
    };

    template<class T1, class T2>
    bool operator==(const allocator<T1>&, const allocator<T2>&) noexcept
    {
        return true;
    }

    template<class T1, class T2>
    bool operator!=(const allocator<T1>&, const allocator<T2>&) noexcept
    {
        return false;
    }
}

#endif
