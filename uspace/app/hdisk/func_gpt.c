/*
 * Copyright (c) 2012, 2013 Dominik Taborsky
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

 /** @addtogroup hdisk
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <sys/types.h>

#include "func_gpt.h"
#include "input.h"

static int set_gpt_partition(tinput_t *, gpt_part_t *);

int add_gpt_part(tinput_t * in, union table_data * data)
{
	gpt_part_t * p = gpt_alloc_partition(data->gpt.parts);
	if (p == NULL) {
		return ENOMEM;
	}

	return set_gpt_partition(in, p);
}

int delete_gpt_part(tinput_t * in, union table_data * data)
{
	size_t idx;

	printf("Number of the partition to delete (counted from 0): ");
	idx = get_input_size_t(in);

	if (gpt_remove_partition(data->gpt.parts, idx) == -1) {
		printf("Warning: running low on memory, not resizing...\n");
	}

	return EOK;
}

int new_gpt_table(tinput_t * in, union table_data * data)
{
	data->gpt.gpt = gpt_alloc_gpt_header();
	data->gpt.parts = gpt_alloc_partitions();
	return EOK;
}

int print_gpt_parts(union table_data * data)
{
	//int rc;
	printf("Current partition scheme (GPT):\n");
	printf("\t\tStart:\tEnd:\tLength:\tType:\tName:\n");
	
	size_t i = 0;
	
	gpt_part_foreach(data->gpt.parts, iter) {
		//printf("\t%10u %10u %10u %3d\n", iter->start_addr, iter->start_addr + iter->length,
		//		iter->length, gpt_get_part_type(iter), gpt_get_part_name(iter));
		printf("%3u\t%10llu %10llu %10llu %3d %s\n", i, gpt_get_start_lba(iter), gpt_get_end_lba(iter),
				gpt_get_end_lba(iter) - gpt_get_start_lba(iter), gpt_get_part_type(iter),
				gpt_get_part_name(iter));
		i++;
	}

	//return rc;
	return EOK;
}

int write_gpt_parts(service_id_t dev_handle, union table_data * data)
{
	int rc;

	rc = gpt_write_partitions(data->gpt.parts, data->gpt.gpt, dev_handle);
	if (rc != EOK) {
		printf("Error: Writing partitions failed: %d (%s)\n", rc, str_error(rc));
		return rc;
	}

	rc = gpt_write_gpt_header(data->gpt.gpt, dev_handle);
	if (rc != EOK) {
		printf("Error: Writing partitions failed: %d (%s)\n", rc, str_error(rc));
		return rc;
	}

	return EOK;
}

int extra_gpt_funcs(tinput_t * in, service_id_t dev_handle, union table_data * data)
{
	printf("Not implemented.\n");
	return EOK;
}

static int set_gpt_partition(tinput_t * in, gpt_part_t * p)
{
	//int rc;

	uint64_t sa, ea;

	printf("Set starting address (number): ");
	sa = get_input_uint64(in);

	printf("Set end addres (number): ");
	ea = get_input_uint64(in);

	if (ea <= sa) {
		printf("Invalid value.\n");
		return EINVAL;
	}


	//p->start_addr = sa;
	gpt_set_start_lba(p, sa);
	//p->length = ea - sa;
	gpt_set_end_lba(p, ea);

	return EOK;
}

