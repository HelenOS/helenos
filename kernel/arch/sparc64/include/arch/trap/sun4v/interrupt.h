/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains interrupt vector trap handler.
 */

#ifndef KERN_sparc64_TRAP_SUN4V_INTERRUPT_H_
#define KERN_sparc64_TRAP_SUN4V_INTERRUPT_H_

#ifndef __ASSEMBLER__

#include <arch/istate_struct.h>

extern void sun4v_ipi_init(void);
extern void cpu_mondo(unsigned int, istate_t *);

#endif

#endif

/** @}
 */
