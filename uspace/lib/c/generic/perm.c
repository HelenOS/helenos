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
