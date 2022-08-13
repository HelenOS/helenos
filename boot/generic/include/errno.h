/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_ERRNO_H_
#define BOOT_ERRNO_H_

#define EOK        0    /* No error. */
#define ENOENT     -1   /* No such entry. */
#define ENOMEM     -2   /* Not enough memory. */
#define ELIMIT     -3   /* Limit exceeded. */
#define EINVAL     -14  /* Invalid value. */
#define EOVERFLOW  -16  /* The result does not fit its size. */

typedef int errno_t;

#endif

/** @}
 */
