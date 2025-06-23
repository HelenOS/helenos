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

#ifndef _HR_STRIPE_H
#define _HR_STRIPE_H

#include <fibril_synch.h>
#include <errno.h>
#include <hr.h>
#include <io/log.h>

#include "io.h"
#include "var.h"

typedef struct {
	uint64_t start;
	uint64_t end;
} range_t;

typedef struct hr_stripe {
	hr_volume_t *vol;
	bool write;
	bool subtract;
	size_t strips_touched;
	size_t partial_strips_touched;
	struct {
		range_t range;
		uint64_t cnt;
		uint64_t strip_off;
		const void *data_write;
		void *data_read;
	} *extent_span;
	uint64_t p_extent; /* parity extent index for this stripe */

	hr_fgroup_t *worker_group;

	errno_t rc;
	bool abort;
	bool done;

	fibril_mutex_t parity_lock;
	uint8_t *parity; /* the actual parity strip */
	uint64_t parity_size;

	/* parity writers waiting until this many parity commits */
	size_t ps_to_be_added;
	size_t ps_added; /* number of parities commited to stripe */
	fibril_condvar_t ps_added_cv;
	bool p_count_final;

	/*
	 * Possibly need 2 ranges because single IO that partially spans
	 * 2 strips and overflows to second one without creating an adjacent
	 * range results in parity not being continous.
	 *
	 * Example: 2+1 extents, 4 block strip, last extent parity
	 *
	 *  E0      E1     P
	 * +----+ +----+ +-----+
	 * |    | | IO | | IOP |
	 * |----| |----| |-----|
	 * |    | |    | |     |
	 * |----| |----| |-----|
	 * |    | |    | |     |
	 * |----| |----| |-----|
	 * | IO | |    | | IOP |
	 * +----+ +----+ +-----+
	 *
	 * - need 2 parity writers
	 */
	range_t total_height[2]; /* for knowing writing parity range(s) */
	size_t range_count;
} hr_stripe_t;

extern hr_stripe_t *hr_create_stripes(hr_volume_t *, uint64_t, size_t, bool);
extern void hr_destroy_stripes(hr_stripe_t *, size_t);
extern void hr_reset_stripe(hr_stripe_t *);
extern void hr_stripe_commit_parity(hr_stripe_t *, uint64_t, const void *,
    uint64_t);
extern void hr_stripe_wait_for_parity_commits(hr_stripe_t *);
extern void hr_stripe_parity_abort(hr_stripe_t *);
extern void execute_stripe(hr_stripe_t *, size_t);
extern void wait_for_stripe(hr_stripe_t *);

#endif

/** @}
 */
