/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 */

#ifndef KERN_SCANC_H_
#define KERN_SCANC_H_

#include <typedefs.h>

#define SCANCODES  128

extern char32_t sc_primary_map[SCANCODES];
extern char32_t sc_secondary_map[SCANCODES];

#endif

/** @}
 */
