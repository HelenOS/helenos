/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <io/verify.h>

/* Various error levels */
#define CL_EFATAL  -1
#define CL_EOK     0
#define CL_EFAIL   1
#define CL_EBUSY   2
#define CL_ENOENT  3
#define CL_ENOMEM  4
#define CL_EPERM   5
#define CL_ENOTSUP 6
#define CL_EEXEC   7
#define CL_EEXIST  8
#define CL_ETOOBIG 9

/* Just like 'errno' */
extern volatile int cli_errno;

extern void cli_error(int, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);

#endif
