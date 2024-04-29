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

	rc = block_init(sid);
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

	rc = block_init(sid);
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
