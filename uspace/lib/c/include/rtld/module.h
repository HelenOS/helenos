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

#ifndef _LIBC_RTLD_MODULE_H_
#define _LIBC_RTLD_MODULE_H_

#include <rtld/dynamic.h>
#include <adt/list.h>
#include <types/rtld/module.h>
#include <types/rtld/rtld.h>

extern errno_t module_create_static_exec(rtld_t *, module_t **);
extern void module_process_relocs(module_t *);
extern module_t *module_find(rtld_t *, const char *);
extern module_t *module_load(rtld_t *, const char *, mlflags_t);
extern errno_t module_load_deps(module_t *, mlflags_t);
extern module_t *module_by_id(rtld_t *, unsigned long);

extern void modules_process_relocs(rtld_t *, module_t *);
extern void modules_process_tls(rtld_t *);
extern void modules_untag(rtld_t *);

#endif

/** @}
 */
