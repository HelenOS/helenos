/*
 * Copyright (c) 2017 Jaroslav Jindrak
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

#ifndef LIBCPP_MEMORY
#define LIBCPP_MEMORY

#include <internal/aux.hpp>
#include <utility>
#include <type_traits>

namespace std
{
    /**
     * 20.7.3, pointer traits:
     */

    template<class Ptr>
    struct pointer_traits
    {
        using pointer = Ptr;
        // TODO: element type, difference type

        // TODO: this is conditional, see standard
        template<class U>
        using rebind = typename Ptr::template rebind<U>;
    };

    /**
     * 20.7.8, allocator traits:
     */

    namespace aux
    {
        /**
         * The standard mandates that we reuse type from allocators
         * *if* they are defined or that we use something different.
         * These structures help us alternate between those by
         * using SFINAE.
         * TODO: Create tests for these!
         */

        template<class T, class = void>
        struct get_pointer: aux::type_is<typename T::value_type*>
        { /* DUMMY BODY */ };

        template<class T>
        struct get_pointer<T, void_t<typename T::pointer>>
            : aux::type_is<typename T::pointer>
        { /* DUMMY BODY */ };

        template<class T, class Ptr, class = void>
        struct get_const_pointer
            : aux::type_is<typename pointer_traits<Ptr>::template rebind<const typename T::value_type>>
        { /* DUMMY BODY */ };

        template<class T, class Ptr>
        struct get_const_pointer<T, Ptr, void_t<typename T::const_pointer>>
            : aux::type_is<typename T::const_pointer>
        { /* DUMMY BODY */ };

        template<class T, class Ptr, class = void>
        struct get_void_pointer
            : aux::type_is<typename pointer_traits<Ptr>::template rebind<void>>
        { /* DUMMY BODY */ };

        template<class T, class Ptr>
        struct get_void_pointer<T, Ptr, void_t<typename T::void_pointer>>
            : aux::type_is<typename T::void_pointer>
        { /* DUMMY BODY */ };

        template<class T, class Ptr, class = void>
        struct get_const_void_pointer
            : aux::type_is<typename pointer_traits<Ptr>::template rebind<const void>>
        { /* DUMMY BODY */ };

        template<class T, class Ptr>
        struct get_const_void_pointer<T, Ptr, void_t<typename T::const_void_pointer>>
            : aux::type_is<typename T::const_void_pointer>
        { /* DUMMY BODY */ };

        template<class T, class Ptr, class = void>
        struct get_difference_type
            : aux::type_is<typename pointer_traits<Ptr>::difference_type>
        { /* DUMMY BODY */ };

        template<class T, class Ptr>
        struct get_difference_type<T, Ptr, void_t<typename T::difference_type>>
            : aux::type_is<typename T::difference_type>
        { /* DUMMY BODY */ };

        template<class T, class Difference, class = void>
        struct get_size_type: aux::type_is<make_unsigned_t<Difference>>
        { /* DUMMY BODY */ };

        template<class T, class Difference>
        struct get_size_type<T, Difference, void_t<typename T::size_type>>
            : aux::type_is<typename T::size_type>
        { /* DUMMY BODY */ };

        template<class T, class = void>
        struct get_copy_propagate: aux::type_is<false_type>
        { /* DUMMY BODY */ };

        template<class T>
        struct get_copy_propagate<T, void_t<typename T::propagate_on_container_copy_assignment>>
            : aux::type_is<typename T::propagate_on_container_copy_assignment>
        { /* DUMMY BODY */ };

        template<class T, class = void>
        struct get_move_propagate: aux::type_is<false_type>
        { /* DUMMY BODY */ };

        template<class T>
        struct get_move_propagate<T, void_t<typename T::propagate_on_container_move_assignment>>
            : aux::type_is<typename T::propagate_on_container_move_assignment>
        { /* DUMMY BODY */ };

        template<class T, class = void>
        struct get_swap_propagate: aux::type_is<false_type>
        { /* DUMMY BODY */ };

        template<class T>
        struct get_swap_propagate<T, void_t<typename T::propagate_on_container_swap>>
            : aux::type_is<typename T::propagate_on_container_swap>
        { /* DUMMY BODY */ };

        template<class T, class = void>
        struct get_always_equal: aux::type_is<typename is_empty<T>::type>
        { /* DUMMY BODY */ };

        template<class T>
        struct get_always_equal<T, void_t<typename T::is_always_equal>>
            : aux::type_is<typename T::is_always_equal>
        { /* DUMMY BODY */ };

        template<class Alloc, class T, class = void>
        struct get_rebind_other
        { /* DUMMY BODY */ };

        template<class Alloc, class T>
        struct get_rebind_other<Alloc, T, void_t<typename Alloc::template rebind<T>::other>>
            : aux::type_is<typename Alloc::template rebind<T>::other>
        { /* DUMMY BODY */ };

        /* TODO: How am I suppose to do this?!
        template<template<class T, class... Args> class Alloc>
        struct get_rebind_args;
        */

        template<class Alloc, class T>
        struct get_rebind_args: aux::type_is<typename get_rebind_other<Alloc, T>::type>
        { /* DUMMY BODY */ };
    }

    template<class Alloc>
    struct allocator_traits
    {
        using allocator_type = Alloc;

        using value_type = typename Alloc::value_type;
        using pointer = typename aux::get_pointer<Alloc>::type;
        using const_pointer = typename aux::get_const_pointer<Alloc, pointer>::type;
        // TODO: fix void pointer typedefs
        /* using void_pointer = typename aux::get_void_pointer<Alloc, pointer>::type; */
        /* using const_void_pointer = typename aux::get_const_void_pointer<Alloc, pointer>::type; */
        using void_pointer = void*;
        using const_void_pointer = const void*;
        using difference_type = typename aux::get_difference_type<Alloc, pointer>::type;
        using size_type = typename aux::get_size_type<Alloc, difference_type>::type;

        using propagate_on_container_copy_assignment = typename aux::get_copy_propagate<Alloc>::type;
        using propagate_on_container_move_assignment = typename aux::get_move_propagate<Alloc>::type;
        using propagate_on_container_swap = typename aux::get_swap_propagate<Alloc>::type;
        using is_always_equal = typename aux::get_always_equal<Alloc>::type;

        template<class T>
        using rebind_alloc = typename aux::get_rebind_args<Alloc, T>;

        template<class T>
        using rebind_traits = allocator_traits<rebind_alloc<T>>;

        static pointer allocate(Alloc& alloc, size_type n)
        {
            return alloc.allocate(n);
        }

        static pointer allocate(Alloc& alloc, size_type n, const_void_pointer hint)
        { // TODO: this when it's well formed, otherwise alloc.allocate(n)
            return alloc.allocate(n, hint);
        }

        static void deallocate(Alloc& alloc, pointer ptr, size_type n)
        {
            alloc.deallocate(ptr, n);
        }

        template<class T, class... Args>
        static void construct(Alloc& alloc, T* ptr, Args&&... args)
        {
            // TODO: implement
        }

        template<class T>
        static void destroy(Alloc& alloc, T* ptr)
        {
            // TODO: implement
        }

        static size_type max_size(const Alloc& alloc) noexcept
        {
            // TODO: implement
            return 0;
        }

        static Alloc select_on_container_copy_construction(const Alloc& alloc)
        {
            // TODO: implement
            return Alloc{};
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
            {
                // TODO: implement
            }

            ~allocator() = default;

            pointer address(reference x) const noexcept
            {
                return addressof(x);
            }

            const_pointer address(const_reference x) const noexcept
            {
                return addressof(x);
            }

            pointer allocate(size_type n, allocator<void>::const_pointer hint = 0)
            {
                /**
                 * Note: The usage of hint is unspecified.
                 *       TODO: Check HelenOS hint allocation capabilities.
                 * TODO: assert that n < max_size()
                 */
                return static_cast<pointer>(::operator new(n * sizeof(value_type)));
            }

            void deallocate(pointer ptr, size_type n)
            {
                ::operator delete(ptr, n);
            }

            size_type max_size() const noexcept
            { // TODO: implement, max argument to allocate
                return 0xFFFFFFFF;
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

    /**
     * 20.7.10, raw storage iterator:
     */

    // TODO: implement

    /**
     * 20.7.11, temporary buffers:
     */

    // TODO: implement

    /**
     * 20.7.12, specialized algorithms:
     */

    template<class T>
    T* addressof(T& x) noexcept
    {
        return reinterpret_cast<T*>(
            &const_cast<char&>(
                reinterpret_cast<const volatile char&>(x)
        ));
    }

    template<class Iterator>
    struct iterator_traits;

    template<class InputIterator, class ForwardIterator>
    ForwardIterator unitialized_copy(
        InputIterator first, InputIterator last,
        ForwardIterator result
    )
    {
        for (; first != last; ++first, ++result)
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(*first);

        return result;
    }

    template<class InputIterator, class Size, class ForwardIterator>
    ForwardIterator unitialized_copy_n(
        InputIterator first, Size n, ForwardIterator result
    )
    {
        for (; n > 0; ++first, --n, ++result)
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(*first);

        return result;
    }

    template<class ForwardIterator, class T>
    void unitialized_fill(
        ForwardIterator first, ForwardIterator last, const T& x
    )
    {
        for (; first != last; ++first)
            ::new (static_cast<void*>(&*first)) typename iterator_traits<ForwardIterator>::value_type(x);
    }

    template<class ForwardIterator, class Size, class T>
    ForwardIterator unitialized_fill_n(
        ForwardIterator first, Size n, const T& x
    )
    {
        for (; n > 0; ++first, --n)
            ::new (static_cast<void*>(&*first)) typename iterator_traits<ForwardIterator>::value_type(x);

        return first;
    }
}

#endif
