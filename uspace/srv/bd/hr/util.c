/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#include <adt/list.h>
#include <block.h>
#include <errno.h>
#include <fibril_synch.h>
#include <hr.h>
#include <inttypes.h>
#include <io/log.h>
#include <loc.h>
#include <mem.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>
#include <str_error.h>
#include <vbd.h>

#include "io.h"
#include "superblock.h"
#include "util.h"
#include "var.h"

static bool hr_range_lock_overlap(hr_range_lock_t *, hr_range_lock_t *);
static errno_t hr_add_svc_linked_to_list(list_t *, service_id_t, bool, void *);
static void free_dev_list_member(struct dev_list_member *);
static void free_svc_id_list(list_t *);
static errno_t hr_fill_disk_part_svcs_list(list_t *);
static errno_t block_init_dev_list(list_t *);
static void block_fini_dev_list(list_t *);
static errno_t hr_util_get_matching_md_svcs_list(list_t *, list_t *,
    service_id_t, hr_metadata_type_t, void *);
static errno_t hr_util_assemble_from_matching_list(list_t *,
    hr_metadata_type_t, uint8_t);
static errno_t hr_fill_svcs_list_from_cfg(hr_config_t *, list_t *);
static errno_t hr_swap_hs(hr_volume_t *, size_t, size_t);

#define HR_RL_LIST_LOCK(vol) (fibril_mutex_lock(&(vol)->range_lock_list_lock))
#define HR_RL_LIST_UNLOCK(vol) \
    (fibril_mutex_unlock(&(vol)->range_lock_list_lock))

extern loc_srv_t *hr_srv;
extern list_t hr_volumes;
extern fibril_rwlock_t hr_volumes_lock;

/*
 * malloc() wrapper that behaves like
 * FreeBSD malloc(9) with M_WAITOK flag.
 *
 * Return value is never NULL.
 */
void *hr_malloc_waitok(size_t size)
{
	void *ret;
	while ((ret = malloc(size)) == NULL)
		fibril_usleep(MSEC2USEC(250)); /* sleep 250ms */

	return ret;
}

void *hr_calloc_waitok(size_t nmemb, size_t size)
{
	void *ret;
	while ((ret = calloc(nmemb, size)) == NULL)
		fibril_usleep(MSEC2USEC(250)); /* sleep 250ms */

	return ret;
}

errno_t hr_create_vol_struct(hr_volume_t **rvol, hr_level_t level,
    const char *devname, hr_metadata_type_t metadata_type, uint8_t vflags)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;

	hr_volume_t *vol = calloc(1, sizeof(hr_volume_t));
	if (vol == NULL)
		return ENOMEM;

	str_cpy(vol->devname, HR_DEVNAME_LEN, devname);
	vol->level = level;

	vol->vflags = vflags;

	vol->meta_ops = hr_get_meta_type_ops(metadata_type);

	switch (level) {
	case HR_LVL_0:
		vol->hr_ops.create = hr_raid0_create;
		vol->hr_ops.init = hr_raid0_init;
		vol->hr_ops.vol_state_eval = hr_raid0_vol_state_eval;
		vol->hr_ops.ext_state_cb = hr_raid0_ext_state_cb;
		break;
	case HR_LVL_1:
		vol->hr_ops.create = hr_raid1_create;
		vol->hr_ops.init = hr_raid1_init;
		vol->hr_ops.vol_state_eval = hr_raid1_vol_state_eval;
		vol->hr_ops.ext_state_cb = hr_raid1_ext_state_cb;
		break;
	case HR_LVL_4:
	case HR_LVL_5:
		vol->hr_ops.create = hr_raid5_create;
		vol->hr_ops.init = hr_raid5_init;
		vol->hr_ops.vol_state_eval = hr_raid5_vol_state_eval;
		vol->hr_ops.ext_state_cb = hr_raid5_ext_state_cb;
		break;
	default:
		HR_DEBUG("unkown level: %d, aborting\n", vol->level);
		rc = EINVAL;
		goto error;
	}

	if (level == HR_LVL_4 || level == HR_LVL_5)
		vol->fge = hr_fpool_create(16, 32, sizeof(hr_io_raid5_t));
	else
		vol->fge = hr_fpool_create(16, 32, sizeof(hr_io_t));

	if (vol->fge == NULL) {
		rc = ENOMEM;
		goto error;
	}

	vol->state = HR_VOL_NONE;

	fibril_mutex_initialize(&vol->md_lock);

	fibril_rwlock_initialize(&vol->extents_lock);
	fibril_rwlock_initialize(&vol->states_lock);

	fibril_mutex_initialize(&vol->hotspare_lock);

	list_initialize(&vol->range_lock_list);
	fibril_mutex_initialize(&vol->range_lock_list_lock);

	atomic_init(&vol->state_dirty, false);
	atomic_init(&vol->first_write, false);
	for (size_t i = 0; i < HR_MAX_EXTENTS; i++)
		atomic_init(&vol->last_ext_pos_arr[i], 0);
	atomic_init(&vol->last_ext_used, 0);
	atomic_init(&vol->rebuild_blk, 0);
	atomic_init(&vol->open_cnt, 0);

	*rvol = vol;

	return EOK;
error:
	free(vol);
	return rc;
}

void hr_destroy_vol_struct(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	if (vol == NULL)
		return;

	hr_fpool_destroy(vol->fge);
	hr_fini_devs(vol);
	free(vol->in_mem_md);
	free(vol);
}

errno_t hr_get_volume_svcs(size_t *rcnt, service_id_t **rsvcs)
{
	size_t i;
	service_id_t *vol_svcs;

	if (rcnt == NULL || rsvcs == NULL)
		return EINVAL;

	fibril_rwlock_read_lock(&hr_volumes_lock);

	size_t vol_cnt = list_count(&hr_volumes);
	vol_svcs = malloc(vol_cnt * sizeof(service_id_t));
	if (vol_svcs == NULL) {
		fibril_rwlock_read_unlock(&hr_volumes_lock);
		return ENOMEM;
	}

	i = 0;
	list_foreach(hr_volumes, lvolumes, hr_volume_t, iter)
		vol_svcs[i++] = iter->svc_id;

	fibril_rwlock_read_unlock(&hr_volumes_lock);

	*rcnt = vol_cnt;
	*rsvcs = vol_svcs;

	return EOK;
}

hr_volume_t *hr_get_volume(service_id_t svc_id)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *rvol = NULL;

	fibril_rwlock_read_lock(&hr_volumes_lock);
	list_foreach(hr_volumes, lvolumes, hr_volume_t, iter) {
		if (iter->svc_id == svc_id) {
			rvol = iter;
			break;
		}
	}
	fibril_rwlock_read_unlock(&hr_volumes_lock);

	return rvol;
}

errno_t hr_remove_volume(service_id_t svc_id)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol = hr_get_volume(svc_id);
	if (vol == NULL)
		return ENOENT;

	fibril_rwlock_write_lock(&hr_volumes_lock);

	int open_cnt = atomic_load_explicit(&vol->open_cnt,
	    memory_order_relaxed);

	/*
	 * The atomicity of this if condition (and this whole
	 * operation) is provided by the write lock - no new
	 * bd connection can come, because we need to get the
	 * bd_srvs_t from the volume, which we get from the list.
	 * (see hr_client_conn() in hr.c)
	 */
	if (open_cnt > 0) {
		fibril_rwlock_write_unlock(&hr_volumes_lock);
		return EBUSY;
	}

	list_remove(&vol->lvolumes);

	fibril_rwlock_write_unlock(&hr_volumes_lock);

	/* save metadata, but we don't care about states anymore */
	vol->meta_ops->save(vol, NO_STATE_CALLBACK);

	HR_NOTE("deactivating volume \"%s\"\n", vol->devname);

	hr_destroy_vol_struct(vol);

	errno_t rc = loc_service_unregister(hr_srv, svc_id);
	return rc;
}

errno_t hr_init_extents_from_cfg(hr_volume_t *vol, hr_config_t *cfg)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno, smallest_blkno = ~0ULL;
	size_t i, bsize;
	size_t last_bsize = 0;

	for (i = 0; i < cfg->dev_no; i++) {
		service_id_t svc_id = cfg->devs[i];
		if (svc_id == 0) {
			rc = EINVAL;
			goto error;
		}

		HR_DEBUG("%s(): block_init() on (%" PRIun ")\n", __func__,
		    svc_id);
		rc = block_init(svc_id);
		if (rc != EOK) {
			HR_DEBUG("%s(): initing (%" PRIun ") failed, "
			    "aborting\n", __func__, svc_id);
			goto error;
		}

		rc = block_get_nblocks(svc_id, &blkno);
		if (rc != EOK)
			goto error;

		rc = block_get_bsize(svc_id, &bsize);
		if (rc != EOK)
			goto error;

		if (last_bsize != 0 && bsize != last_bsize) {
			HR_DEBUG("block sizes differ\n");
			rc = EINVAL;
			goto error;
		}

		vol->extents[i].svc_id = svc_id;
		vol->extents[i].state = HR_EXT_ONLINE;

		if (blkno < smallest_blkno)
			smallest_blkno = blkno;
		last_bsize = bsize;
	}

	vol->bsize = last_bsize;
	vol->extent_no = cfg->dev_no;
	vol->truncated_blkno = smallest_blkno;

	for (i = 0; i < HR_MAX_HOTSPARES; i++)
		vol->hotspares[i].state = HR_EXT_MISSING;

	return EOK;

error:
	for (i = 0; i < HR_MAX_EXTENTS; i++) {
		if (vol->extents[i].svc_id != 0)
			block_fini(vol->extents[i].svc_id);
	}

	return rc;
}

void hr_fini_devs(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	size_t i;

	for (i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].svc_id != 0) {
			HR_DEBUG("hr_fini_devs(): block_fini() on "
			    "(%" PRIun ")\n", vol->extents[i].svc_id);
			block_fini(vol->extents[i].svc_id);
		}
	}

	for (i = 0; i < vol->hotspare_no; i++) {
		if (vol->hotspares[i].svc_id != 0) {
			HR_DEBUG("hr_fini_devs(): block_fini() on "
			    "(%" PRIun ")\n",
			    vol->hotspares[i].svc_id);
			block_fini(vol->hotspares[i].svc_id);
		}
	}
}

errno_t hr_register_volume(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	service_id_t new_id;
	category_id_t cat_id;
	const char *devname = vol->devname;

	rc = loc_service_register(hr_srv, devname, fallback_port_id, &new_id);
	if (rc != EOK) {
		HR_ERROR("unable to register device \"%s\": %s\n",
		    devname, str_error(rc));
		return rc;
	}

	rc = loc_category_get_id("raid", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		HR_ERROR("failed resolving category \"raid\": %s\n",
		    str_error(rc));
		goto error;
	}

	rc = loc_service_add_to_cat(hr_srv, new_id, cat_id);
	if (rc != EOK) {
		HR_ERROR("failed adding \"%s\" to category \"raid\": %s\n",
		    devname, str_error(rc));
		goto error;
	}

	vol->svc_id = new_id;
	return EOK;
error:
	rc = loc_service_unregister(hr_srv, new_id);
	return rc;
}

errno_t hr_check_ba_range(hr_volume_t *vol, size_t cnt, uint64_t ba)
{
	if (ba + cnt > vol->data_blkno)
		return ERANGE;
	return EOK;
}

void hr_add_data_offset(hr_volume_t *vol, uint64_t *ba)
{
	*ba = *ba + vol->data_offset;
}

void hr_sub_data_offset(hr_volume_t *vol, uint64_t *ba)
{
	*ba = *ba - vol->data_offset;
}

void hr_update_ext_state(hr_volume_t *vol, size_t ext_idx, hr_ext_state_t s)
{
	if (vol->level != HR_LVL_0)
		assert(fibril_rwlock_is_locked(&vol->extents_lock));

	assert(fibril_rwlock_is_write_locked(&vol->states_lock));

	assert(ext_idx < vol->extent_no);

	hr_ext_state_t old = vol->extents[ext_idx].state;
	HR_DEBUG("\"%s\": changing extent %zu state: %s -> %s\n",
	    vol->devname, ext_idx, hr_get_ext_state_str(old),
	    hr_get_ext_state_str(s));
	vol->extents[ext_idx].state = s;
}

void hr_update_hotspare_state(hr_volume_t *vol, size_t hs_idx,
    hr_ext_state_t s)
{
	assert(fibril_mutex_is_locked(&vol->hotspare_lock));

	assert(hs_idx < vol->hotspare_no);

	hr_ext_state_t old = vol->hotspares[hs_idx].state;
	HR_DEBUG("\"%s\": changing hotspare %zu state: %s -> %s\n",
	    vol->devname, hs_idx, hr_get_ext_state_str(old),
	    hr_get_ext_state_str(s));
	vol->hotspares[hs_idx].state = s;
}

void hr_update_vol_state(hr_volume_t *vol, hr_vol_state_t new)
{
	assert(fibril_rwlock_is_write_locked(&vol->states_lock));

	HR_NOTE("\"%s\": volume state changed: %s -> %s\n", vol->devname,
	    hr_get_vol_state_str(vol->state), hr_get_vol_state_str(new));
	vol->state = new;
}

void hr_update_ext_svc_id(hr_volume_t *vol, size_t ext_idx, service_id_t new)
{
	if (vol->level != HR_LVL_0)
		assert(fibril_rwlock_is_write_locked(&vol->extents_lock));

	assert(ext_idx < vol->extent_no);

	service_id_t old = vol->extents[ext_idx].svc_id;
	HR_DEBUG("\"%s\": changing extent no. %zu svc_id: (%" PRIun ") -> "
	    "(%" PRIun ")\n", vol->devname, ext_idx, old, new);
	vol->extents[ext_idx].svc_id = new;
}

void hr_update_hotspare_svc_id(hr_volume_t *vol, size_t hs_idx,
    service_id_t new)
{
	assert(fibril_mutex_is_locked(&vol->hotspare_lock));

	assert(hs_idx < vol->hotspare_no);

	service_id_t old = vol->hotspares[hs_idx].svc_id;
	HR_DEBUG("\"%s\": changing hotspare no. %zu svc_id: (%" PRIun ") -> "
	    "(%" PRIun ")\n", vol->devname, hs_idx, old, new);
	vol->hotspares[hs_idx].svc_id = new;
}

size_t hr_count_extents(hr_volume_t *vol, hr_ext_state_t state)
{
	if (vol->level != HR_LVL_0)
		assert(fibril_rwlock_is_locked(&vol->extents_lock));
	assert(fibril_rwlock_is_locked(&vol->states_lock));

	size_t count = 0;
	for (size_t i = 0; i < vol->extent_no; i++)
		if (vol->extents[i].state == state)
			count++;

	return count;
}

hr_range_lock_t *hr_range_lock_acquire(hr_volume_t *vol, uint64_t ba,
    uint64_t cnt)
{
	hr_range_lock_t *rl = hr_malloc_waitok(sizeof(hr_range_lock_t));

	rl->vol = vol;
	rl->off = ba;
	rl->len = cnt;

	rl->pending = 1;
	rl->ignore = false;

	link_initialize(&rl->link);
	fibril_mutex_initialize(&rl->lock);

	fibril_mutex_lock(&rl->lock);

again:
	HR_RL_LIST_LOCK(vol);
	list_foreach(vol->range_lock_list, link, hr_range_lock_t, rlp) {
		if (rlp->ignore)
			continue;
		if (hr_range_lock_overlap(rlp, rl)) {
			rlp->pending++;

			HR_RL_LIST_UNLOCK(vol);

			fibril_mutex_lock(&rlp->lock);

			HR_RL_LIST_LOCK(vol);

			rlp->pending--;

			/*
			 * when ignore is set, after HR_RL_LIST_UNLOCK(),
			 * noone new is going to be able to start sleeping
			 * on the ignored range lock, only already waiting
			 * IOs will come through here
			 */
			rlp->ignore = true;

			fibril_mutex_unlock(&rlp->lock);

			if (rlp->pending == 0) {
				list_remove(&rlp->link);
				free(rlp);
			}

			HR_RL_LIST_UNLOCK(vol);
			goto again;
		}
	}

	list_append(&rl->link, &vol->range_lock_list);

	HR_RL_LIST_UNLOCK(vol);
	return rl;
}

void hr_range_lock_release(hr_range_lock_t *rl)
{
	if (rl == NULL)
		return;

	HR_RL_LIST_LOCK(rl->vol);

	rl->pending--;

	fibril_mutex_unlock(&rl->lock);

	if (rl->pending == 0) {
		list_remove(&rl->link);
		free(rl);
	}

	HR_RL_LIST_UNLOCK(rl->vol);
}

static bool hr_range_lock_overlap(hr_range_lock_t *rl1, hr_range_lock_t *rl2)
{
	uint64_t rl1_start = rl1->off;
	uint64_t rl1_end = rl1->off + rl1->len - 1;
	uint64_t rl2_start = rl2->off;
	uint64_t rl2_end = rl2->off + rl2->len - 1;

	/* one ends before the other starts */
	if (rl1_end < rl2_start || rl2_end < rl1_start)
		return false;

	return true;
}

void hr_mark_vol_state_dirty(hr_volume_t *vol)
{
	atomic_store(&vol->state_dirty, true);
}

static errno_t hr_add_svc_linked_to_list(list_t *list, service_id_t svc_id,
    bool inited, void *md)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;
	struct dev_list_member *to_add;

	if (list == NULL)
		return EINVAL;

	to_add = malloc(sizeof(struct dev_list_member));
	if (to_add == NULL) {
		rc = ENOMEM;
		goto error;
	}

	to_add->svc_id = svc_id;
	to_add->inited = inited;
	to_add->fini = true;

	if (md != NULL) {
		to_add->md = md;
		to_add->md_present = true;
	} else {
		to_add->md_present = false;
	}

	list_append(&to_add->link, list);

error:
	return rc;
}

static void free_dev_list_member(struct dev_list_member *p)
{
	HR_DEBUG("%s()", __func__);

	if (p->md_present)
		free(p->md);
	free(p);
}

static void free_svc_id_list(list_t *list)
{
	HR_DEBUG("%s()", __func__);

	struct dev_list_member *dev_id;
	while (!list_empty(list)) {
		dev_id = list_pop(list, struct dev_list_member, link);

		free_dev_list_member(dev_id);
	}
}

static errno_t hr_fill_disk_part_svcs_list(list_t *list)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t disk_count;
	service_id_t *disk_svcs = NULL;
	vbd_t *vbd = NULL;

	rc = vbd_create(&vbd);
	if (rc != EOK)
		goto error;

	rc = vbd_get_disks(vbd, &disk_svcs, &disk_count);
	if (rc != EOK)
		goto error;

	for (size_t i = 0; i < disk_count; i++) {
		vbd_disk_info_t disk_info;
		rc = vbd_disk_info(vbd, disk_svcs[i], &disk_info);
		if (rc != EOK)
			goto error;

		if (disk_info.ltype != lt_none) {
			size_t part_count;
			service_id_t *part_ids = NULL;
			rc = vbd_label_get_parts(vbd, disk_svcs[i], &part_ids,
			    &part_count);
			if (rc != EOK)
				goto error;

			for (size_t j = 0; j < part_count; j++) {
				vbd_part_info_t part_info;
				rc = vbd_part_get_info(vbd, part_ids[j],
				    &part_info);
				if (rc != EOK) {
					free(part_ids);
					goto error;
				}

				rc = hr_add_svc_linked_to_list(list,
				    part_info.svc_id, false, NULL);
				if (rc != EOK) {
					free(part_ids);
					goto error;
				}
			}

			free(part_ids);

			/*
			 * vbd can detect some bogus label type, but
			 * no partitions. In that case we handle the
			 * svc_id as a label-less disk.
			 *
			 * This can happen when creating an exfat fs
			 * in FreeBSD for example.
			 */
			if (part_count == 0)
				disk_info.ltype = lt_none;
		}

		if (disk_info.ltype == lt_none) {
			rc = hr_add_svc_linked_to_list(list, disk_svcs[i],
			    false, NULL);
			if (rc != EOK)
				goto error;
		}
	}

	free(disk_svcs);
	vbd_destroy(vbd);
	return EOK;
error:
	free_svc_id_list(list);
	if (disk_svcs != NULL)
		free(disk_svcs);
	vbd_destroy(vbd);

	return rc;
}

static errno_t block_init_dev_list(list_t *list)
{
	HR_DEBUG("%s()", __func__);

	list_foreach_safe(*list, cur_link, next_link) {
		struct dev_list_member *iter;
		iter = list_get_instance(cur_link, struct dev_list_member,
		    link);

		if (iter->inited)
			continue;

		errno_t rc = block_init(iter->svc_id);

		/* already used as an extent of active volume */
		/* XXX: figure out how it is with hotspares too */
		if (rc == EEXIST) {
			list_remove(cur_link);
			free_dev_list_member(iter);
			continue;
		}

		if (rc != EOK)
			return rc;

		iter->inited = true;
		iter->fini = true;
	}

	return EOK;
}

static void block_fini_dev_list(list_t *list)
{
	HR_DEBUG("%s()", __func__);

	list_foreach(*list, link, struct dev_list_member, iter) {
		if (iter->inited && iter->fini) {
			block_fini(iter->svc_id);
			iter->inited = false;
			iter->fini = false;
		}
	}
}

static errno_t hr_util_get_matching_md_svcs_list(list_t *rlist, list_t *list,
    service_id_t svc_id, hr_metadata_type_t type_main,
    void *metadata_struct_main)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	hr_superblock_ops_t *meta_ops = hr_get_meta_type_ops(type_main);

	list_foreach(*list, link, struct dev_list_member, iter) {
		if (iter->svc_id == svc_id)
			continue;

		void *metadata_struct;
		hr_metadata_type_t type;

		rc = hr_find_metadata(iter->svc_id, &metadata_struct, &type);
		if (rc == ENOFS)
			continue;
		if (rc != EOK)
			goto error;

		if (type != type_main) {
			free(metadata_struct);
			continue;
		}

		if (!meta_ops->compare_uuids(metadata_struct_main,
		    metadata_struct)) {
			free(metadata_struct);
			continue;
		}

		rc = hr_add_svc_linked_to_list(rlist, iter->svc_id, true,
		    metadata_struct);
		if (rc != EOK)
			goto error;
	}

	return  EOK;
error:
	free_svc_id_list(rlist);
	return rc;
}

static errno_t hr_util_assemble_from_matching_list(list_t *list,
    hr_metadata_type_t type, uint8_t vflags)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	hr_superblock_ops_t *meta_ops = hr_get_meta_type_ops(type);

	link_t *memb_l = list_first(list);
	struct dev_list_member *memb = list_get_instance(memb_l,
	    struct dev_list_member, link);

	hr_level_t level = meta_ops->get_level(memb->md);
	const char *devname = meta_ops->get_devname(memb->md);

	hr_volume_t *vol;
	rc = hr_create_vol_struct(&vol, level, devname, type, vflags);
	if (rc != EOK)
		return rc;

	meta_ops->init_meta2vol(list, vol);
	if (rc != EOK)
		goto error;

	rc = vol->hr_ops.create(vol);
	if (rc != EOK)
		goto error;

	for (size_t e = 0; e < vol->extent_no; e++) {
		if (vol->extents[e].svc_id == 0)
			continue;
		list_foreach(*list, link, struct dev_list_member, iter) {
			if (iter->svc_id == vol->extents[e].svc_id)
				iter->fini = false;
		}
	}

	rc = hr_register_volume(vol);
	if (rc != EOK)
		goto error;

	fibril_rwlock_write_lock(&hr_volumes_lock);
	list_append(&vol->lvolumes, &hr_volumes);
	fibril_rwlock_write_unlock(&hr_volumes_lock);

	HR_NOTE("assembled volume \"%s\"\n", vol->devname);

	return EOK;
error:
	/* let the caller fini the block svc list */
	for (size_t e = 0; e < vol->extent_no; e++)
		vol->extents[e].svc_id = 0;

	hr_destroy_vol_struct(vol);

	return rc;
}

static errno_t hr_fill_svcs_list_from_cfg(hr_config_t *cfg, list_t *list)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;
	for (size_t i = 0; i < cfg->dev_no; ++i) {
		rc = hr_add_svc_linked_to_list(list, cfg->devs[i], false,
		    NULL);
		if (rc != EOK)
			goto error;
	}

	return EOK;
error:
	free_svc_id_list(list);
	return rc;
}

errno_t hr_util_try_assemble(hr_config_t *cfg, size_t *rassembled_cnt)
{
	HR_DEBUG("%s()", __func__);

	/*
	 * scan partitions or disks:
	 *
	 * When we find a metadata block with valid
	 * magic, take UUID and try to find other matching
	 * UUIDs.
	 *
	 * We ignore extents that are a part of already
	 * active volumes. (even when the counter is lower
	 * on active volumes... XXX: use timestamp as initial counter value
	 * when assembling, or writing dirty metadata?)
	 */

	size_t asm_cnt = 0;
	errno_t rc;
	list_t dev_id_list;
	uint8_t vflags = 0;

	list_initialize(&dev_id_list);

	if (cfg == NULL) {
		rc = hr_fill_disk_part_svcs_list(&dev_id_list);
	} else {
		rc = hr_fill_svcs_list_from_cfg(cfg, &dev_id_list);
		vflags = cfg->vol_flags;
	}

	if (rc != EOK)
		goto error;

	rc = block_init_dev_list(&dev_id_list);
	if (rc != EOK)
		goto error;

	struct dev_list_member *iter;
	while (!list_empty(&dev_id_list)) {
		iter = list_pop(&dev_id_list, struct dev_list_member, link);

		void *metadata_struct_main;
		hr_metadata_type_t type;

		rc = hr_find_metadata(iter->svc_id, &metadata_struct_main, &type);
		if (rc == ENOFS) {
			block_fini(iter->svc_id);
			free_dev_list_member(iter);
			rc = EOK;
			continue;
		}

		if (rc != EOK) {
			block_fini(iter->svc_id);
			free_dev_list_member(iter);
			goto error;
		}

		char *svc_name = NULL;
		rc = loc_service_get_name(iter->svc_id, &svc_name);
		if (rc != EOK) {
			block_fini(iter->svc_id);
			free_dev_list_member(iter);
			goto error;
		}
		HR_DEBUG("found valid metadata on %s (type = %s), matching "
		    "other extents\n",
		    svc_name, hr_get_metadata_type_str(type));
		free(svc_name);

		list_t matching_svcs_list;
		list_initialize(&matching_svcs_list);

		rc = hr_util_get_matching_md_svcs_list(&matching_svcs_list,
		    &dev_id_list, iter->svc_id, type, metadata_struct_main);
		if (rc != EOK) {
			block_fini(iter->svc_id);
			free_dev_list_member(iter);
			goto error;
		}

		/* add current iter to list as well */
		rc = hr_add_svc_linked_to_list(&matching_svcs_list,
		    iter->svc_id, true, metadata_struct_main);
		if (rc != EOK) {
			block_fini(iter->svc_id);
			free_svc_id_list(&matching_svcs_list);
			goto error;
		}

		free_dev_list_member(iter);

		/* remove matching list members from dev_id_list */
		list_foreach(matching_svcs_list, link, struct dev_list_member,
		    iter2) {
			struct dev_list_member *to_remove;
			list_foreach_safe(dev_id_list, cur_link, next_link) {
				to_remove = list_get_instance(cur_link,
				    struct dev_list_member, link);
				if (to_remove->svc_id == iter2->svc_id) {
					list_remove(cur_link);
					free_dev_list_member(to_remove);
				}
			}
		}

		rc = hr_util_assemble_from_matching_list(&matching_svcs_list,
		    type, vflags);
		switch (rc) {
		case EOK:
			asm_cnt++;
			break;
		case ENOMEM:
			goto error;
		default:
			rc = EOK;
		}
		block_fini_dev_list(&matching_svcs_list);
		free_svc_id_list(&matching_svcs_list);
	}

error:
	if (rassembled_cnt != NULL)
		*rassembled_cnt = asm_cnt;

	block_fini_dev_list(&dev_id_list);
	free_svc_id_list(&dev_id_list);

	return rc;
}

errno_t hr_util_add_hotspare(hr_volume_t *vol, service_id_t hotspare)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	fibril_mutex_lock(&vol->hotspare_lock);

	if (vol->hotspare_no >= HR_MAX_HOTSPARES) {
		HR_ERROR("%s(): cannot add more hotspares "
		    "to \"%s\"\n", __func__, vol->devname);
		rc = ELIMIT;
		goto error;
	}

	for (size_t i = 0; i < vol->hotspare_no; i++) {
		if (vol->hotspares[i].svc_id == hotspare) {
			HR_ERROR("%s(): hotspare (%" PRIun ") already used in "
			    "%s\n", __func__, hotspare, vol->devname);
			rc = EEXIST;
			goto error;
		}
	}

	rc = block_init(hotspare);
	if (rc != EOK)
		goto error;

	uint64_t hs_blkno;
	rc = block_get_nblocks(hotspare, &hs_blkno);
	if (rc != EOK) {
		block_fini(hotspare);
		goto error;
	}

	if (hs_blkno < vol->truncated_blkno) {
		HR_ERROR("%s(): hotspare (%" PRIun ") doesn't have enough "
		    "blocks\n", __func__, hotspare);

		rc = EINVAL;
		block_fini(hotspare);
		goto error;
	}

	size_t hs_idx = vol->hotspare_no;

	vol->hotspare_no++;

	hr_update_hotspare_svc_id(vol, hs_idx, hotspare);
	hr_update_hotspare_state(vol, hs_idx, HR_EXT_HOTSPARE);

	hr_mark_vol_state_dirty(vol);
error:
	fibril_mutex_unlock(&vol->hotspare_lock);
	return rc;
}

void hr_raid5_xor(void *dst, const void *src, size_t size)
{
	size_t i;
	uint64_t *d = dst;
	const uint64_t *s = src;

	for (i = 0; i < size / sizeof(uint64_t); ++i)
		*d++ ^= *s++;
}

errno_t hr_sync_extents(hr_volume_t *vol)
{
	errno_t rc = EOK;

	fibril_rwlock_read_lock(&vol->extents_lock);
	for (size_t e = 0; e < vol->extent_no; e++) {
		fibril_rwlock_read_lock(&vol->states_lock);
		hr_ext_state_t s = vol->extents[e].state;
		fibril_rwlock_read_unlock(&vol->states_lock);

		service_id_t svc_id = vol->extents[e].svc_id;

		if (s == HR_EXT_ONLINE || s == HR_EXT_REBUILD) {
			errno_t rc = hr_sync_cache(svc_id, 0, 0);
			if (rc != EOK && rc != ENOTSUP)
				vol->hr_ops.ext_state_cb(vol, e, rc);
		}
	}
	fibril_rwlock_read_unlock(&vol->extents_lock);

	vol->hr_ops.vol_state_eval(vol);

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_vol_state_t s = vol->state;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (s == HR_VOL_FAULTY)
		rc = EIO;

	return rc;
}

errno_t hr_init_rebuild(hr_volume_t *vol, size_t *rebuild_idx)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;
	size_t bad = vol->extent_no;

	if (vol->level == HR_LVL_0)
		return EINVAL;

	fibril_rwlock_read_lock(&vol->states_lock);
	if (vol->state != HR_VOL_DEGRADED) {
		fibril_rwlock_read_unlock(&vol->states_lock);
		return EINVAL;
	}
	fibril_rwlock_read_unlock(&vol->states_lock);

	fibril_rwlock_write_lock(&vol->extents_lock);
	fibril_rwlock_write_lock(&vol->states_lock);
	fibril_mutex_lock(&vol->hotspare_lock);

	size_t rebuild = vol->extent_no;
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_REBUILD) {
			rebuild = i;
			break;
		}
	}

	if (rebuild < vol->extent_no) {
		bad = rebuild;
		goto init_rebuild;
	}

	size_t invalid = vol->extent_no;
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_INVALID) {
			invalid = i;
			break;
		}
	}

	if (invalid < vol->extent_no) {
		bad = invalid;
		goto init_rebuild;
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state != HR_EXT_ONLINE) {
			bad = i;
			break;
		}
	}

	if (bad == vol->extent_no || vol->hotspare_no == 0) {
		rc = EINVAL;
		goto error;
	}

	size_t hotspare_idx = vol->hotspare_no - 1;

	hr_ext_state_t hs_state = vol->hotspares[hotspare_idx].state;
	if (hs_state != HR_EXT_HOTSPARE) {
		HR_ERROR("hr_raid1_rebuild(): invalid hotspare"
		    "state \"%s\", aborting rebuild\n",
		    hr_get_ext_state_str(hs_state));
		rc = EINVAL;
		goto error;
	}

	rc = hr_swap_hs(vol, bad, hotspare_idx);
	if (rc != EOK) {
		HR_ERROR("hr_raid1_rebuild(): swapping "
		    "hotspare failed, aborting rebuild\n");
		goto error;
	}

	hr_extent_t *rebuild_ext = &vol->extents[bad];

	HR_DEBUG("hr_raid1_rebuild(): starting REBUILD on extent no. %zu "
	    "(%"  PRIun  ")\n", bad, rebuild_ext->svc_id);

init_rebuild:
	hr_update_ext_state(vol, bad, HR_EXT_REBUILD);
	hr_update_vol_state(vol, HR_VOL_REBUILD);

	*rebuild_idx = bad;
error:
	fibril_mutex_unlock(&vol->hotspare_lock);
	fibril_rwlock_write_unlock(&vol->states_lock);
	fibril_rwlock_write_unlock(&vol->extents_lock);

	return rc;
}

static errno_t hr_swap_hs(hr_volume_t *vol, size_t bad, size_t hs)
{
	HR_DEBUG("%s()", __func__);

	service_id_t faulty_svc_id = vol->extents[bad].svc_id;
	service_id_t hs_svc_id = vol->hotspares[hs].svc_id;

	hr_update_ext_svc_id(vol, bad, hs_svc_id);
	hr_update_ext_state(vol, bad, HR_EXT_HOTSPARE);

	hr_update_hotspare_svc_id(vol, hs, 0);
	hr_update_hotspare_state(vol, hs, HR_EXT_MISSING);

	vol->hotspare_no--;

	if (faulty_svc_id != 0)
		block_fini(faulty_svc_id);

	return EOK;
}

uint32_t hr_closest_pow2(uint32_t n)
{
	if (n == 0)
		return 0;

	n |= (n >> 1);
	n |= (n >> 2);
	n |= (n >> 4);
	n |= (n >> 8);
	n |= (n >> 16);
	return n - (n >> 1);
}

/** @}
 */
