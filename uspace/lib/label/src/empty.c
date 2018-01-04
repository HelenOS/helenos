/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup liblabel
 * @{
 */
/**
 * @file Empty partition handling
 * @brief
 */

#include <errno.h>
#include <io/log.h>
#include <label/empty.h>
#include <loc.h>
#include <stdlib.h>

/*
 * The partition will be considered empty if at least a minimum number of
 * bytes *and* blocks (whichever is larger) at the beginning and end of
 * the partition is zero.
 */
enum {
	min_empty_bytes = 16384,
	/*
	 * First block in ISO 9660 that cannot be empty is the first
	 * volume descriptor at LBA 16
	 */
	min_empty_blocks = 17
};

static bool mem_is_zero(void *buf, size_t size)
{
	uint8_t *bp;
	size_t i;

	bp = (uint8_t *)buf;
	for (i = 0; i < size; i++) {
		if (bp[i] != 0)
			return false;
	}

	return true;
}

/** Calculate number of blocks to check.
 *
 * Will store to @a *ncb the number of blocks that should be checked
 * at the beginning and end of device each.
 *
 * @param nblocks Total number of blocks on block device
 * @param block_size Block size
 * @param ncb Place to store number of blocks to check.
 */
static void calc_num_check_blocks(aoff64_t nblocks, size_t block_size,
    aoff64_t *ncb)
{
	aoff64_t n;

	/* Check first 16 kiB / 16 blocks, whichever is more */
	n = (min_empty_bytes + block_size - 1) / block_size;
	if (n < min_empty_blocks)
		n = min_empty_blocks;
	/*
	 * Limit to half of the device so we do not process the same blocks
	 * twice
	 */
	if (n > (nblocks + 1) / 2)
		n = (nblocks + 1) / 2;

	*ncb = n;
}

errno_t label_bd_is_empty(label_bd_t *bd, bool *rempty)
{
	errno_t rc;
	void *buf = NULL;
	aoff64_t nblocks;
	aoff64_t n;
	aoff64_t i;
	size_t block_size;
	bool empty;

	rc = bd->ops->get_bsize(bd->arg, &block_size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting "
		    "block size.");
		rc = EIO;
		goto error;
	}

	rc = bd->ops->get_nblocks(bd->arg, &nblocks);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting "
		    "number of blocks.");
		rc = EIO;
		goto error;
	}

	calc_num_check_blocks(nblocks, block_size, &n);

	buf = calloc(block_size, 1);
	if (buf == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error allocating buffer.");
		rc = ENOMEM;
		goto error;
	}

	empty = true;

	for (i = 0; i < n; i++) {
		rc = bd->ops->read(bd->arg, i, 1, buf);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
			    "reading blocks.");
			rc = EIO;
			goto error;
		}

		if (!mem_is_zero(buf, block_size)) {
			empty = false;
			goto done;
		}
	}

	for (i = 0; i < n; i++) {
		rc = bd->ops->read(bd->arg, nblocks - n + i, 1, buf);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
			    "reading blocks.");
			rc = EIO;
			goto error;
		}

		if (!mem_is_zero(buf, block_size)) {
			empty = false;
			goto done;
		}
	}

done:
	free(buf);
	*rempty = empty;
	return EOK;
error:
	if (buf != NULL)
		free(buf);
	return rc;
}

errno_t label_bd_empty(label_bd_t *bd)
{
	errno_t rc;
	void *buf = NULL;
	aoff64_t nblocks;
	aoff64_t n;
	aoff64_t i;
	size_t block_size;

	rc = bd->ops->get_bsize(bd->arg, &block_size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting "
		    "block size.");
		rc = EIO;
		goto error;
	}

	rc = bd->ops->get_nblocks(bd->arg, &nblocks);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting "
		    "number of blocks.");
		rc = EIO;
		goto error;
	}

	calc_num_check_blocks(nblocks, block_size, &n);

	buf = calloc(block_size, 1);
	if (buf == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error allocating buffer.");
		rc = ENOMEM;
		goto error;
	}

	for (i = 0; i < n; i++) {
		rc = bd->ops->write(bd->arg, i, 1, buf);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
			    "reading blocks.");
			rc = EIO;
			goto error;
		}
	}

	for (i = 0; i < n; i++) {
		rc = bd->ops->write(bd->arg, nblocks - n + i, 1, buf);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
			    "reading blocks.");
			rc = EIO;
			goto error;
		}
	}

	free(buf);
	return EOK;
error:
	if (buf != NULL)
		free(buf);
	return rc;
}

errno_t label_part_empty(label_part_t *part)
{
	errno_t rc;
	void *buf = NULL;
	aoff64_t block0;
	aoff64_t nblocks;
	aoff64_t n;
	aoff64_t i;
	size_t block_size;
	label_bd_t *bd;

	bd = &part->label->bd;

	block_size = part->label->block_size;
	block0 = part->block0;
	nblocks = part->nblocks;

	calc_num_check_blocks(nblocks, block_size, &n);

	buf = calloc(block_size, 1);
	if (buf == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error allocating buffer.");
		rc = ENOMEM;
		goto error;
	}

	for (i = 0; i < n; i++) {
		rc = bd->ops->write(bd->arg, block0 + i, 1, buf);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
			    "reading blocks.");
			rc = EIO;
			goto error;
		}
	}

	for (i = 0; i < n; i++) {
		rc = bd->ops->write(bd->arg, block0 + nblocks - n + i, 1, buf);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
			    "reading blocks.");
			rc = EIO;
			goto error;
		}
	}

	free(buf);
	return EOK;
error:
	if (buf != NULL)
		free(buf);
	return rc;
}

/** @}
 */
