/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libriff
 * @{
 */
/**
 * @file RIFF chunk.
 */

#ifndef RIFF_CHUNK_H
#define RIFF_CHUNK_H

#include <errno.h>
#include <stddef.h>
#include <types/riff/chunk.h>

extern errno_t riff_wopen(const char *, riffw_t **);
extern errno_t riff_wclose(riffw_t *);
extern errno_t riff_wchunk_start(riffw_t *, riff_ckid_t, riff_wchunk_t *);
extern errno_t riff_wchunk_end(riffw_t *, riff_wchunk_t *);
extern errno_t riff_write(riffw_t *, void *, size_t);
extern errno_t riff_write_uint32(riffw_t *, uint32_t);

extern errno_t riff_ropen(const char *, riff_rchunk_t *, riffr_t **);
extern errno_t riff_rclose(riffr_t *);
extern errno_t riff_read_uint32(riff_rchunk_t *, uint32_t *);
extern errno_t riff_rchunk_start(riff_rchunk_t *, riff_rchunk_t *);
extern errno_t riff_rchunk_match(riff_rchunk_t *, riff_ckid_t, riff_rchunk_t *);
extern errno_t riff_rchunk_list_match(riff_rchunk_t *, riff_ltype_t,
    riff_rchunk_t *);
extern errno_t riff_rchunk_seek(riff_rchunk_t *, long, int);
extern errno_t riff_rchunk_end(riff_rchunk_t *);
extern errno_t riff_read(riff_rchunk_t *, void *, size_t, size_t *);
extern uint32_t riff_rchunk_size(riff_rchunk_t *);

#endif

/** @}
 */
