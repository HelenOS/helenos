/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup volsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef TYPES_VOLUME_H_
#define TYPES_VOLUME_H_

#include <adt/list.h>
#include <refcount.h>
#include <fibril_synch.h>
#include <sif.h>

/** Volume */
typedef struct vol_volume {
	/** Containing volume list */
	struct vol_volumes *volumes;
	/** Link to vol_volumes */
	link_t lvolumes;
	/** ID used by clients to refer to the volume */
	volume_id_t id;
	/** Reference count */
	atomic_refcount_t refcnt;
	/** Volume label */
	char *label;
	/** Mount point */
	char *mountp;
	/** SIF node for this volume */
	sif_node_t *nvolume;
} vol_volume_t;

/** Volumes */
typedef struct vol_volumes {
	/** Synchronize access to list of volumes */
	fibril_mutex_t lock;
	/** Volumes (list of vol_volume_t) */
	list_t volumes;
	/** Cconfiguration repo session */
	sif_sess_t *repo;
	/** Volumes SIF node */
	sif_node_t *nvolumes;
	/** Next ID */
	sysarg_t next_id;
} vol_volumes_t;

#endif

/** @}
 */
