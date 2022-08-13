/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Dummy serial line output.
 */

#ifndef KERN_DSRLNOUT_H_
#define KERN_DSRLNOUT_H_

#include <typedefs.h>
#include <stdint.h>
#include <console/chardev.h>

extern outdev_t *dsrlnout_init(ioport8_t *, uintptr_t);

#endif

/** @}
 */
