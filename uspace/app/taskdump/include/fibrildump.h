/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup edit
 * @{
 */
/**
 * @file
 */

#ifndef FIBRILDUMP_H
#define FIBRILDUMP_H

#include <async.h>
#include <symtab.h>

extern errno_t fibrils_dump(symtab_t *, async_sess_t *sess);

#endif

/** @}
 */
