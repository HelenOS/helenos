/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file
 * Floating point related constants.
 */

#ifndef POSIX_FENV_H_
#define POSIX_FENV_H_

#define FE_DIVBYZERO 1
#define FE_INEXACT 2
#define FE_INVALID 4
#define FE_OVERFLOW 8
#define FE_UNDERFLOW 16
#define FE_ALL_EXCEPT (FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW)

#endif

/** @}
 */
