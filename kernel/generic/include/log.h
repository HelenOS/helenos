/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_LOG_H_
#define KERN_LOG_H_

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <printf/verify.h>
#include <abi/log.h>
#include <abi/klog.h>

extern void log_init(void);
extern void log_begin(log_facility_t, log_level_t);
extern void log_end(void);
extern int log_vprintf(const char *, va_list);
extern int log_printf(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern int log(log_facility_t, log_level_t, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(3, 4);

extern sys_errno_t sys_klog(sysarg_t, uspace_addr_t buf, size_t size,
    sysarg_t level, uspace_ptr_size_t uspace_nread);

#endif /* KERN_LOG_H_ */

/** @}
 */
