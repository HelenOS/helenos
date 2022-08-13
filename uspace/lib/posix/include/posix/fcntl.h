/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file File control.
 */

#ifndef POSIX_FCNTL_H_
#define POSIX_FCNTL_H_

#include <sys/types.h>

#define O_CREAT   1
#define O_EXCL    2
#define O_TRUNC   4
#define O_APPEND  8
#define O_RDONLY  16
#define O_RDWR    32
#define O_WRONLY  64

/* Mask for file access modes. */
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

/* Dummy compatibility flag */
#define O_NOCTTY 0

/* fcntl commands */
#define F_DUPFD            0 /* Duplicate file descriptor. */
#define F_DUPFD_CLOEXEC    1 /* Same as F_DUPFD but with FD_CLOEXEC flag set. */
#define F_GETFD            2 /* Get file descriptor flags. */
#define F_SETFD            3 /* Set file descriptor flags. */
#define F_GETFL            4 /* Get file status and access flags. */
#define F_SETFL            5 /* Set file status flags. */
#define F_GETOWN           6 /* Get socket owner. */
#define F_SETOWN           7 /* Set socket owner. */
#define F_GETLK            8 /* Get locking information. */
#define F_SETLK            9 /* Set locking information. */
#define F_SETLKW          10 /* Set locking information; wait if blocked. */

/* File descriptor flags used with F_GETFD and F_SETFD. */
#define FD_CLOEXEC         1 /* Close on exec. */

__C_DECLS_BEGIN;

extern int open(const char *pathname, int flags, ...);
extern int fcntl(int fd, int cmd, ...);

__C_DECLS_END;

#endif /* POSIX_FCNTL_H_ */

/** @}
 */
