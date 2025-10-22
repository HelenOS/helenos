/*
 * Copyright (c) 2025 Miroslav Cimerman <mc@doas.su>
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

#ifndef _HR_IO_H
#define _HR_IO_H

#include "parity_stripe.h"
#include "var.h"
#include "util.h"

typedef struct hr_io {
	hr_bd_op_type_t type; /* read/write */
	uint64_t ba;
	uint64_t cnt;
	void *data_read;
	const void *data_write;
	size_t extent; /* extent index */
	hr_volume_t *vol; /* volume back-pointer */
} hr_io_t;

typedef struct hr_io_raid5 {
	uint64_t ba;
	uint64_t cnt;
	void *data_read;
	const void *data_write;
	size_t extent;
	uint64_t strip_off; /* needed for offseting parity commits */
	hr_stripe_t *stripe;
	hr_volume_t *vol;
} hr_io_raid5_t;

extern errno_t hr_write_direct(service_id_t, uint64_t, size_t, const void *);
extern errno_t hr_read_direct(service_id_t, uint64_t, size_t, void *);
extern errno_t hr_sync_cache(service_id_t, uint64_t, size_t);

extern errno_t hr_io_worker(void *);

extern errno_t hr_io_raid5_basic_reader(void *);
extern errno_t hr_io_raid5_reader(void *);
extern errno_t hr_io_raid5_basic_writer(void *);
extern errno_t hr_io_raid5_writer(void *);
extern errno_t hr_io_raid5_noop_writer(void *);
extern errno_t hr_io_raid5_parity_getter(void *);
extern errno_t hr_io_raid5_subtract_writer(void *);
extern errno_t hr_io_raid5_reconstruct_reader(void *);
extern errno_t hr_io_raid5_parity_writer(void *);

#endif

/** @}
 */
