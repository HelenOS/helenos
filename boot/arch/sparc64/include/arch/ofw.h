/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_sparc64_OFW_H_
#define BOOT_sparc64_OFW_H_

#include <stdint.h>

#define OFW_ADDRESS_CELLS  2
#define OFW_SIZE_CELLS     2

extern uintptr_t ofw_get_physmem_start(void);
extern void ofw_cpu(uint16_t, uintptr_t);

#endif
