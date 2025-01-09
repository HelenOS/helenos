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
#include <sif.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>

#include "volume.h"
#include "types/volume.h"

static void vol_volume_delete(vol_volume_t *);
static void vol_volume_add_locked(vol_volumes_t *, vol_volume_t *);
static errno_t vol_volume_lookup_ref_locked(vol_volumes_t *, const char *,
    vol_volume_t **);
static errno_t vol_volumes_load(sif_node_t *, vol_volumes_t *);
static errno_t vol_volumes_save(vol_volumes_t *, sif_node_t *);

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

	refcount_init(&volume->refcnt);
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "Freeing volume %p", volume);

	free(volume->label);
	free(volume->mountp);
	free(volume);
}

/** Create list of volumes.
 *
 * @param cfg_path Path to file containing configuration repository in SIF
 * @param rvolumes Place to store pointer to list of volumes.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_volumes_create(const char *cfg_path,
    vol_volumes_t **rvolumes)
{
	vol_volumes_t *volumes;
	sif_doc_t *doc = NULL;
	sif_node_t *node;
	sif_node_t *nvolumes;
	const char *ntype;
	errno_t rc;

	volumes = calloc(1, sizeof(vol_volumes_t));
	if (volumes == NULL)
		return ENOMEM;

	volumes->cfg_path = str_dup(cfg_path);
	if (volumes->cfg_path == NULL) {
		rc = ENOMEM;
		goto error;
	}

	fibril_mutex_initialize(&volumes->lock);
	list_initialize(&volumes->volumes);
	volumes->next_id = 1;

	/* Try opening existing repository */
	rc = sif_load(cfg_path, &doc);
	if (rc != EOK) {
		/* Failed to open existing, create new repository */
		rc = sif_new(&doc);
		if (rc != EOK)
			goto error;

		/* Create 'volumes' node. */
		rc = sif_node_append_child(sif_get_root(doc), "volumes",
		    &nvolumes);
		if (rc != EOK)
			goto error;

		rc = sif_save(doc, cfg_path);
		if (rc != EOK)
			goto error;

		sif_delete(doc);
	} else {
		/*
		 * Loaded existing configuration. Find 'volumes' node, should
		 * be the first child of the root node.
		 */
		node = sif_node_first_child(sif_get_root(doc));

		/* Verify it's the correct node type */
		ntype = sif_node_get_type(node);
		if (str_cmp(ntype, "volumes") != 0) {
			rc = EIO;
			goto error;
		}

		rc = vol_volumes_load(node, volumes);
		if (rc != EOK)
			goto error;

		sif_delete(doc);
	}

	*rvolumes = volumes;
	return EOK;
error:
	if (doc != NULL)
		(void) sif_delete(doc);
	if (volumes != NULL && volumes->cfg_path != NULL)
		free(volumes->cfg_path);
	if (volumes != NULL)
		free(volumes);

	return rc;
}

/** Merge list of volumes into new file.
 *
 * @param volumes List of volumes
 * @param cfg_path Path to file containing configuration repository in SIF
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_volumes_merge_to(vol_volumes_t *volumes, const char *cfg_path)
{
	sif_doc_t *doc = NULL;
	sif_node_t *node;
	const char *ntype;
	char *dcfg_path;
	errno_t rc;

	dcfg_path = str_dup(cfg_path);
	if (dcfg_path == NULL) {
		rc = ENOMEM;
		goto error;
	}

	free(volumes->cfg_path);
	volumes->cfg_path = dcfg_path;

	/* Try opening existing repository */
	rc = sif_load(cfg_path, &doc);
	if (rc != EOK) {
		/* Failed to open existing, create new repository */
		rc = vol_volumes_sync(volumes);
		if (rc != EOK)
			goto error;
	} else {
		/*
		 * Loaded existing configuration. Find 'volumes' node, should
		 * be the first child of the root node.
		 */
		node = sif_node_first_child(sif_get_root(doc));

		/* Verify it's the correct node type */
		ntype = sif_node_get_type(node);
		if (str_cmp(ntype, "volumes") != 0) {
			rc = EIO;
			goto error;
		}

		rc = vol_volumes_load(node, volumes);
		if (rc != EOK)
			goto error;

		sif_delete(doc);
	}

	return EOK;
error:
	if (doc != NULL)
		(void) sif_delete(doc);
	return rc;
}

/** Sync volume configuration to config file.
 *
 * @param volumes List of volumes
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_volumes_sync(vol_volumes_t *volumes)
{
	sif_doc_t *doc = NULL;
	errno_t rc;

	rc = sif_new(&doc);
	if (rc != EOK)
		goto error;

	rc = vol_volumes_save(volumes, sif_get_root(doc));
	if (rc != EOK)
		goto error;

	rc = sif_save(doc, volumes->cfg_path);
	if (rc != EOK)
		goto error;

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		(void) sif_delete(doc);
	return rc;
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

	free(volumes->cfg_path);
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_volume_add_locked(%p)", volume);

	volume->volumes = volumes;
	list_append(&volume->lvolumes, &volumes->volumes);
	volume->id.id = volumes->next_id;
	++volumes->next_id;
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
			refcount_up(&volume->refcnt);
			*rvolume = volume;
			return EOK;
		}
	}

	/* No existing volume found. Create a new one. */
	volume = vol_volume_new();
	if (volume == NULL)
		return ENOMEM;

	free(volume->label);
	volume->label = str_dup(label);

	if (volume->label == NULL) {
		vol_volume_delete(volume);
		return ENOMEM;
	}

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

/** Find volume structure by ID with locked volumes lock.
 * *
 * @param volumes List of volumes
 * @param vid Volume ID
 * @param rvolume Place to store pointer to volume structure (existing or new)
 *
 * @return EOK on success, ENOENT if not found
 */
static errno_t vol_volume_find_by_id_ref_locked(vol_volumes_t *volumes,
    volume_id_t vid, vol_volume_t **rvolume)
{
	assert(fibril_mutex_is_locked(&volumes->lock));

	list_foreach(volumes->volumes, lvolumes, vol_volume_t, volume) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2,
		    "vol_volume_find_by_id_ref_locked(%zu==%zu)?",
		    volume->id.id, vid.id);
		if (volume->id.id == vid.id) {
			log_msg(LOG_DEFAULT, LVL_DEBUG2,
			    "vol_volume_find_by_id_ref_locked: found");
			/* Add reference */
			refcount_up(&volume->refcnt);
			*rvolume = volume;
			return EOK;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG2,
	    "vol_volume_find_by_id_ref_locked: not found");
	return ENOENT;
}

/** Find volume by ID.
 *
 * @param volumes Volumes
 * @param vid Volume ID
 * @param rvolume Place to store pointer to volume, with reference count
 *                increased.
 * @return EOK on success or an error code
 */
errno_t vol_volume_find_by_id_ref(vol_volumes_t *volumes, volume_id_t vid,
    vol_volume_t **rvolume)
{
	errno_t rc;

	fibril_mutex_lock(&volumes->lock);
	rc = vol_volume_find_by_id_ref_locked(volumes, vid, rvolume);
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
	if (refcount_down(&volume->refcnt)) {
		/* No more references. Check if volume is persistent. */
		list_remove(&volume->lvolumes);
		vol_volume_delete(volume);
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
	char *old_mp;
	bool was_persist;

	mp = str_dup(mountp);
	if (mp == NULL)
		return ENOMEM;

	was_persist = vol_volume_is_persist(volume);

	old_mp = volume->mountp;
	volume->mountp = mp;

	if (vol_volume_is_persist(volume)) {
		if (!was_persist) {
			/*
			 * Volume is now persistent. Prevent it from being
			 * freed.
			 */
			refcount_up(&volume->refcnt);
		}
	} else {
		if (was_persist) {
			/*
			 * Volume is now non-persistent
			 * Allow volume to be freed.
			 */
			vol_volume_del_ref(volume);
		}
	}

	vol_volumes_sync(volume->volumes);
	free(old_mp);
	return EOK;
}

/** Get list of volume IDs.
 *
 * Get the list of IDs of all persistent volumes (volume configuration
 * entries).
 *
 * @param volumes Volumes
 * @param id_buf Buffer to hold the IDs
 * @param buf_size Buffer size in bytes
 * @param act_size Place to store actual number bytes needed
 * @return EOK on success or an error code
 */
errno_t vol_get_ids(vol_volumes_t *volumes, volume_id_t *id_buf,
    size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	fibril_mutex_lock(&volumes->lock);

	buf_cnt = buf_size / sizeof(volume_id_t);

	act_cnt = 0;
	list_foreach(volumes->volumes, lvolumes, vol_volume_t, volume) {
		if (vol_volume_is_persist(volume))
			++act_cnt;
	}
	*act_size = act_cnt * sizeof(volume_id_t);

	if (buf_size % sizeof(volume_id_t) != 0) {
		fibril_mutex_unlock(&volumes->lock);
		return EINVAL;
	}

	size_t pos = 0;
	list_foreach(volumes->volumes, lvolumes, vol_volume_t, volume) {
		if (vol_volume_is_persist(volume)) {
			if (pos < buf_cnt)
				id_buf[pos].id = volume->id.id;
			pos++;
		}
	}

	fibril_mutex_unlock(&volumes->lock);
	return EOK;
}

/** Load volumes from SIF document.
 *
 * @param nvolumes Volumes node
 * @param volumes Volumes object
 *
 * @return EOK on success or error code
 */
static errno_t vol_volumes_load(sif_node_t *nvolumes, vol_volumes_t *volumes)
{
	sif_node_t *nvolume;
	vol_volume_t *volume = NULL;
	const char *label;
	const char *mountp;
	errno_t rc;

	nvolume = sif_node_first_child(nvolumes);
	while (nvolume != NULL) {
		if (str_cmp(sif_node_get_type(nvolume), "volume") != 0) {
			rc = EIO;
			goto error;
		}

		volume = vol_volume_new();
		if (volume == NULL) {
			rc = ENOMEM;
			goto error;
		}

		label = sif_node_get_attr(nvolume, "label");
		mountp = sif_node_get_attr(nvolume, "mountp");

		if (label == NULL || mountp == NULL) {
			rc = EIO;
			goto error;
		}

		free(volume->label);
		free(volume->mountp);

		volume->label = str_dup(label);
		volume->mountp = str_dup(mountp);

		fibril_mutex_lock(&volumes->lock);
		vol_volume_add_locked(volumes, volume);
		fibril_mutex_unlock(&volumes->lock);
		nvolume = sif_node_next_child(nvolume);
	}

	return EOK;
error:
	if (volume != NULL)
		vol_volume_delete(volume);
	return rc;
}

/** Save volumes to SIF document.
 *
 * @param volumes List of volumes
 * @param rnode Configuration root node
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_volumes_save(vol_volumes_t *volumes, sif_node_t *rnode)
{
	sif_node_t *nvolumes;
	sif_node_t *node;
	link_t *link;
	vol_volume_t *volume;
	errno_t rc;

	/* Create 'volumes' node. */
	rc = sif_node_append_child(rnode, "volumes", &nvolumes);
	if (rc != EOK)
		goto error;

	link = list_first(&volumes->volumes);
	while (link != NULL) {
		volume = list_get_instance(link, vol_volume_t, lvolumes);

		if (vol_volume_is_persist(volume)) {
			/* Create 'volume' node. */
			rc = sif_node_append_child(nvolumes, "volume", &node);
			if (rc != EOK)
				goto error;

			rc = sif_node_set_attr(node, "label", volume->label);
			if (rc != EOK)
				goto error;

			rc = sif_node_set_attr(node, "mountp", volume->mountp);
			if (rc != EOK)
				goto error;
		}

		link = list_next(&volume->lvolumes, &volumes->volumes);
	}

	return EOK;
error:
	return rc;
}

/** Get volume information.
 *
 * @param volume Volume
 * @param vinfo Volume information structure to safe info to
 * @return EOK on success or an error code
 */
errno_t vol_volume_get_info(vol_volume_t *volume, vol_info_t *vinfo)
{
	vinfo->id = volume->id;
	str_cpy(vinfo->label, sizeof(vinfo->label), volume->label);
	str_cpy(vinfo->path, sizeof(vinfo->path), volume->mountp);
	return EOK;
}

/** @}
 */
