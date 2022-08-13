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

#ifndef _LIBC_RTLD_H_
#define _LIBC_RTLD_H_

#include <adt/list.h>
#include <elf/elf_mod.h>

#include <rtld/dynamic.h>
#include <tls.h>
#include <types/rtld/rtld.h>

extern rtld_t *runtime_env;

extern errno_t rtld_init_static(void);
extern errno_t rtld_prog_process(elf_finfo_t *, rtld_t **);
extern tcb_t *rtld_tls_make(rtld_t *);
extern unsigned long rtld_get_next_id(rtld_t *);
extern void *rtld_tls_get_addr(rtld_t *, tcb_t *, unsigned long, unsigned long);

#endif

/** @}
 */
