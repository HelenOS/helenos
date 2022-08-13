/*
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief	sun4v-specific IPI functions
 */

#ifndef KERN_sparc64_sun4v_IPI_H_
#define KERN_sparc64_sun4v_IPI_H_

#include <stdint.h>

extern uint64_t ipi_brodcast_to(void (*)(void), uint16_t cpu_list[], uint64_t);
extern uint64_t ipi_unicast_to(void (*)(void), uint16_t);

#endif

/** @}
 */
