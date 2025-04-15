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

struct svc_id_linked;

static bool	hr_range_lock_overlap(hr_range_lock_t *, hr_range_lock_t *);
static errno_t	hr_add_svc_linked_to_list(list_t *, service_id_t, bool,
    hr_metadata_t *);
static void	free_svc_id_linked(struct svc_id_linked *);
static void	free_svc_id_list(list_t *);
static errno_t	hr_fill_disk_part_svcs_list(list_t *);
static errno_t	block_init_dev_list(list_t *);
static void	block_fini_dev_list(list_t *);
static errno_t	hr_util_get_matching_md_svcs_list(list_t *, list_t *,
    service_id_t, hr_metadata_t *);
static errno_t	hr_util_assemble_from_matching_list(list_t *);
static errno_t	hr_fill_svcs_list_from_cfg(hr_config_t *, list_t *);

#define HR_RL_LIST_LOCK(vol) (fibril_mutex_lock(&vol->range_lock_list_lock))
#define HR_RL_LIST_UNLOCK(vol) \
    (fibril_mutex_unlock(&vol->range_lock_list_lock))

struct svc_id_linked {
	link_t		 link;
	service_id_t	 svc_id;
	hr_metadata_t	*md;
	bool		 inited;
	bool		 md_present;
};

extern loc_srv_t *hr_srv;
extern list_t hr_volumes;
extern fibril_rwlock_t hr_volumes_lock;

errno_t hr_create_vol_struct(hr_volume_t **rvol, hr_level_t level,
    const char *devname)
{
	errno_t rc;

	hr_volume_t *vol = calloc(1, sizeof(hr_volume_t));
	if (vol == NULL)
		return ENOMEM;

	str_cpy(vol->devname, HR_DEVNAME_LEN, devname);
	vol->level = level;

	switch (level) {
	case HR_LVL_1:
		vol->hr_ops.create = hr_raid1_create;
		vol->hr_ops.init = hr_raid1_init;
		vol->hr_ops.status_event = hr_raid1_status_event;
		vol->hr_ops.add_hotspare = hr_raid1_add_hotspare;
		break;
	case HR_LVL_0:
		vol->hr_ops.create = hr_raid0_create;
		vol->hr_ops.init = hr_raid0_init;
		vol->hr_ops.status_event = hr_raid0_status_event;
		break;
	case HR_LVL_4:
		vol->hr_ops.create = hr_raid5_create;
		vol->hr_ops.init = hr_raid5_init;
		vol->hr_ops.status_event = hr_raid5_status_event;
		vol->hr_ops.add_hotspare = hr_raid5_add_hotspare;
		break;
	case HR_LVL_5:
		vol->hr_ops.create = hr_raid5_create;
		vol->hr_ops.init = hr_raid5_init;
		vol->hr_ops.status_event = hr_raid5_status_event;
		vol->hr_ops.add_hotspare = hr_raid5_add_hotspare;
		break;
	default:
		HR_DEBUG("unkown level: %d, aborting\n", vol->level);
		rc = EINVAL;
		goto error;
	}

	vol->fge = hr_fpool_create(16, 32, sizeof(hr_io_t));
	if (vol->fge == NULL) {
		rc = ENOMEM;
		goto error;
	}

	vol->in_mem_md = calloc(1, sizeof(hr_metadata_t));
	if (vol->in_mem_md == NULL) {
		free(vol->fge);
		rc = ENOMEM;
		goto error;
	}

	vol->status = HR_VOL_NONE;

	fibril_mutex_initialize(&vol->lock); /* XXX: will remove this */

	fibril_mutex_initialize(&vol->md_lock);

	fibril_rwlock_initialize(&vol->extents_lock);
	fibril_rwlock_initialize(&vol->states_lock);

	fibril_mutex_initialize(&vol->hotspare_lock);

	list_initialize(&vol->range_lock_list);
	fibril_mutex_initialize(&vol->range_lock_list_lock);

	atomic_init(&vol->rebuild_blk, 0);
	atomic_init(&vol->state_dirty, false);
	atomic_init(&vol->open_cnt, 0);

	*rvol = vol;

	return EOK;
error:
	free(vol);
	return rc;
}

void hr_destroy_vol_struct(hr_volume_t *vol)
{
	if (vol == NULL)
		return;

	hr_fpool_destroy(vol->fge);
	hr_fini_devs(vol);
	free(vol->in_mem_md);
	free(vol);
}

hr_volume_t *hr_get_volume(service_id_t svc_id)
{
	HR_DEBUG("hr_get_volume(): (%" PRIun ")\n", svc_id);

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
	HR_DEBUG("hr_remove_volume(): (%" PRIun ")\n", svc_id);

	errno_t rc;

	fibril_rwlock_write_lock(&hr_volumes_lock);
	list_foreach(hr_volumes, lvolumes, hr_volume_t, vol) {
		if (vol->svc_id == svc_id) {
			int open_cnt = atomic_load_explicit(&vol->open_cnt,
			    memory_order_relaxed);
			/*
			 * The "atomicity" of this if condition is provided
			 * by the write lock - no new bd connection can
			 * come, because we need to get the bd_srvs_t from
			 * volume, which we get from the list.
			 * (see hr_client_conn() in hr.c)
			 */
			if (open_cnt > 0) {
				fibril_rwlock_write_unlock(&hr_volumes_lock);
				return EBUSY;
			}
			list_remove(&vol->lvolumes);
			fibril_rwlock_write_unlock(&hr_volumes_lock);

			hr_metadata_save(vol, NO_STATE_CALLBACK);

			hr_destroy_vol_struct(vol);

			rc = loc_service_unregister(hr_srv, svc_id);
			return rc;
		}
	}

	fibril_rwlock_write_unlock(&hr_volumes_lock);
	return ENOENT;
}

errno_t hr_init_extents_from_cfg(hr_volume_t *vol, hr_config_t *cfg)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno;
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
			HR_DEBUG("%s(): initing (%" PRIun ") failed, aborting\n",
			    __func__, svc_id);
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
		vol->extents[i].blkno = blkno;
		vol->extents[i].status = HR_EXT_ONLINE;

		last_bsize = bsize;
	}

	vol->bsize = last_bsize;
	vol->extent_no = cfg->dev_no;

	for (i = 0; i < HR_MAX_HOTSPARES; i++)
		vol->hotspares[i].status = HR_EXT_MISSING;

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
			HR_DEBUG("hr_fini_devs(): block_fini() on (%" PRIun ")\n",
			    vol->extents[i].svc_id);
			block_fini(vol->extents[i].svc_id);
		}
	}

	for (i = 0; i < vol->hotspare_no; i++) {
		if (vol->hotspares[i].svc_id != 0) {
			HR_DEBUG("hr_fini_devs(): block_fini() on (%" PRIun ")\n",
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
	char *fullname = NULL;
	char *devname = vol->devname;

	if (asprintf(&fullname, "devices/%s", devname) < 0)
		return ENOMEM;

	rc = loc_service_register(hr_srv, fullname, &new_id);
	if (rc != EOK) {
		HR_ERROR("unable to register device \"%s\": %s\n",
		    fullname, str_error(rc));
		free(fullname);
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
		    fullname, str_error(rc));
		goto error;
	}

	vol->svc_id = new_id;

	free(fullname);
	return EOK;
error:
	rc = loc_service_unregister(hr_srv, new_id);
	free(fullname);
	return rc;
}

errno_t hr_check_ba_range(hr_volume_t *vol, size_t cnt, uint64_t ba)
{
	if (ba + cnt > vol->data_blkno)
		return ERANGE;
	return EOK;
}

void hr_add_ba_offset(hr_volume_t *vol, uint64_t *ba)
{
	*ba = *ba + vol->data_offset;
}

void hr_update_ext_status(hr_volume_t *vol, size_t ext_idx, hr_ext_status_t s)
{
	if (vol->level != HR_LVL_0)
		assert(fibril_rwlock_is_locked(&vol->extents_lock));

	assert(fibril_rwlock_is_write_locked(&vol->states_lock));

	assert(ext_idx < vol->extent_no);

	hr_ext_status_t old = vol->extents[ext_idx].status;
	HR_WARN("\"%s\": changing extent %zu state: %s -> %s\n",
	    vol->devname, ext_idx, hr_get_ext_status_msg(old),
	    hr_get_ext_status_msg(s));
	vol->extents[ext_idx].status = s;
}

void hr_update_hotspare_status(hr_volume_t *vol, size_t hs_idx,
    hr_ext_status_t s)
{
	assert(fibril_mutex_is_locked(&vol->hotspare_lock));

	assert(hs_idx < vol->hotspare_no);

	hr_ext_status_t old = vol->hotspares[hs_idx].status;
	HR_WARN("\"%s\": changing hotspare %zu state: %s -> %s\n",
	    vol->devname, hs_idx, hr_get_ext_status_msg(old),
	    hr_get_ext_status_msg(s));
	vol->hotspares[hs_idx].status = s;
}

void hr_update_vol_status(hr_volume_t *vol, hr_vol_status_t new)
{
	assert(fibril_rwlock_is_write_locked(&vol->states_lock));

	HR_WARN("\"%s\": changing volume state: %s -> %s\n", vol->devname,
	    hr_get_vol_status_msg(vol->status), hr_get_vol_status_msg(new));
	vol->status = new;
}

void hr_update_ext_svc_id(hr_volume_t *vol, size_t ext_idx, service_id_t new)
{
	if (vol->level != HR_LVL_0)
		assert(fibril_rwlock_is_write_locked(&vol->extents_lock));

	assert(ext_idx < vol->extent_no);

	service_id_t old = vol->extents[ext_idx].svc_id;
	HR_WARN("\"%s\": changing extent no. %zu svc_id: (%" PRIun ") -> "
	    "(%" PRIun ")\n", vol->devname, ext_idx, old, new);
	vol->extents[ext_idx].svc_id = new;
}

void hr_update_hotspare_svc_id(hr_volume_t *vol, size_t hs_idx,
    service_id_t new)
{
	assert(fibril_mutex_is_locked(&vol->hotspare_lock));

	assert(hs_idx < vol->hotspare_no);

	service_id_t old = vol->hotspares[hs_idx].svc_id;
	HR_WARN("\"%s\": changing hotspare no. %zu svc_id: (%" PRIun ") -> "
	    "(%" PRIun ")\n", vol->devname, hs_idx, old, new);
	vol->hotspares[hs_idx].svc_id = new;
}

/*
 * Do a whole sync (ba = 0, cnt = 0) across all extents,
 * and update extent status. *For now*, the caller has to
 * update volume status after the syncs.
 *
 * TODO: add update_vol_status fcn ptr for each raid
 */
void hr_sync_all_extents(hr_volume_t *vol)
{
	errno_t rc;

	fibril_mutex_lock(&vol->lock);
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].status != HR_EXT_ONLINE)
			continue;
		rc = block_sync_cache(vol->extents[i].svc_id, 0, 0);
		if (rc == ENOMEM || rc == ENOTSUP)
			continue;
		if (rc != EOK) {
			if (rc == ENOENT)
				hr_update_ext_status(vol, i, HR_EXT_MISSING);
			else if (rc != EOK)
				hr_update_ext_status(vol, i, HR_EXT_FAILED);
		}
	}
	fibril_mutex_unlock(&vol->lock);
}

size_t hr_count_extents(hr_volume_t *vol, hr_ext_status_t status)
{
	if (vol->level != HR_LVL_0)
		assert(fibril_rwlock_is_locked(&vol->extents_lock));
	assert(fibril_rwlock_is_locked(&vol->states_lock));

	size_t count = 0;
	for (size_t i = 0; i < vol->extent_no; i++)
		if (vol->extents[i].status == status)
			count++;

	return count;
}

hr_range_lock_t *hr_range_lock_acquire(hr_volume_t *vol, uint64_t ba,
    uint64_t cnt)
{
	hr_range_lock_t *rl = malloc(sizeof(hr_range_lock_t));
	if (rl == NULL)
		return NULL;

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
    bool inited, hr_metadata_t *md)
{
	errno_t rc = EOK;
	struct svc_id_linked *to_add;

	to_add = malloc(sizeof(struct svc_id_linked));
	if (to_add == NULL) {
		rc = ENOMEM;
		goto error;
	}
	to_add->svc_id = svc_id;
	to_add->inited = inited;

	if (md != NULL) {
		to_add->md = malloc(sizeof(hr_metadata_t));
		if (to_add->md == NULL) {
			rc = ENOMEM;
			goto error;
		}
		to_add->md_present = true;
		memcpy(to_add->md, md, sizeof(*md));
	} else {
		to_add->md_present = false;
	}

	list_append(&to_add->link, list);

error:
	return rc;
}

static void free_svc_id_linked(struct svc_id_linked *p)
{
	if (p->md_present)
		free(p->md);
	free(p);
}

static void free_svc_id_list(list_t *list)
{
	struct svc_id_linked *dev_id;
	while (!list_empty(list)) {
		dev_id = list_pop(list, struct svc_id_linked, link);
		free_svc_id_linked(dev_id);
	}
}

static errno_t hr_fill_disk_part_svcs_list(list_t *list)
{
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

		if (disk_info.ltype == lt_none) {
			rc = hr_add_svc_linked_to_list(list, disk_svcs[i], false, NULL);
			if (rc != EOK)
				goto error;
		} else {
			size_t part_count;
			service_id_t *part_ids = NULL;
			rc = vbd_label_get_parts(vbd, disk_svcs[i], &part_ids, &part_count);
			if (rc != EOK)
				goto error;

			for (size_t j = 0; j < part_count; j++) {
				vbd_part_info_t part_info;
				rc = vbd_part_get_info(vbd, part_ids[j], &part_info);
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
	list_foreach_safe(*list, cur_link, next_link) {
		struct svc_id_linked *iter;
		iter = list_get_instance(cur_link, struct svc_id_linked, link);

		if (iter->inited)
			continue;

		errno_t rc = block_init(iter->svc_id);

		/* already used as an extent of active volume */
		/* XXX: figure out how it is with hotspares too */
		if (rc == EEXIST) {
			list_remove(cur_link);
			free_svc_id_linked(iter);
			continue;
		}

		if (rc != EOK)
			return rc;

		iter->inited = true;
	}

	return EOK;
}

static void block_fini_dev_list(list_t *list)
{
	list_foreach(*list, link, struct svc_id_linked, iter) {
		if (iter->inited) {
			block_fini(iter->svc_id);
			iter->inited = false;
		}
	}
}

static errno_t hr_util_get_matching_md_svcs_list(list_t *rlist, list_t *devlist,
    service_id_t svc_id, hr_metadata_t *md_main)
{
	errno_t rc = EOK;

	list_foreach(*devlist, link, struct svc_id_linked, iter) {
		if (iter->svc_id == svc_id)
			continue;
		void *md_block;
		hr_metadata_t md;
		rc = hr_get_metadata_block(iter->svc_id, &md_block);
		if (rc != EOK)
			goto error;
		hr_decode_metadata_from_block(md_block, &md);

		free(md_block);

		if (!hr_valid_md_magic(&md))
			continue;

		if (memcmp(md_main->uuid, md.uuid, HR_UUID_LEN) != 0)
			continue;

		/*
		 * XXX: can I assume bsize and everything is fine when
		 * UUID matches?
		 */

		rc = hr_add_svc_linked_to_list(rlist, iter->svc_id, true, &md);
		if (rc != EOK)
			goto error;
	}

	return  EOK;
error:
	free_svc_id_list(rlist);
	return rc;
}

static errno_t hr_util_assemble_from_matching_list(list_t *list)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	hr_metadata_t *main_md = NULL;
	size_t max_counter_val = 0;

	list_foreach(*list, link, struct svc_id_linked, iter) {
		/* hr_metadata_dump(iter->md); */
		if (iter->md->counter >= max_counter_val) {
			max_counter_val = iter->md->counter;
			main_md = iter->md;
		}
	}

	assert(main_md != NULL);

	hr_volume_t *vol;
	rc = hr_create_vol_struct(&vol, (hr_level_t)main_md->level,
	    main_md->devname);
	if (rc != EOK)
		goto error;

	vol->nblocks = main_md->nblocks;
	vol->data_blkno = main_md->data_blkno;
	vol->truncated_blkno = main_md->truncated_blkno;
	vol->data_offset = main_md->data_offset;
	vol->metadata_version = main_md->version;
	vol->extent_no = main_md->extent_no;
	/* vol->level = main_md->level; */
	vol->layout = main_md->layout;
	vol->strip_size = main_md->strip_size;
	vol->bsize = main_md->bsize;
	/* memcpy(vol->devname, main_md->devname, HR_DEVNAME_LEN); */

	memcpy(vol->in_mem_md, main_md, sizeof(hr_metadata_t));

	list_foreach(*list, link, struct svc_id_linked, iter) {
		vol->extents[iter->md->index].svc_id = iter->svc_id;

		uint64_t blkno;
		rc = block_get_nblocks(iter->svc_id, &blkno);
		if (rc != EOK)
			goto error;
		vol->extents[iter->md->index].blkno = blkno;

		if (iter->md->counter == max_counter_val)
			vol->extents[iter->md->index].status = HR_EXT_ONLINE;
		else
			vol->extents[iter->md->index].status = HR_EXT_INVALID;
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].status == HR_EXT_NONE)
			vol->extents[i].status = HR_EXT_MISSING;
	}

	/*
	 * TODO: something like mark md dirty or whatever
	 *	- probably will be handled by each md type differently,
	 *	  by specific function pointers
	 *	- deal with this when foreign md will be handled
	 *
	 * XXX: if thats the only thing that can change in metadata
	 * during volume runtime, then whatever, but if more
	 * things will need to be synced, think of something more clever
	 *
	 * TODO: remove from here and increment it the "first" time (if nothing
	 * happens - no state changes, no rebuild, etc) - only after the first
	 * write... but for now leave it here
	 */
	vol->in_mem_md->counter++;

	rc = vol->hr_ops.create(vol);
	if (rc != EOK)
		goto error;

	hr_metadata_save(vol, WITH_STATE_CALLBACK);

	fibril_rwlock_write_lock(&hr_volumes_lock);

	list_foreach(hr_volumes, lvolumes, hr_volume_t, other) {
		uint8_t *our_uuid = vol->in_mem_md->uuid;
		uint8_t *other_uuid = other->in_mem_md->uuid;
		if (memcmp(our_uuid, other_uuid, HR_UUID_LEN) == 0) {
			rc = EEXIST;
			fibril_rwlock_write_unlock(&hr_volumes_lock);
			goto error;
		}
	}

	/*
	 * XXX: register it here
	 * ... if it fails on EEXIST try different name... like + 1 on the end
	 *
	 * or have metadata edit utility as a part of hrctl..., or create
	 * the original name + 4 random characters, tell the user that the device
	 * was created with this new name, and add a option to hrctl to rename
	 * an active array, and then write the new dirty metadata...
	 *
	 * or just refuse to assemble a name that is already used...
	 *
	 * TODO: discuss
	 */
	rc = hr_register_volume(vol);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&hr_volumes_lock);
		goto error;
	}

	list_append(&vol->lvolumes, &hr_volumes);

	fibril_rwlock_write_unlock(&hr_volumes_lock);

	return EOK;
error:
	hr_destroy_vol_struct(vol);
	return rc;
}

static errno_t hr_fill_svcs_list_from_cfg(hr_config_t *cfg, list_t *list)
{
	errno_t rc = EOK;
	for (size_t i = 0; i < cfg->dev_no; ++i) {
		rc = hr_add_svc_linked_to_list(list, cfg->devs[i], false, NULL);
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

	list_initialize(&dev_id_list);

	if (cfg == NULL)
		rc = hr_fill_disk_part_svcs_list(&dev_id_list);
	else
		rc = hr_fill_svcs_list_from_cfg(cfg, &dev_id_list);

	if (rc != EOK)
		goto error;

	rc = block_init_dev_list(&dev_id_list);
	if (rc != EOK)
		goto error;

	struct svc_id_linked *iter;
	while (!list_empty(&dev_id_list)) {
		iter = list_pop(&dev_id_list, struct svc_id_linked, link);

		void *metadata_block;
		hr_metadata_t metadata;

		rc = hr_get_metadata_block(iter->svc_id, &metadata_block);
		if (rc != EOK)
			goto error;

		hr_decode_metadata_from_block(metadata_block, &metadata);

		free(metadata_block);

		if (!hr_valid_md_magic(&metadata)) {
			block_fini(iter->svc_id);
			free_svc_id_linked(iter);
			continue;
		}

		/* hr_metadata_dump(&metadata); */

		char *svc_name = NULL;
		rc = loc_service_get_name(iter->svc_id, &svc_name);
		if (rc != EOK)
			goto error;

		HR_DEBUG("found valid metadata on %s, "
		    "will try to match other extents\n", svc_name);

		free(svc_name);

		list_t matching_svcs_list;
		list_initialize(&matching_svcs_list);

		rc = hr_util_get_matching_md_svcs_list(&matching_svcs_list,
		    &dev_id_list, iter->svc_id, &metadata);
		if (rc != EOK)
			goto error;

		/* add current iter to list as well */
		rc = hr_add_svc_linked_to_list(&matching_svcs_list,
		    iter->svc_id, true, &metadata);
		if (rc != EOK) {
			free_svc_id_list(&matching_svcs_list);
			goto error;
		}

		/* remove matching list members from dev_id_list */
		list_foreach(matching_svcs_list, link, struct svc_id_linked,
		    iter2) {
			struct svc_id_linked *to_remove;
			list_foreach_safe(dev_id_list, cur_link, next_link) {
				to_remove = list_get_instance(cur_link,
				    struct svc_id_linked, link);
				if (to_remove->svc_id == iter2->svc_id) {
					list_remove(cur_link);
					free_svc_id_linked(to_remove);
				}
			}
		}

		rc = hr_util_assemble_from_matching_list(&matching_svcs_list);
		switch (rc) {
		case EOK:
			asm_cnt++;
			break;
		case EEXIST:
			/*
			 * A race is detected this way, because we don't want
			 * to hold the hr_volumes list lock for a long time,
			 * for all assembly attempts. XXX: discuss...
			 */
			rc = EOK;
			break;
		default:
			block_fini_dev_list(&matching_svcs_list);
			free_svc_id_list(&matching_svcs_list);
			goto error;
		}

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

	rc = block_init(hotspare);
	if (rc != EOK)
		goto error;

	uint64_t hs_blkno;
	rc = block_get_nblocks(hotspare, &hs_blkno);
	if (rc != EOK) {
		block_fini(hotspare);
		goto error;
	}

	/*
	 * TODO: make more flexible, when will have foreign md, the calculation
	 * will differ, maybe something new like vol->md_hs_blkno will be enough
	 */
	if (hs_blkno < vol->truncated_blkno - HR_META_SIZE) {
		rc = EINVAL;
		block_fini(hotspare);
		goto error;
	}

	size_t hs_idx = vol->hotspare_no;

	vol->hotspare_no++;

	hr_update_hotspare_svc_id(vol, hs_idx, hotspare);
	hr_update_hotspare_status(vol, hs_idx, HR_EXT_HOTSPARE);

	hr_mark_vol_state_dirty(vol);
error:
	fibril_mutex_unlock(&vol->hotspare_lock);
	return rc;
}

/** @}
 */
