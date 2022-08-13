/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
typedef struct sif_sess sif_sess_t;

struct sif_trans;
typedef struct sif_trans sif_trans_t;

struct sif_node;
typedef struct sif_node sif_node_t;

errno_t sif_create(const char *, sif_sess_t **);
errno_t sif_open(const char *, sif_sess_t **);
errno_t sif_close(sif_sess_t *);
sif_node_t *sif_get_root(sif_sess_t *);

sif_node_t *sif_node_first_child(sif_node_t *);
sif_node_t *sif_node_next_child(sif_node_t *);
const char *sif_node_get_type(sif_node_t *);
const char *sif_node_get_attr(sif_node_t *, const char *);

errno_t sif_trans_begin(sif_sess_t *, sif_trans_t **);
void sif_trans_abort(sif_trans_t *);
errno_t sif_trans_end(sif_trans_t *);

errno_t sif_node_prepend_child(sif_trans_t *, sif_node_t *, const char *,
    sif_node_t **);
errno_t sif_node_append_child(sif_trans_t *, sif_node_t *, const char *,
    sif_node_t **);
errno_t sif_node_insert_before(sif_trans_t *, sif_node_t *, const char *,
    sif_node_t **);
errno_t sif_node_insert_after(sif_trans_t *, sif_node_t *, const char *,
    sif_node_t **);
void sif_node_destroy(sif_trans_t *, sif_node_t *);
errno_t sif_node_set_attr(sif_trans_t *, sif_node_t *, const char *,
    const char *);
void sif_node_unset_attr(sif_trans_t *, sif_node_t *, const char *);

#endif

/** @}
 */
