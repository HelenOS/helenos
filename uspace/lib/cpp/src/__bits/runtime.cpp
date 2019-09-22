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

#include <__bits/abi.hpp>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <mutex>

void* __dso_handle = nullptr;

namespace __cxxabiv1
{
    namespace aux
    {
        struct destructor_t
        {
            void (*func)(void*);
            void* ptr;
            void* dso;
        };

        destructor_t* destructors{nullptr};
        std::size_t destructor_count{0};
        std::size_t destructor_size{32};

        /**
         * C atexit does not pass any arguments,
         * but __cxa_finalize requires one so we
         * use a wrapper.
         */
        void atexit_destructors()
        {
            __cxa_finalize(nullptr);
        }
    }

    /**
     * No need for a body, this function is called when a virtual
     * call of a pure virtual function cannot be made.
     */
    extern "C" void __cxa_pure_virtual()
    {
        std::terminate();
    }

#ifdef __arm__
    extern "C" int __aeabi_atexit(void* p, void (*f)(void*), void* d)
    {
        return __cxa_atexit(f, p, d);
    }
#endif

    extern "C" int __cxa_atexit(void (*f)(void*), void* p, void* d)
    {
        if (!aux::destructors)
        {
            aux::destructors = new aux::destructor_t[aux::destructor_size];
            std::atexit(aux::atexit_destructors);
        }
        else if (aux::destructor_count >= aux::destructor_size)
        {
            auto tmp = std::realloc(aux::destructors, aux::destructor_size * 2);

            if (!tmp)
                return -1;

            aux::destructors = static_cast<aux::destructor_t*>(tmp);
            aux::destructor_size *= 2;
        }

        auto& destr = aux::destructors[aux::destructor_count++];
        destr.func = f;
        destr.ptr = p;
        destr.dso = d;

        return 0;
    }

    extern "C" void __cxa_finalize(void *f)
    {
        if (!f)
        {
            for (std::size_t i = aux::destructor_count; i > 0; --i)
            {
                if (aux::destructors[i - 1].func)
                    (*aux::destructors[i - 1].func)(aux::destructors[i - 1].ptr);
            }
        }
        else
        {
            for (std::size_t i = aux::destructor_count; i > 0; --i)
            {
                if (aux::destructors[i - 1].func == f)
                {
                    (*aux::destructors[i - 1].func)(aux::destructors[i - 1].ptr);
                    aux::destructors[i - 1].func = nullptr;
                    aux::destructors[i - 1].ptr = nullptr;
                    aux::destructors[i - 1].dso = nullptr;
                    // TODO: shift and decrement count
                }
            }
        }
    }

    using guard_t = std::uint64_t;
    std::mutex static_guard_mtx{};

    extern "C" int __cxa_guard_acquire(guard_t* guard)
    {
        static_guard_mtx.lock();

        return !*((std::uint8_t*)guard);
    }

    extern "C" void __cxa_guard_release(guard_t* guard)
    {
        *((std::uint8_t*)guard) = 1;

        static_guard_mtx.unlock();
    }

    extern "C" void __cxa_guard_abort(guard_t* guard)
    {
        static_guard_mtx.unlock();
    }

    __fundamental_type_info::~__fundamental_type_info()
    { /* DUMMY BODY */ }

    __array_type_info::~__array_type_info()
    { /* DUMMY BODY */ }

    __function_type_info::~__function_type_info()
    { /* DUMMY BODY */ }

    __enum_type_info::~__enum_type_info()
    { /* DUMMY BODY */ }

    __class_type_info::~__class_type_info()
    { /* DUMMY BODY */ }

    __si_class_type_info::~__si_class_type_info()
    { /* DUMMY BODY */ }

    __vmi_class_type_info::~__vmi_class_type_info()
    { /* DUMMY BODY */ }

    __pbase_type_info::~__pbase_type_info()
    { /* DUMMY BODY */ }

    __pointer_type_info::~__pointer_type_info()
    { /* DUMMY BODY */ }

    __pointer_to_member_type_info::~__pointer_to_member_type_info()
    { /* DUMMY BODY */ }

    /**
     * This structure represents part of the vtable segment
     * that contains data related to dynamic_cast.
     */
    struct vtable
    {
        // Unimportant to us.

        std::ptrdiff_t offset_to_top;
        std::type_info* tinfo;

        // Actual vtable.
    };
    extern "C" void* __dynamic_cast(const void* sub, const __class_type_info* src,
                                    const __class_type_info* dst, std::ptrdiff_t offset)
    {
        // TODO: implement
        // NOTE: as far as I understand it, we get vtable prefix from sub, get the type_info
        //       ptr from that and then climb the inheritance hierarchy upwards till we either
        //       fint dst or fail and return nullptr
        return nullptr;
    }

    // Needed on arm.
    extern "C" void __cxa_end_cleanup()
    { /* DUMMY BODY */ }

    extern "C" int __cxa_thread_atexit(void(*)(void*), void*, void*)
    {
        // TODO: needed for thread_local variables
        __unimplemented();
        return 0;
    }
}
