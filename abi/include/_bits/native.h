/*
 * Copyright (c) 2017 CZ.NIC, z.s.p.o.
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

/*
 * Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 * A bunch of type aliases HelenOS code uses.
 *
 * They were originally defined as either u/int32_t or u/int64_t,
 * specifically for each architecture, but in practice they are
 * currently assumed to be identical to u/intptr_t, so we do just that.
 */

#ifndef _BITS_NATIVE_H_
#define _BITS_NATIVE_H_

#include <inttypes.h>
#include <_bits/decls.h>

__HELENOS_DECLS_BEGIN;

typedef uintptr_t pfn_t;
typedef uintptr_t ipl_t;
typedef uintptr_t sysarg_t;
typedef intptr_t  native_t;

#ifdef KERNEL

typedef sysarg_t uspace_addr_t;
/* We might implement a way to check validity of the type some day. */
#define uspace_ptr(type) uspace_addr_t
#define USPACE_NULL 0

#else /* !KERNEL */

typedef void *uspace_addr_t;
#define uspace_ptr(type) type *

#endif

// TODO: Put this in a better location.
#define uspace_ptr_as_area_info_t uspace_ptr(as_area_info_t)
#define uspace_ptr_as_area_pager_info_t uspace_ptr(as_area_pager_info_t)
#define uspace_ptr_cap_irq_handle_t uspace_ptr(cap_irq_handle_t)
#define uspace_ptr_cap_phone_handle_t uspace_ptr(cap_phone_handle_t)
#define uspace_ptr_cap_waitq_handle_t uspace_ptr(cap_waitq_handle_t)
#define uspace_ptr_char uspace_ptr(char)
#define uspace_ptr_const_char uspace_ptr(const char)
#define uspace_ptr_ddi_ioarg_t uspace_ptr(ddi_ioarg_t)
#define uspace_ptr_ipc_data_t uspace_ptr(ipc_data_t)
#define uspace_ptr_irq_code_t uspace_ptr(irq_code_t)
#define uspace_ptr_size_t uspace_ptr(size_t)
#define uspace_ptr_struct_uspace_arg uspace_ptr(struct uspace_arg)
#define uspace_ptr_sysarg64_t uspace_ptr(sysarg64_t)
#define uspace_ptr_task_id_t uspace_ptr(task_id_t)
#define uspace_ptr_thread_id_t uspace_ptr(thread_id_t)
#define uspace_ptr_uintptr_t uspace_ptr(uintptr_t)
#define uspace_ptr_uspace_arg_t uspace_ptr(uspace_arg_t)
#define uspace_ptr_uspace_thread_function_t uspace_ptr(uspace_thread_function_t)

__HELENOS_DECLS_END;

#endif

/** @}
 */
