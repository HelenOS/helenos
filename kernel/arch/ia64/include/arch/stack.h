/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_STACK_H_
#define KERN_ia64_STACK_H_

#include <config.h>

#define MEM_STACK_SIZE	(STACK_SIZE / 2)

#define STACK_ITEM_SIZE			8
#define STACK_ALIGNMENT			16
#define STACK_SCRATCH_AREA_SIZE		16
#define REGISTER_STACK_ALIGNMENT 	8

#endif

/** @}
 */
