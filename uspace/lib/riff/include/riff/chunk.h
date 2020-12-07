/*
 * Copyright (c) 2015 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
