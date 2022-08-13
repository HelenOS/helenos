/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file Block device client interface
 */

#ifndef LIBDEVICE_BD_H
#define LIBDEVICE_BD_H

#include <async.h>
#include <offset.h>

typedef struct {
	async_sess_t *sess;
} bd_t;

extern errno_t bd_open(async_sess_t *, bd_t **);
extern void bd_close(bd_t *);
extern errno_t bd_read_blocks(bd_t *, aoff64_t, size_t, void *, size_t);
extern errno_t bd_read_toc(bd_t *, uint8_t, void *, size_t);
extern errno_t bd_write_blocks(bd_t *, aoff64_t, size_t, const void *, size_t);
extern errno_t bd_sync_cache(bd_t *, aoff64_t, size_t);
extern errno_t bd_get_block_size(bd_t *, size_t *);
extern errno_t bd_get_num_blocks(bd_t *, aoff64_t *);

#endif

/** @}
 */
