/*
 * SPDX-FileCopyrightText: 2021 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_MEMBER_H_
#define KERN_MEMBER_H_

#include <stdint.h>

#define member_to_inst(ptr_member, type, member_identif) \
	((type *) (((uintptr_t) (ptr_member)) - \
	    offsetof(type, member_identif)))

#endif

/** @}
 */
