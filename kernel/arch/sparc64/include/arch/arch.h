/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief Various sparc64-specific macros.
 */

#ifndef KERN_sparc64_ARCH_H_
#define KERN_sparc64_ARCH_H_

#ifndef __ASSEMBLER__
#include <arch.h>
#endif

#include <arch/boot/boot.h>

#if defined (SUN4U)
#include <arch/sun4u/arch.h>
#elif defined (SUN4V)
#include <arch/sun4v/arch.h>
#endif

#define ASI_AIUP  0x10  /** Access to primary context with user privileges. */
#define ASI_AIUS  0x11  /** Access to secondary context with user privileges. */

#define NWINDOWS  8  /** Number of register window sets. */

#ifndef __ASSEMBLER__

extern arch_ops_t *sparc64_ops;

extern void sparc64_pre_main(bootinfo_t *);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
