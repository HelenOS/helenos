/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_SYMTAB_H_
#define KERN_SYMTAB_H_

#include <symtab_lookup.h>
#include <console/chardev.h>

extern void symtab_print_search(const char *);
extern const char *symtab_hints_enum(const char *, const char **, void **);

#endif

/** @}
 */
