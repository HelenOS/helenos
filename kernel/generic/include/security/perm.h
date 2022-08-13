/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
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

extern sys_errno_t sys_perm_grant(uspace_ptr_sysarg64_t, perm_t);
extern sys_errno_t sys_perm_revoke(uspace_ptr_sysarg64_t, perm_t);

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

extern sys_errno_t sys_perm_grant(sysarg_t, perm_t);
extern sys_errno_t sys_perm_revoke(sysarg_t, perm_t);

#endif  /* __64_BITS__ */

#endif

/** @}
 */
