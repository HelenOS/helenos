/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <libc.h>
#include <str.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <abi/klog.h>
#include <io/klog.h>
#include <abi/log.h>

errno_t klog_write(log_level_t lvl, const void *buf, size_t size)
{
	return (errno_t) __SYSCALL4(SYS_KLOG, KLOG_WRITE, (sysarg_t) buf,
	    size, lvl);
}

errno_t klog_read(void *data, size_t size, size_t *nread)
{
	return (errno_t) __SYSCALL5(SYS_KLOG, KLOG_READ, (uintptr_t) data,
	    size, 0, (sysarg_t) nread);
}

/** @}
 */
