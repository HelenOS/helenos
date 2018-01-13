/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

/**
 * @file
 * @brief Task permissions definitions.
 *
 * Permissions represent virtual rights that entitle their
 * holder to perform certain security sensitive tasks.
 *
 * Each task can have arbitrary combination of the permissions 
 * defined in this file. Therefore, they are required to be powers
 * of two.
 */

#ifndef __PERM_H__
#define __PERM_H__

#include <typedefs.h>

/**
 * PERM_PERM allows its holder to grant/revoke arbitrary permission to/from
 * other tasks.
 */
#define PERM_PERM        (1 << 0)

/**
 * PERM_MEM_MANAGER allows its holder to map physical memory to other tasks.
 */
#define PERM_MEM_MANAGER (1 << 1)

/**
 * PERM_IO_MANAGER allows its holder to access I/O space to other tasks.
 */
#define PERM_IO_MANAGER  (1 << 2)

/**
 * PERM_IRQ_REG entitles its holder to register IRQ handlers.
 */
#define PERM_IRQ_REG     (1 << 3)

typedef uint32_t perm_t;

#ifdef __32_BITS__

extern sys_errno_t sys_perm_grant(sysarg64_t *, perm_t);
extern sys_errno_t sys_perm_revoke(sysarg64_t *, perm_t);

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

extern sys_errno_t sys_perm_grant(sysarg_t, perm_t);
extern sys_errno_t sys_perm_revoke(sysarg_t, perm_t);

#endif  /* __64_BITS__ */

#endif

/** @}
 */
