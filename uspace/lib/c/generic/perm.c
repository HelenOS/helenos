/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file  perm.c
 * @brief Functions to grant/revoke permissions to/from a task.
 */

#include <perm.h>
#include <task.h>
#include <libc.h>
#include <types/common.h>

/** Grant permissions to a task.
 *
 * @param id    Destination task ID.
 * @param perms Permissions to grant.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
errno_t perm_grant(task_id_t id, unsigned int perms)
{
#ifdef __32_BITS__
	sysarg64_t arg = (sysarg64_t) id;
	return (errno_t) __SYSCALL2(SYS_PERM_GRANT, (sysarg_t) &arg, (sysarg_t) perms);
#endif

#ifdef __64_BITS__
	return (errno_t) __SYSCALL2(SYS_PERM_GRANT, (sysarg_t) id, (sysarg_t) perms);
#endif
}

/** Revoke permissions from a task.
 *
 * @param id    Destination task ID.
 * @param perms Permissions to revoke.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
errno_t perm_revoke(task_id_t id, unsigned int perms)
{
#ifdef __32_BITS__
	sysarg64_t arg = (sysarg64_t) id;
	return (errno_t) __SYSCALL2(SYS_PERM_REVOKE, (sysarg_t) &arg, (sysarg_t) perms);
#endif

#ifdef __64_BITS__
	return (errno_t) __SYSCALL2(SYS_PERM_REVOKE, (sysarg_t) id, (sysarg_t) perms);
#endif
}

/** @}
 */
