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

/** @addtogroup hbench
 * @{
 */

#include <block.h>
#include <loc.h>
#include <str_error.h>
#include <stdio.h>
#include <stdlib.h>
#include "../hbench.h"

/** Execute disk random read benchmark. */
static bool runner(bench_env_t *env, bench_run_t *run, uint64_t size)
{
	const char *disk;
	const char *nbstr;
	service_id_t svcid;
	size_t block_size;
	aoff64_t dev_nblocks;
	aoff64_t baddr;
	bool block_inited = false;
	char *buf = NULL;
	uint64_t i;
	errno_t rc;
	int nitem;
	unsigned nb;

	disk = bench_env_param_get(env, "disk", NULL);
	if (disk == NULL) {
		bench_run_fail(run, "You must specify 'disk' parameter.");
		goto error;
	}

	nbstr = bench_env_param_get(env, "nb", "1");
	nitem = sscanf(nbstr, "%u", &nb);
	if (nitem < 1) {
		bench_run_fail(run, "'nb' must be an integer number of blocks.");
		goto error;
	}

	rc = loc_service_get_id(disk, &svcid, 0);
	if (rc != EOK) {
		bench_run_fail(run, "failed resolving device '%s'", disk);
		goto error;
	}

	rc = block_init(svcid);
	if (rc != EOK) {
		bench_run_fail(run, "failed opening block device '%s'",
		    disk);
	}

	block_inited = true;

	rc = block_get_bsize(svcid, &block_size);
	if (rc != EOK) {
		bench_run_fail(run, "error determining device block size.");
		goto error;
	}

	rc = block_get_nblocks(svcid, &dev_nblocks);
	if (rc != EOK) {
		bench_run_fail(run, "failed to obtain block device size.\n");
		goto error;
	}

	if (dev_nblocks < nb) {
		bench_run_fail(run, "device is smaller than %u blocks.\n",
		    nb);
		goto error;
	}

	buf = malloc(block_size * nb);
	if (buf == NULL) {
		bench_run_fail(run, "failed to allocate buffer (%zu bytes)",
		    block_size * nb);
		goto error;
	}

	bench_run_start(run);
	for (i = 0; i < size; i++) {
		/* Generate pseudo-random block address */
		baddr = (rand() + rand() * RAND_MAX) % (dev_nblocks - nb + 1);

		rc = block_read_direct(svcid, baddr, nb, buf);
		if (rc != EOK) {
			bench_run_fail(run, "failed to read blockd %llu-%llu: "
			    "%s", (unsigned long long)baddr,
			    (unsigned long long)(baddr + nb - 1),
			    str_error(errno));
			goto error;
		}
	}

	bench_run_stop(run);
	block_fini(svcid);
	free(buf);

	return true;
error:
	if (buf != NULL)
		free(buf);
	if (block_inited)
		block_fini(svcid);
	return false;
}

benchmark_t benchmark_rand_read = {
	.name = "rand_read",
	.desc = "Random disk read (must set 'disk' parameter).",
	.entry = &runner,
	.setup = NULL,
	.teardown = NULL
};

/**
 * @}
 */
