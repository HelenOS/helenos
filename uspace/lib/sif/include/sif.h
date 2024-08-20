/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libsif
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef LIBSIF_SIF_H_
#define LIBSIF_SIF_H_

#include <errno.h>

struct sif_sess;
typedef struct sif_sess sif_doc_t;

struct sif_node;
typedef struct sif_node sif_node_t;

errno_t sif_new(sif_doc_t **);
errno_t sif_load(const char *, sif_doc_t **);
errno_t sif_save(sif_doc_t *, const char *);
void sif_delete(sif_doc_t *);
sif_node_t *sif_get_root(sif_doc_t *);

sif_node_t *sif_node_first_child(sif_node_t *);
sif_node_t *sif_node_next_child(sif_node_t *);
const char *sif_node_get_type(sif_node_t *);
const char *sif_node_get_attr(sif_node_t *, const char *);

errno_t sif_node_prepend_child(sif_node_t *, const char *, sif_node_t **);
errno_t sif_node_append_child(sif_node_t *, const char *, sif_node_t **);
errno_t sif_node_insert_before(sif_node_t *, const char *, sif_node_t **);
errno_t sif_node_insert_after(sif_node_t *, const char *, sif_node_t **);
void sif_node_destroy(sif_node_t *);
errno_t sif_node_set_attr(sif_node_t *, const char *,
    const char *);
void sif_node_unset_attr(sif_node_t *, const char *);

#endif

/** @}
 */
