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

/**
 * This file contains glue code that makes different
 * architectures pass.
 */

#ifdef PLATFORM_arm32

/**
 * ARM32 does not have GCC atomic operations inlined by
 * the compiler, so we need to define stubs for our library
 * to compile on this architecture.
 * TODO: make this synchronized
 */
extern "C"
{
#define LIBCPP_GLUE_OP_AND_FETCH(NAME, OP, TYPE, SIZE) \
    TYPE __sync_##NAME##_and_fetch_##SIZE (volatile void* vptr, TYPE val) \
    { \
        TYPE* ptr = (TYPE*)vptr; \
        *ptr = *ptr OP val; \
        return *ptr; \
    }

LIBCPP_GLUE_OP_AND_FETCH(add, +, unsigned, 4)
LIBCPP_GLUE_OP_AND_FETCH(sub, -, unsigned, 4)

#define LIBCPP_GLUE_CMP_AND_SWAP(TYPE, SIZE) \
    TYPE __sync_val_compare_and_swap_##SIZE (TYPE* ptr, TYPE old_val, TYPE new_val) \
    { \
        if (*ptr == old_val) \
            *ptr = new_val; \
        return *ptr; \
    }

LIBCPP_GLUE_CMP_AND_SWAP(unsigned, 4)
}
#endif
