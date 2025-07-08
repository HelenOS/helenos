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

#include <stdlib.h>
#include <stdio.h>
#include <str.h>

#include "io.h"
#include "parity_stripe.h"
#include "util.h"
#include "var.h"

static void hr_execute_write_stripe_degraded_mixed(hr_stripe_t *, size_t);
static void hr_execute_write_stripe_degraded(hr_stripe_t *, size_t);
static void hr_execute_write_stripe_optimal_reconstruct(hr_stripe_t *);
static void hr_execute_write_stripe_optimal_subtract(hr_stripe_t *);
static void hr_execute_write_stripe(hr_stripe_t *, size_t);
static void hr_execute_read_stripe(hr_stripe_t *, size_t);
static void hr_execute_write_stripe_degraded_good(hr_stripe_t *, size_t);
static bool hr_stripe_range_non_extension(const range_t *, const range_t *,
    range_t *);
static size_t hr_stripe_merge_extent_spans(hr_stripe_t *, size_t, range_t [2]);
static void hr_stripe_extend_range(range_t *, const range_t *);
static bool hr_ranges_overlap(const range_t *, const range_t *, range_t *);

hr_stripe_t *hr_create_stripes(hr_volume_t *vol, uint64_t strip_size,
    size_t cnt, bool write)
{
	hr_stripe_t *stripes = hr_calloc_waitok(cnt, sizeof(*stripes));

	for (size_t i = 0; i < cnt; i++) {
		fibril_mutex_initialize(&stripes[i].parity_lock);
		fibril_condvar_initialize(&stripes[i].ps_added_cv);
		stripes[i].vol = vol;
		stripes[i].write = write;
		stripes[i].parity = hr_calloc_waitok(1, strip_size);
		stripes[i].parity_size = strip_size;
		stripes[i].extent_span = hr_calloc_waitok(vol->extent_no,
		    sizeof(*stripes[i].extent_span));
	}

	return stripes;
}

void hr_destroy_stripes(hr_stripe_t *stripes, size_t cnt)
{
	if (stripes == NULL)
		return;

	for (size_t i = 0; i < cnt; i++) {
		if (stripes[i].parity != NULL)
			free(stripes[i].parity);
		if (stripes[i].extent_span != NULL)
			free(stripes[i].extent_span);
	}

	free(stripes);
}

void hr_reset_stripe(hr_stripe_t *stripe)
{
	memset(stripe->parity, 0, stripe->parity_size);
	stripe->ps_added = 0;
	stripe->ps_to_be_added = 0;
	stripe->p_count_final = false;

	stripe->rc = EOK;
	stripe->abort = false;
	stripe->done = false;
}

void hr_stripe_commit_parity(hr_stripe_t *stripe, uint64_t strip_off,
    const void *data, uint64_t size)
{
	fibril_mutex_lock(&stripe->parity_lock);
	hr_raid5_xor(stripe->parity + strip_off, data, size);
	stripe->ps_added++;
	fibril_condvar_broadcast(&stripe->ps_added_cv);
	fibril_mutex_unlock(&stripe->parity_lock);
}

void hr_stripe_wait_for_parity_commits(hr_stripe_t *stripe)
{
	fibril_mutex_lock(&stripe->parity_lock);
	while ((!stripe->p_count_final ||
	    stripe->ps_added < stripe->ps_to_be_added) && !stripe->abort) {
		fibril_condvar_wait(&stripe->ps_added_cv, &stripe->parity_lock);
	}
	fibril_mutex_unlock(&stripe->parity_lock);
}

void hr_stripe_parity_abort(hr_stripe_t *stripe)
{
	fibril_mutex_lock(&stripe->parity_lock);
	stripe->abort = true;
	fibril_condvar_broadcast(&stripe->ps_added_cv);
	fibril_mutex_unlock(&stripe->parity_lock);
}

void hr_execute_stripe(hr_stripe_t *stripe, size_t bad_extent)
{
	if (stripe->write)
		hr_execute_write_stripe(stripe, bad_extent);
	else
		hr_execute_read_stripe(stripe, bad_extent);
}

void hr_wait_for_stripe(hr_stripe_t *stripe)
{
	stripe->rc = hr_fgroup_wait(stripe->worker_group, NULL, NULL);
	if (stripe->rc == EAGAIN)
		hr_reset_stripe(stripe);
	else
		stripe->done = true;
}

static void hr_execute_write_stripe_degraded_good(hr_stripe_t *stripe,
    size_t bad_extent)
{
	hr_volume_t *vol = stripe->vol;

	stripe->ps_to_be_added = stripe->strips_touched; /* writers */
	stripe->ps_to_be_added += stripe->range_count; /* parity readers */
	stripe->p_count_final = true;

	size_t worker_cnt = stripe->strips_touched + stripe->range_count * 2;
	stripe->worker_group = hr_fgroup_create(vol->fge, worker_cnt);

	for (size_t e = 0; e < vol->extent_no; e++) {
		if (e == bad_extent || e == stripe->p_extent)
			continue;
		if (stripe->extent_span[e].cnt == 0)
			continue;

		hr_io_raid5_t *io = hr_fgroup_alloc(stripe->worker_group);
		io->extent = e;
		io->data_write = stripe->extent_span[e].data_write;
		io->ba = stripe->extent_span[e].range.start;
		io->cnt = stripe->extent_span[e].cnt;
		io->strip_off = stripe->extent_span[e].strip_off * vol->bsize;
		io->vol = vol;
		io->stripe = stripe;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_subtract_writer, io);
	}

	for (size_t r = 0; r < stripe->range_count; r++) {
		hr_io_raid5_t *p_reader = hr_fgroup_alloc(stripe->worker_group);
		p_reader->extent = stripe->p_extent;
		p_reader->ba = stripe->total_height[r].start;
		p_reader->cnt = stripe->total_height[r].end -
		    stripe->total_height[r].start + 1;
		p_reader->vol = vol;
		p_reader->stripe = stripe;

		p_reader->strip_off = p_reader->ba;
		hr_sub_data_offset(vol, &p_reader->strip_off);
		p_reader->strip_off %= vol->strip_size / vol->bsize;
		p_reader->strip_off *= vol->bsize;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_reconstruct_reader, p_reader);

		hr_io_raid5_t *p_writer = hr_fgroup_alloc(stripe->worker_group);
		p_writer->extent = stripe->p_extent;
		p_writer->ba = stripe->total_height[r].start;
		p_writer->cnt = stripe->total_height[r].end -
		    stripe->total_height[r].start + 1;
		p_writer->vol = vol;
		p_writer->stripe = stripe;

		p_writer->strip_off = p_writer->ba;
		hr_sub_data_offset(vol, &p_writer->strip_off);
		p_writer->strip_off %= vol->strip_size / vol->bsize;
		p_writer->strip_off *= vol->bsize;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_parity_writer, p_writer);
	}
}

static void hr_execute_write_stripe_degraded_mixed(hr_stripe_t *stripe,
    size_t bad_extent)
{
	hr_volume_t *vol = stripe->vol;

	size_t worker_cnt = (vol->extent_no - 2) * 3 + 3; /* upper bound */
	stripe->worker_group = hr_fgroup_create(vol->fge, worker_cnt);

	stripe->ps_to_be_added = 1;

	hr_io_raid5_t *nop_write = hr_fgroup_alloc(stripe->worker_group);
	nop_write->ba = stripe->extent_span[bad_extent].range.start;
	nop_write->cnt = stripe->extent_span[bad_extent].cnt;
	nop_write->strip_off =
	    stripe->extent_span[bad_extent].strip_off * vol->bsize;
	nop_write->data_write = stripe->extent_span[bad_extent].data_write;
	nop_write->vol = vol;
	nop_write->stripe = stripe;

	hr_fgroup_submit(stripe->worker_group, hr_io_raid5_noop_writer,
	    nop_write);

	for (size_t e = 0; e < vol->extent_no; e++) {
		if (e == bad_extent || e == stripe->p_extent)
			continue;

		range_t uncommon = { 0, 0 };
		bool has_uncommon;
		has_uncommon = hr_stripe_range_non_extension(
		    &stripe->extent_span[bad_extent].range,
		    &stripe->extent_span[e].range,
		    &uncommon);

		if (stripe->extent_span[e].cnt == 0 || has_uncommon) {
			stripe->ps_to_be_added++;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			if (stripe->extent_span[bad_extent].cnt == 0) {
				io->ba =
				    stripe->extent_span[bad_extent].range.start;
				io->cnt = stripe->extent_span[bad_extent].cnt;
			} else {
				io->ba = uncommon.start;
				io->cnt = uncommon.end - uncommon.start + 1;
			}
			io->strip_off =
			    stripe->extent_span[bad_extent].strip_off *
			    vol->bsize;
			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_reconstruct_reader, io);

			if (stripe->extent_span[e].cnt == 0)
				continue;
		}

		range_t overlap_range;
		bool overlap_up = true;
		if (hr_ranges_overlap(&stripe->extent_span[e].range,
		    &stripe->extent_span[bad_extent].range,
		    &overlap_range)) {
			stripe->ps_to_be_added++;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			io->ba = overlap_range.start;
			io->cnt = overlap_range.end - overlap_range.start + 1;

			size_t diff = overlap_range.start -
			    stripe->extent_span[e].range.start;

			io->strip_off =
			    (stripe->extent_span[e].strip_off + diff) *
			    vol->bsize;

			io->data_write = stripe->extent_span[e].data_write;
			io->data_write += diff * vol->bsize;
			if (diff == 0)
				overlap_up = false;

			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_writer, io);
		}

		bool has_independent;
		range_t independent = { 0, 0 };
		has_independent = hr_stripe_range_non_extension(
		    &stripe->extent_span[e].range,
		    &stripe->extent_span[bad_extent].range,
		    &independent);
		if (has_independent) {
			stripe->ps_to_be_added++;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			io->ba = independent.start;
			io->cnt = independent.end - independent.start + 1;
			size_t diff = 0;
			if (!overlap_up) {
				diff = overlap_range.end -
				    overlap_range.start + 1;
			}
			io->strip_off =
			    (stripe->extent_span[e].strip_off + diff) *
			    vol->bsize;
			io->data_write = stripe->extent_span[e].data_write;
			io->data_write += diff * vol->bsize;
			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_subtract_writer, io);
		}
	}

	bool has_independent = false;
	range_t independent = { 0, 0 };
	for (size_t r = 0; r < stripe->range_count; r++) {
		has_independent = hr_stripe_range_non_extension(
		    &stripe->total_height[r],
		    &stripe->extent_span[bad_extent].range,
		    &independent);
		if (has_independent) {
			stripe->ps_to_be_added++;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = stripe->p_extent;
			io->ba = independent.start;
			io->cnt = independent.end - independent.start + 1;

			io->strip_off = io->ba;
			hr_sub_data_offset(vol, &io->strip_off);
			io->strip_off %= vol->strip_size / vol->bsize;
			io->strip_off *= vol->bsize;

			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_reconstruct_reader, io);
		}

		hr_io_raid5_t *pio = hr_fgroup_alloc(stripe->worker_group);
		pio->extent = stripe->p_extent;
		pio->ba = stripe->total_height[r].start;
		pio->cnt = stripe->total_height[r].end -
		    stripe->total_height[r].start + 1;
		pio->strip_off = pio->ba;
		hr_sub_data_offset(vol, &pio->strip_off);
		pio->strip_off %= vol->strip_size / vol->bsize;
		pio->strip_off *= vol->bsize;
		pio->vol = vol;
		pio->stripe = stripe;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_parity_writer, pio);
	}

	stripe->p_count_final = true;
	fibril_condvar_broadcast(&stripe->ps_added_cv);
}

static void hr_execute_write_stripe_degraded(hr_stripe_t *stripe,
    size_t bad_extent)
{
	hr_volume_t *vol = stripe->vol;

	/* parity is bad, issue non-redundant writes */
	if (bad_extent == stripe->p_extent) {
		stripe->worker_group =
		    hr_fgroup_create(vol->fge, stripe->strips_touched);

		for (size_t e = 0; e < vol->extent_no; e++) {
			if (e == bad_extent)
				continue;
			if (stripe->extent_span[e].cnt == 0)
				continue;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			io->data_write = stripe->extent_span[e].data_write;
			io->ba = stripe->extent_span[e].range.start;
			io->cnt = stripe->extent_span[e].cnt;
			io->strip_off =
			    stripe->extent_span[e].strip_off * vol->bsize;
			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_basic_writer, io);
		}

		return;
	}

	stripe->range_count = hr_stripe_merge_extent_spans(stripe,
	    vol->extent_no, stripe->total_height);

	if (stripe->extent_span[bad_extent].cnt > 0)
		hr_execute_write_stripe_degraded_mixed(stripe, bad_extent);
	else
		hr_execute_write_stripe_degraded_good(stripe, bad_extent);
}

static void hr_execute_write_stripe_optimal_reconstruct(hr_stripe_t *stripe)
{
	hr_volume_t *vol = stripe->vol;

	stripe->range_count = hr_stripe_merge_extent_spans(stripe,
	    vol->extent_no, stripe->total_height);

	bool full_stripe = false;
	size_t worker_cnt;
	if (stripe->strips_touched == vol->extent_no - 1 &&
	    stripe->partial_strips_touched == 0) {
		/* full-stripe */
		worker_cnt = stripe->strips_touched; /* writers */
		worker_cnt += 1; /* parity writer */

		stripe->ps_to_be_added = stripe->strips_touched;
		stripe->p_count_final = true;

		full_stripe = true;
	} else {
		worker_cnt = stripe->strips_touched; /* writers */

		/* readers (upper bound) */
		worker_cnt += (vol->extent_no - 1) - stripe->strips_touched;
		worker_cnt += stripe->partial_strips_touched;

		worker_cnt += stripe->range_count; /* parity writer(s) */

		stripe->ps_to_be_added = stripe->strips_touched; /* writers */
	}

	stripe->worker_group = hr_fgroup_create(vol->fge, worker_cnt);

	for (size_t e = 0; e < vol->extent_no; e++) {
		if (e == stripe->p_extent)
			continue;

		if (stripe->extent_span[e].cnt == 0)
			continue;

		hr_io_raid5_t *io = hr_fgroup_alloc(stripe->worker_group);
		io->extent = e;
		io->data_write = stripe->extent_span[e].data_write;
		io->ba = stripe->extent_span[e].range.start;
		io->cnt = stripe->extent_span[e].cnt;
		io->strip_off = stripe->extent_span[e].strip_off * vol->bsize;
		io->vol = vol;
		io->stripe = stripe;

		hr_fgroup_submit(stripe->worker_group, hr_io_raid5_writer, io);
	}

	for (size_t r = 0; r < stripe->range_count; r++) {
		if (full_stripe)
			goto skip_readers;
		for (size_t e = 0; e < vol->extent_no; e++) {
			if (e == stripe->p_extent)
				continue;

			range_t range_extension = { 0, 0 };

			bool need_reader = false;
			if (stripe->extent_span[e].cnt == 0) {
				range_extension = stripe->total_height[r];
				need_reader = true;
			} else {
				need_reader = hr_stripe_range_non_extension(
				    &stripe->total_height[r],
				    &stripe->extent_span[e].range,
				    &range_extension);
			}

			if (need_reader) {
				stripe->ps_to_be_added++;

				hr_io_raid5_t *io =
				    hr_fgroup_alloc(stripe->worker_group);
				io->extent = e;
				io->ba = range_extension.start;
				io->cnt = range_extension.end -
				    range_extension.start + 1;
				io->vol = vol;
				io->stripe = stripe;

				io->strip_off = io->ba;
				hr_sub_data_offset(vol, &io->strip_off);
				io->strip_off %= vol->strip_size / vol->bsize;
				io->strip_off *= vol->bsize;

				hr_fgroup_submit(stripe->worker_group,
				    hr_io_raid5_reconstruct_reader, io);
			}
		}

		stripe->p_count_final = true;
		fibril_condvar_broadcast(&stripe->ps_added_cv);

	skip_readers:

		/* parity writer */
		hr_io_raid5_t *io = hr_fgroup_alloc(stripe->worker_group);
		io->extent = stripe->p_extent;
		io->ba = stripe->total_height[r].start;
		io->cnt = stripe->total_height[r].end -
		    stripe->total_height[r].start + 1;
		io->vol = vol;
		io->stripe = stripe;

		io->strip_off = io->ba;
		hr_sub_data_offset(vol, &io->strip_off);
		io->strip_off %= vol->strip_size / vol->bsize;
		io->strip_off *= vol->bsize;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_parity_writer, io);
	}
}

static void hr_execute_write_stripe_optimal_subtract(hr_stripe_t *stripe)
{
	hr_volume_t *vol = stripe->vol;

	stripe->range_count = hr_stripe_merge_extent_spans(stripe,
	    vol->extent_no, stripe->total_height);

	size_t worker_cnt;
	worker_cnt = stripe->strips_touched; /* writers */
	worker_cnt += stripe->range_count * 2; /* parity readers & writers */

	stripe->ps_to_be_added = stripe->strips_touched; /* writers */
	stripe->ps_to_be_added += stripe->range_count; /* parity readers */
	stripe->p_count_final = true;

	stripe->worker_group = hr_fgroup_create(vol->fge, worker_cnt);

	for (size_t e = 0; e < vol->extent_no; e++) {
		if (e == stripe->p_extent)
			continue;

		if (stripe->extent_span[e].cnt == 0)
			continue;

		hr_io_raid5_t *io = hr_fgroup_alloc(stripe->worker_group);
		io->extent = e;
		io->data_write = stripe->extent_span[e].data_write;
		io->ba = stripe->extent_span[e].range.start;
		io->cnt = stripe->extent_span[e].cnt;
		io->strip_off = stripe->extent_span[e].strip_off * vol->bsize;
		io->vol = vol;
		io->stripe = stripe;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_subtract_writer, io);
	}

	for (size_t r = 0; r < stripe->range_count; r++) {
		hr_io_raid5_t *p_reader = hr_fgroup_alloc(stripe->worker_group);
		p_reader->extent = stripe->p_extent;
		p_reader->ba = stripe->total_height[r].start;
		p_reader->cnt = stripe->total_height[r].end -
		    stripe->total_height[r].start + 1;
		p_reader->vol = vol;
		p_reader->stripe = stripe;

		p_reader->strip_off = p_reader->ba;
		hr_sub_data_offset(vol, &p_reader->strip_off);
		p_reader->strip_off %= vol->strip_size / vol->bsize;
		p_reader->strip_off *= vol->bsize;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_reconstruct_reader, p_reader);

		hr_io_raid5_t *p_writer = hr_fgroup_alloc(stripe->worker_group);
		p_writer->extent = stripe->p_extent;
		p_writer->ba = stripe->total_height[r].start;
		p_writer->cnt = stripe->total_height[r].end -
		    stripe->total_height[r].start + 1;
		p_writer->vol = vol;
		p_writer->stripe = stripe;

		p_writer->strip_off = p_writer->ba;
		hr_sub_data_offset(vol, &p_writer->strip_off);
		p_writer->strip_off %= vol->strip_size / vol->bsize;
		p_writer->strip_off *= vol->bsize;

		hr_fgroup_submit(stripe->worker_group,
		    hr_io_raid5_parity_writer, p_writer);
	}

}

static void hr_execute_write_stripe(hr_stripe_t *stripe, size_t bad_extent)
{
	hr_volume_t *vol = stripe->vol;

	if (bad_extent < vol->extent_no) {
		hr_execute_write_stripe_degraded(stripe, bad_extent);
		return;
	}

	if (stripe->subtract)
		hr_execute_write_stripe_optimal_subtract(stripe);
	else
		hr_execute_write_stripe_optimal_reconstruct(stripe);
}

static void hr_execute_read_stripe(hr_stripe_t *stripe, size_t bad_extent)
{
	hr_volume_t *vol = stripe->vol;

	/* no parity involved */
	if (bad_extent == vol->extent_no ||
	    bad_extent == stripe->p_extent ||
	    stripe->extent_span[bad_extent].cnt == 0) {
		stripe->worker_group =
		    hr_fgroup_create(vol->fge, stripe->strips_touched);
		for (size_t e = 0; e < vol->extent_no; e++) {
			if (e == bad_extent || e == stripe->p_extent)
				continue;
			if (stripe->extent_span[e].cnt == 0)
				continue;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			io->data_read = stripe->extent_span[e].data_read;
			io->ba = stripe->extent_span[e].range.start;
			io->cnt = stripe->extent_span[e].cnt;
			io->strip_off =
			    stripe->extent_span[e].strip_off * vol->bsize;
			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_basic_reader, io);
		}

		return;
	}

	/* parity involved */

	size_t worker_cnt = (vol->extent_no - 2) * 2 + 1; /* upper bound */
	stripe->worker_group = hr_fgroup_create(vol->fge, worker_cnt);

	stripe->ps_to_be_added = 0;

	for (size_t e = 0; e < vol->extent_no; e++) {
		if (e == bad_extent || e == stripe->p_extent)
			continue;

		range_t uncommon = { 0, 0 };
		bool has_uncommon;
		has_uncommon = hr_stripe_range_non_extension(
		    &stripe->extent_span[bad_extent].range,
		    &stripe->extent_span[e].range,
		    &uncommon);

		if (stripe->extent_span[e].cnt == 0 || has_uncommon) {

			stripe->ps_to_be_added++;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			if (stripe->extent_span[bad_extent].cnt == 0) {
				io->ba =
				    stripe->extent_span[bad_extent].range.start;
				io->cnt = stripe->extent_span[bad_extent].cnt;
			} else {
				io->ba = uncommon.start;
				io->cnt = uncommon.end - uncommon.start + 1;
			}
			io->strip_off =
			    stripe->extent_span[bad_extent].strip_off *
			    vol->bsize;
			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_reconstruct_reader, io);

			if (stripe->extent_span[e].cnt == 0)
				continue;
		}

		range_t overlap_range;
		bool overlap_up = true;
		if (hr_ranges_overlap(&stripe->extent_span[e].range,
		    &stripe->extent_span[bad_extent].range,
		    &overlap_range)) {

			stripe->ps_to_be_added++;

			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			io->ba = overlap_range.start;
			io->cnt = overlap_range.end - overlap_range.start + 1;

			size_t diff = overlap_range.start -
			    stripe->extent_span[e].range.start;
			io->strip_off =
			    (stripe->extent_span[e].strip_off + diff) *
			    vol->bsize;

			io->data_read = stripe->extent_span[e].data_read;
			io->data_read += diff * vol->bsize;
			if (diff == 0)
				overlap_up = false;

			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_reader, io);
		}

		bool has_independent;
		range_t independent = { 0, 0 };
		has_independent = hr_stripe_range_non_extension(
		    &stripe->extent_span[e].range,
		    &uncommon,
		    &independent);
		if (has_independent) {
			hr_io_raid5_t *io =
			    hr_fgroup_alloc(stripe->worker_group);
			io->extent = e;
			io->ba = independent.start;
			io->cnt = independent.end - independent.start + 1;
			size_t diff = 0;
			if (!overlap_up) {
				diff =
				    overlap_range.end - overlap_range.start + 1;
			}
			io->strip_off =
			    (stripe->extent_span[e].strip_off + diff) *
			    vol->bsize;
			io->data_read = stripe->extent_span[e].data_read;
			io->data_read += diff * vol->bsize;
			io->vol = vol;
			io->stripe = stripe;

			hr_fgroup_submit(stripe->worker_group,
			    hr_io_raid5_basic_reader, io);
		}
	}

	stripe->ps_to_be_added++;

	hr_io_raid5_t *io = hr_fgroup_alloc(stripe->worker_group);
	io->extent = stripe->p_extent;
	io->ba = stripe->extent_span[bad_extent].range.start;
	io->cnt = stripe->extent_span[bad_extent].cnt;
	io->strip_off = stripe->extent_span[bad_extent].strip_off * vol->bsize;
	io->vol = vol;
	io->stripe = stripe;

	hr_fgroup_submit(stripe->worker_group, hr_io_raid5_reconstruct_reader,
	    io);

	stripe->p_count_final = true;
	fibril_condvar_broadcast(&stripe->ps_added_cv);

	hr_io_raid5_t *pcopier_io = hr_fgroup_alloc(stripe->worker_group);
	pcopier_io->cnt = stripe->extent_span[bad_extent].cnt;
	pcopier_io->strip_off =
	    stripe->extent_span[bad_extent].strip_off * vol->bsize;
	pcopier_io->data_read = stripe->extent_span[bad_extent].data_read;
	pcopier_io->vol = vol;
	pcopier_io->stripe = stripe;

	hr_fgroup_submit(stripe->worker_group, hr_io_raid5_parity_getter,
	    pcopier_io);
}

/** Get non-overlapping part of 2 ranges.
 *
 *  Return part of @param r1 not in @param r2.
 *
 *  @param r1 Main range.
 *  @param r2 Queried range.
 *  @param out Place to store resulting range.
 *
 *  @return true if output range is non-empty, else false.
 */
static bool hr_stripe_range_non_extension(const range_t *r1, const range_t *r2,
    range_t *out)
{
	if (r1->end < r2->start) {
		*out = *r1;
		return true;
	}

	if (r1->start > r2->end) {
		*out = *r1;
		return true;
	}

	if (r1->start < r2->start && r1->end >= r2->start) {
		out->start = r1->start;
		out->end = r2->start - 1;
		return out->start <= out->end;
	}

	if (r1->start <= r2->end && r1->end > r2->end) {
		out->start = r2->end + 1;
		out->end = r1->end;
		return out->start <= out->end;
	}

	return false;
}

/** Merge adjascent or overlapping extent spans.
 *
 *  @param s Stripe.
 *  @param extent_no Number of extents.
 *  @param out Place to store resulting ranges.
 *
 *  @return Number of resulting ranges.
 */
static size_t hr_stripe_merge_extent_spans(hr_stripe_t *s, size_t extent_no,
    range_t out[2])
{
	size_t out_count = 0;

	for (size_t i = 0; i < extent_no; i++) {
		if (s->extent_span[i].cnt == 0)
			continue;
		const range_t *r = &s->extent_span[i].range;
		bool merged = false;

		for (size_t j = 0; j < out_count; j++) {
			if (hr_ranges_overlap(&out[j], r, NULL)) {
				hr_stripe_extend_range(&out[j], r);
				merged = true;

				if (out_count == 2 &&
				    hr_ranges_overlap(&out[0], &out[1], NULL)) {
					hr_stripe_extend_range(&out[0], &out[1]);
					out_count = 1;
				}

				break;
			}
		}

		if (!merged) {
			assert(out_count < 2);
			out[out_count++] = *r;
		}
	}

	return out_count;
}

/** Extend a range.
 *
 *  @param r1 Output range.
 *  @param r2 Range to extend the output one with.
 *
 */
static void hr_stripe_extend_range(range_t *r1, const range_t *r2)
{
	if (r2->start < r1->start)
		r1->start = r2->start;
	if (r2->end > r1->end)
		r1->end = r2->end;
}

static bool hr_ranges_overlap(const range_t *a, const range_t *b, range_t *out)
{
	uint64_t start = a->start > b->start ? a->start : b->start;
	uint64_t end = a->end < b->end ? a->end : b->end;

	if (start <= end) {
		if (out != NULL) {
			out->start = start;
			out->end = end;
		}

		return true;
	}

	return false;
}

/** @}
 */
