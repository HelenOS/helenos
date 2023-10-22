/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <typeinfo>

namespace __cxxabiv1
{
    /**
     * Stack unwinding functionality - Level 1.
     */

    enum _Unwind_Reason_Code
    {
        _URC_NO_REASON                = 0,
        _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
        _URC_FATAL_PHASE2_ERROR       = 2,
        _URC_FATAL_PHASE1_ERROR       = 3,
        _URC_NORMAL_STOP              = 4,
        _URC_END_OF_STACK             = 5,
        _URC_HANDLER_FOUND            = 6,
        _URC_INSTALL_CONTEXT          = 7,
        _URC_CONTINUE_UNWIND          = 8
    };

    struct _Unwind_Exception;
    using _Unwind_Exception_Cleanup_Fn = void (*)(_Unwind_Reason_Code, _Unwind_Exception*);

    struct _Unwind_Exception
    {
        std::uint64_t exception_class;
        _Unwind_Exception_Cleanup_Fn exception_cleanup;
        std::uint64_t private_1;
        std::uint64_t private_2;
    };

    /* Opaque structure. */
    struct _Unwind_Context;

    using _Unwind_Action = int;
    namespace
    {
        const _Unwind_Action _UA_SEARCH_PHASE  = 1;
        const _Unwind_Action _UA_CLEANUP_PHASE = 2;
        const _Unwind_Action _UA_HANDLER_FRAME = 4;
        const _Unwind_Action _UA_FORCE_HANDLER = 8;
    }

    /**
     * TODO: Explain parameter semantics.
     */
    using _Unwind_Stop_Fn = _Unwind_Reason_Code(*)(
        int, _Unwind_Action, std::uint64_t, _Unwind_Exception*,
        _Unwind_Context*, void*
    );

    extern "C" _Unwind_Reason_Code _Unwind_ForcedUnwind(_Unwind_Exception*, _Unwind_Stop_Fn, void*)
    {
        // TODO: implement
        return _URC_NO_REASON;
    }

    extern "C" void _Unwind_Resume(_Unwind_Exception*)
    {
        // TODO: implement
    }

    extern "C" void _Unwind_DeleteException(_Unwind_Exception*)
    {
        // TODO: implement
    }

    extern "C" std::uint64_t _Unwind_GetGR(_Unwind_Context*, int)
    {
        // TODO: implement
        return 0;
    }

    extern "C" void _Unwind_SetGR(_Unwind_Context*, int, std::uint64_t)
    {
        // TODO: implement
    }

    extern "C" std::uint64_t _Unwind_GetIP(_Unwind_Context*)
    {
        // TODO: implement
        return 0;
    }

    extern "C" void _Unwind_SetIP(_Unwind_Context*, std::uint64_t)
    {
        // TODO: implement
    }

    extern "C" std::uint64_t _Unwind_GetLanguageSpecificData(_Unwind_Context*)
    {
        // TODO: implement
        return 0;
    }

    extern "C" std::uint64_t _Unwind_GetRegionStart(_Unwind_Context*)
    {
        // TODO: implement
        return 0;
    }

    /**
     * TODO: Explain parameter semantics.
     */
    using __personality_routine = _Unwind_Reason_Code(*)(
        int, _Unwind_Action, std::uint64_t, _Unwind_Exception*,
        _Unwind_Context*, void*
    );

    /**
     * Stack unwinding functionality - Level 2.
     */
    struct __cxa_exception
    {
        std::type_info* exceptionType;
        void (*exceptionDestructor)(void*);
        // TODO: Add handler types to <exception>.
        /* std::unexpected_handler unexpectedHandler; */
        void (*unexpectedHandler)();
        /* std::terminate_handler terminateHandler; */
        void (*terminateHandler)();
        __cxa_exception* nextException;

        int handlerCount;
        int handlerSwitchValue;
        const char* actionRecord;
        const char* languageSpecificData;
        void* catchTemp;
        void* adjujstedPtr;

        _Unwind_Exception unwindHeader;
    };

    struct __cxa_eh_globals
    {
        __cxa_exception* caughtExceptions;
        unsigned int uncaughtExceptions;
    };

    extern "C" __cxa_eh_globals* __cxa_get_globals();

    extern "C" __cxa_eh_globals* __cxa_get_globals_fast();

    extern "C" void* __cxa_allocate_exception(std::size_t thrown_size)
    {
        // TODO: implement
        __unimplemented();
        return nullptr;
    }

    extern "C" void __cxa_free_exception(void* thrown_exception)
    {
        // TODO: implement
        __unimplemented();
    }

    extern "C" void __cxa_throw(void* thrown_exception, std::type_info* tinfo, void (*dest)(void*))
    {
        // TODO: implement
        __unimplemented();
    }

    extern "C" void* __cxa_get_exception_ptr(void*  exception_object)
    {
        // TODO: implement
        __unimplemented();
        return nullptr;
    }

    extern "C" void* __cxa_begin_catch(void* exception_object)
    {
        // TODO: implement
        __unimplemented();
        return nullptr;
    }

    extern "C" void __cxa_end_catch()
    {
        // TODO: implement
        __unimplemented();
    }

    extern "C" void __cxa_rethrow()
    {
        // TODO: implement
        __unimplemented();
    }

    extern "C" void __cxa_bad_cast()
    {
        // TODO: implement
        __unimplemented();
    }

    extern "C" void __cxa_bad_typeid()
    {
        // TODO: implement
        __unimplemented();
    }

    extern "C" void __cxa_throw_bad_array_new_length()
    {
        // TODO: implement
        __unimplemented();
    }

    extern "C" _Unwind_Reason_Code __gxx_personality_v0(
        int, _Unwind_Action, unsigned,
        struct _Unwind_Exception*, struct _Unwind_Context*
    )
    {
        // TODO: implement
        __unimplemented();
        return _URC_NO_REASON;
    }
}
