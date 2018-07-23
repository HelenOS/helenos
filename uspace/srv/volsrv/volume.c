/*
 * Copyright (c) 2018 Jiri Svoboda
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
 * @file Volume handling
 * @brief
 *
 * Volumes are the file systems (or similar) contained in partitions.
 * Each vol_volume_t can be considered the configuration entry
 * for a volume. Each partition has an associated vol_volume_t.
 *
 * If there is any non-default configuration to be remembered for a
 * volume, the vol_volume_t structure is kept around even after the partition
 * is disassociated from it. Otherwise it is deleted once no longer
 * referenced.
 */

#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>

#include "volume.h"
#include "types/volume.h"

static void vol_volume_delete(vol_volume_t *);
static void vol_volume_add_locked(vol_volumes_t *, vol_volume_t *);
static errno_t vol_volume_lookup_ref_locked(vol_volumes_t *, const char *,
    vol_volume_t **);

/** Allocate new volume structure.
 *
 * @return Pointer to new volume structure
 */
static vol_volume_t *vol_volume_new(void)
{
	vol_volume_t *volume = calloc(1, sizeof(vol_volume_t));

	if (volume == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating volume "
		    "structure. Out of memory.");
		return NULL;
	}

	volume->label = str_dup("");
	volume->mountp = str_dup("");

	if (volume->label == NULL || volume->mountp == NULL) {
		vol_volume_delete(volume);
		return NULL;
	}

	atomic_set(&volume->refcnt, 1);
	link_initialize(&volume->lvolumes);
	volume->volumes = NULL;

	return volume;
}

/** Delete volume structure.
 *
 * @param volume Volume structure
 */
static void vol_volume_delete(vol_volume_t *volume)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "Freeing volume %p", volume);

	free(volume->label);
	free(volume->mountp);
	free(volume);
}

/** Create list of volumes.
 *
 * @param rvolumes Place to store pointer to list of volumes.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_volumes_create(vol_volumes_t **rvolumes)
{
	vol_volumes_t *volumes;

	volumes = calloc(1, sizeof(vol_volumes_t));
	if (volumes == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&volumes->lock);
	list_initialize(&volumes->volumes);

	*rvolumes = volumes;
	return EOK;
}

/** Destroy list of volumes.
 *
 * @param volumes List of volumes or @c NULL
 */
void vol_volumes_destroy(vol_volumes_t *volumes)
{
	link_t *link;
	vol_volume_t *volume;

	if (volumes == NULL)
		return;

	link = list_first(&volumes->volumes);
	while (link != NULL) {
		volume = list_get_instance(link, vol_volume_t, lvolumes);

		list_remove(&volume->lvolumes);
		vol_volume_delete(volume);

		link = list_first(&volumes->volumes);
	}

	free(volumes);
}

/** Add new volume structure to list of volumes.
 *
 * @param volumes List of volumes
 * @param volume Volume structure
 */
static void vol_volume_add_locked(vol_volumes_t *volumes,
    vol_volume_t *volume)
{
	assert(fibril_mutex_is_locked(&volumes->lock));
	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_volume_add_locked(%p)", volume);

	volume->volumes = volumes;
	list_append(&volume->lvolumes, &volumes->volumes);
}

/** Look up volume structure with locked volumes lock.
 *
 * If a matching existing volume is found, it is returned. Otherwise
 * a new volume structure is created.
 *
 * @param volumes List of volumes
 * @param label Volume label
 * @param rvolume Place to store pointer to volume structure (existing or new)
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t vol_volume_lookup_ref_locked(vol_volumes_t *volumes,
    const char *label, vol_volume_t **rvolume)
{
	vol_volume_t *volume;

	assert(fibril_mutex_is_locked(&volumes->lock));

	list_foreach(volumes->volumes, lvolumes, vol_volume_t, volume) {
		if (str_cmp(volume->label, label) == 0 &&
		    str_size(label) > 0) {
			/* Add reference */
			atomic_inc(&volume->refcnt);
			*rvolume = volume;
			return EOK;
		}
	}

	/* No existing volume found. Create a new one. */
	volume = vol_volume_new();
	if (volume == NULL)
		return ENOMEM;

	volume->label = str_dup(label);
	vol_volume_add_locked(volumes, volume);

	*rvolume = volume;
	return EOK;
}

/** Look up volume structure.
 *
 * If a matching existing volume is found, it is returned. Otherwise
 * a new volume structure is created.
 *
 * @param volumes List of volumes
 * @param label Volume label
 * @param rvolume Place to store pointer to volume structure (existing or new)
 *
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_volume_lookup_ref(vol_volumes_t *volumes, const char *label,
    vol_volume_t **rvolume)
{
	errno_t rc;

	fibril_mutex_lock(&volumes->lock);
	rc = vol_volume_lookup_ref_locked(volumes, label, rvolume);
	fibril_mutex_unlock(&volumes->lock);

	return rc;
}

/** Determine if volume has non-default settings that need to persist.
 *
 * @param volume Volume
 * @return @c true iff volume has settings that need to persist
 */
static bool vol_volume_is_persist(vol_volume_t *volume)
{
	return str_size(volume->mountp) > 0;
}

/** Delete reference to volume.
 *
 * @param volume Volume
 */
void vol_volume_del_ref(vol_volume_t *volume)
{
	if (atomic_predec(&volume->refcnt) == 0) {
		/* No more references. Check if volume is persistent. */
		if (!vol_volume_is_persist(volume)) {
			list_remove(&volume->lvolumes);
			vol_volume_delete(volume);
		}
	}
}

/** Set volume mount point.
 *
 * @param volume Volume
 * @param mountp Mount point
 *
 * @return EOK on success or error code
 */
errno_t vol_volume_set_mountp(vol_volume_t *volume, const char *mountp)
{
	char *mp;

	mp = str_dup(mountp);
	if (mp == NULL)
		return ENOMEM;

	free(volume->mountp);
	volume->mountp = mp;

	return EOK;
}

/** @}
 */
