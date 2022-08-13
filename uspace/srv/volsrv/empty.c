/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup volsrv
 * @{
 */
/**
 * @file Empty partition handling
 * @brief
 */

#include <block.h>
#include <errno.h>
#include <io/log.h>
#include <label/empty.h>
#include <loc.h>
#include <stdlib.h>

#include "empty.h"

static errno_t empty_get_bsize(void *, size_t *);
static errno_t empty_get_nblocks(void *, aoff64_t *);
static errno_t empty_read(void *, aoff64_t, size_t, void *);
static errno_t empty_write(void *, aoff64_t, size_t, const void *);

/** Provide disk access to liblabel */
static label_bd_ops_t empty_bd_ops = {
	.get_bsize = empty_get_bsize,
	.get_nblocks = empty_get_nblocks,
	.read = empty_read,
	.write = empty_write
};

errno_t volsrv_part_is_empty(service_id_t sid, bool *rempty)
{
	errno_t rc;
	label_bd_t lbd;

	rc = block_init(sid, 2048);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error opening "
		    "block device service %zu", sid);
		return EIO;
	}

	lbd.ops = &empty_bd_ops;
	lbd.arg = (void *)&sid;

	rc = label_bd_is_empty(&lbd, rempty);

	block_fini(sid);
	return rc;
}

errno_t volsrv_part_empty(service_id_t sid)
{
	errno_t rc;
	label_bd_t lbd;

	rc = block_init(sid, 2048);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error opening "
		    "block device service %zu", sid);
		return EIO;
	}

	lbd.ops = &empty_bd_ops;
	lbd.arg = (void *)&sid;

	rc = label_bd_empty(&lbd);

	block_fini(sid);
	return rc;
}

/** Get block size wrapper for liblabel */
static errno_t empty_get_bsize(void *arg, size_t *bsize)
{
	service_id_t svc_id = *(service_id_t *)arg;
	return block_get_bsize(svc_id, bsize);
}

/** Get number of blocks wrapper for liblabel */
static errno_t empty_get_nblocks(void *arg, aoff64_t *nblocks)
{
	service_id_t svc_id = *(service_id_t *)arg;
	return block_get_nblocks(svc_id, nblocks);
}

/** Read blocks wrapper for liblabel */
static errno_t empty_read(void *arg, aoff64_t ba, size_t cnt, void *buf)
{
	service_id_t svc_id = *(service_id_t *)arg;
	return block_read_direct(svc_id, ba, cnt, buf);
}

/** Write blocks wrapper for liblabel */
static errno_t empty_write(void *arg, aoff64_t ba, size_t cnt, const void *data)
{
	service_id_t svc_id = *(service_id_t *)arg;
	return block_write_direct(svc_id, ba, cnt, data);
}

/** @}
 */
