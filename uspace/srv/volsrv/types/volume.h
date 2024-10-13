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
} vol_volume_t;

/** Volumes */
typedef struct vol_volumes {
	/** Synchronize access to list of volumes */
	fibril_mutex_t lock;
	/** Volumes (list of vol_volume_t) */
	list_t volumes;
	/** Next ID */
	sysarg_t next_id;
	/** Configuration file path */
	char *cfg_path;
} vol_volumes_t;

#endif

/** @}
 */
