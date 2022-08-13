/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file
 * @brief	MMU trap handlers that do not fit into the trap table.
 */

.register %g2, #scratch
.register %g3, #scratch

.text

#include <arch/trap/mmu.h>
#include <arch/trap/trap_table.h>
#include <arch/regdef.h>

