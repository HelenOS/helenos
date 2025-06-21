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

#include <block.h>
#include <errno.h>
#include <hr.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#include "io.h"
#include "parity_stripe.h"
#include "util.h"
#include "var.h"

/** Wrapper for block_write_direct(), never returns ENOMEM */
errno_t hr_write_direct(service_id_t service_id, uint64_t ba, size_t cnt,
    const void *data)
{
	errno_t rc;
	while ((rc = block_write_direct(service_id, ba, cnt, data)) == ENOMEM)
		fibril_usleep(MSEC2USEC(250)); /* sleep 250ms */

	if (rc == EAGAIN)
		rc = EIO;

	return rc;
}

/** Wrapper for block_read_direct(), never returns ENOMEM */
errno_t hr_read_direct(service_id_t service_id, uint64_t ba, size_t cnt,
    void *data)
{
	errno_t rc;
	while ((rc = block_read_direct(service_id, ba, cnt, data)) == ENOMEM)
		fibril_usleep(MSEC2USEC(250)); /* sleep 250ms */

	if (rc == EAGAIN)
		rc = EIO;

	return rc;
}

/** Wrapper for block_sync_cache(), never returns ENOMEM */
errno_t hr_sync_cache(service_id_t service_id, uint64_t ba, size_t cnt)
{
	errno_t rc;
	while ((rc = block_sync_cache(service_id, ba, cnt)) == ENOMEM)
		fibril_usleep(MSEC2USEC(250)); /* sleep 250ms */

	if (rc == EAGAIN)
		rc = EIO;

	return rc;
}

errno_t hr_io_worker(void *arg)
{
	hr_io_t *io = arg;

	errno_t rc;
	size_t e = io->extent;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	switch (io->type) {
	case HR_BD_READ:
		rc = hr_read_direct(extents[e].svc_id, io->ba, io->cnt,
		    io->data_read);
		break;
	case HR_BD_WRITE:
		rc = hr_write_direct(extents[e].svc_id, io->ba, io->cnt,
		    io->data_write);
		break;
	default:
		assert(0);
	}

	if (rc != EOK)
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);

	return rc;
}

errno_t hr_io_raid5_basic_reader(void *arg)
{
	errno_t rc;

	hr_io_raid5_t *io = arg;

	size_t ext_idx = io->extent;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	rc = hr_read_direct(extents[ext_idx].svc_id, io->ba, io->cnt,
	    io->data_read);
	if (rc != EOK)
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);

	return rc;
}

errno_t hr_io_raid5_reader(void *arg)
{
	errno_t rc;

	hr_io_raid5_t *io = arg;
	hr_stripe_t *stripe = io->stripe;

	size_t ext_idx = io->extent;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	rc = hr_read_direct(extents[ext_idx].svc_id, io->ba, io->cnt,
	    io->data_read);
	if (rc != EOK) {
		hr_stripe_parity_abort(stripe);
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);
	}

	hr_stripe_commit_parity(stripe, io->strip_off, io->data_read,
	    io->cnt * io->vol->bsize);

	return rc;
}

errno_t hr_io_raid5_basic_writer(void *arg)
{
	errno_t rc;

	hr_io_raid5_t *io = arg;

	size_t ext_idx = io->extent;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	rc = hr_write_direct(extents[ext_idx].svc_id, io->ba, io->cnt,
	    io->data_write);
	if (rc != EOK)
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);

	return rc;
}

errno_t hr_io_raid5_writer(void *arg)
{
	errno_t rc;

	hr_io_raid5_t *io = arg;
	hr_stripe_t *stripe = io->stripe;

	size_t ext_idx = io->extent;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	hr_stripe_commit_parity(stripe, io->strip_off, io->data_write,
	    io->cnt * io->vol->bsize);

	hr_stripe_wait_for_parity_commits(stripe);
	if (stripe->abort)
		return EAGAIN;

	rc = hr_write_direct(extents[ext_idx].svc_id, io->ba, io->cnt,
	    io->data_write);
	if (rc != EOK)
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);

	return rc;
}

errno_t hr_io_raid5_noop_writer(void *arg)
{
	hr_io_raid5_t *io = arg;
	hr_stripe_t *stripe = io->stripe;

	hr_stripe_commit_parity(stripe, io->strip_off, io->data_write,
	    io->cnt * io->vol->bsize);

	return EOK;
}

errno_t hr_io_raid5_parity_getter(void *arg)
{
	hr_io_raid5_t *io = arg;
	hr_stripe_t *stripe = io->stripe;
	size_t bsize = stripe->vol->bsize;

	hr_stripe_wait_for_parity_commits(stripe);
	if (stripe->abort)
		return EAGAIN;

	memcpy(io->data_read, stripe->parity + io->strip_off, io->cnt * bsize);

	return EOK;
}

errno_t hr_io_raid5_subtract_writer(void *arg)
{
	errno_t rc;

	hr_io_raid5_t *io = arg;
	hr_stripe_t *stripe = io->stripe;

	size_t ext_idx = io->extent;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	uint8_t *data = hr_malloc_waitok(io->cnt * io->vol->bsize);

	rc = hr_read_direct(extents[ext_idx].svc_id, io->ba, io->cnt, data);
	if (rc != EOK) {
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);
		hr_stripe_parity_abort(stripe);
		free(data);
		return rc;
	}

	fibril_mutex_lock(&stripe->parity_lock);

	hr_raid5_xor(stripe->parity + io->strip_off, data,
	    io->cnt * io->vol->bsize);

	hr_raid5_xor(stripe->parity + io->strip_off, io->data_write,
	    io->cnt * io->vol->bsize);

	stripe->ps_added++;
	fibril_condvar_broadcast(&stripe->ps_added_cv);
	fibril_mutex_unlock(&stripe->parity_lock);

	hr_stripe_wait_for_parity_commits(stripe);
	if (stripe->abort)
		return EAGAIN;

	rc = hr_write_direct(extents[ext_idx].svc_id, io->ba, io->cnt,
	    io->data_write);
	if (rc != EOK)
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);

	free(data);

	return rc;
}

errno_t hr_io_raid5_reconstruct_reader(void *arg)
{
	errno_t rc;

	hr_io_raid5_t *io = arg;
	hr_stripe_t *stripe = io->stripe;

	size_t ext_idx = io->extent;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	uint8_t *data = hr_malloc_waitok(io->cnt * io->vol->bsize);

	rc = hr_write_direct(extents[ext_idx].svc_id, io->ba, io->cnt, data);
	if (rc != EOK) {
		hr_stripe_parity_abort(stripe);
		io->vol->hr_ops.ext_state_cb(io->vol, io->extent, rc);
		free(data);
		return rc;
	}

	hr_stripe_commit_parity(stripe, io->strip_off, data,
	    io->cnt * io->vol->bsize);

	free(data);

	return EOK;
}

errno_t hr_io_raid5_parity_writer(void *arg)
{
	errno_t rc;

	hr_io_raid5_t *io = arg;
	hr_stripe_t *stripe = io->stripe;

	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;

	hr_stripe_wait_for_parity_commits(stripe);

	if (stripe->abort)
		return EAGAIN;

	rc = hr_write_direct(extents[io->extent].svc_id, io->ba, io->cnt,
	    stripe->parity + io->strip_off);
	if (rc != EOK)
		io->vol->hr_ops.ext_state_cb(io->vol, stripe->p_extent, rc);

	return rc;
}

/** @}
 */
