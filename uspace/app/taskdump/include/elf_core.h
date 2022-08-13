/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup debug
 * @{
 */
/** @file
 */

#ifndef ELF_CORE_H_
#define ELF_CORE_H_

#include <async.h>
#include <elf/elf_linux.h>
#include <libarch/istate.h>

extern errno_t elf_core_save(const char *, as_area_info_t *, unsigned int,
    async_sess_t *, istate_t *);

#endif

/** @}
 */
