/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_TYPES_RTLD_RTLD_H_
#define _LIBC_TYPES_RTLD_RTLD_H_

#include <adt/list.h>
#include <elf/elf_mod.h>
#include <stddef.h>
#include <stdint.h>

#include <types/rtld/module.h>

typedef struct rtld {
	elf_dyn_t *rtld_dynamic;
	module_t rtld;

	module_t *program;

	/** Next module ID */
	unsigned long next_id;

	/** Size of initial TLS tdata + tbss */
	size_t tls_size;
	size_t tls_align;

	/** List of all loaded modules including rtld and the program */
	list_t modules;

	/** List of initial modules */
	list_t imodules;
} rtld_t;

#endif

/** @}
 */
