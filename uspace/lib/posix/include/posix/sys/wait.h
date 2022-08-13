/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Support for waiting.
 */

#ifndef POSIX_SYS_WAIT_H_
#define POSIX_SYS_WAIT_H_

#include <sys/types.h>
#include <_bits/decls.h>

#define WIFEXITED(status) __posix_wifexited(status)
#define WEXITSTATUS(status) __posix_wexitstatus(status)
#define WIFSIGNALED(status) __posix_wifsignaled(status)
#define WTERMSIG(status) __posix_wtermsig(status)

__C_DECLS_BEGIN;

extern int __posix_wifexited(int status);
extern int __posix_wexitstatus(int status);
extern int __posix_wifsignaled(int status);
extern int __posix_wtermsig(int status);

extern pid_t wait(int *stat_ptr);
extern pid_t waitpid(pid_t pid, int *stat_ptr, int options);

__C_DECLS_END;

#endif /* POSIX_SYS_WAIT_H_ */

/** @}
 */
