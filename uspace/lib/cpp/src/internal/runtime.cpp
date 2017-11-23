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

#include <internal/abi.hpp>

namespace __cxxabiv1
{

    /**
     * No need for a body, this function is called when a virtual
     * call of a pure virtual function cannot be made.
     */
    // TODO: terminate in this
    extern "C" void __cxa_pure_call()
    { /* DUMMY BODY */ }

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
}
