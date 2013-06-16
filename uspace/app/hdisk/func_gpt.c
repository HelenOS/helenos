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


int construct_gpt_label(label_t *this)
{
	this->layout = LYT_GPT;
	this->alignment = 1;
	
	this->add_part    = add_gpt_part;
	this->delete_part = delete_gpt_part;
	this->new_label   = new_gpt_label;
	this->print_parts = print_gpt_parts;
	this->read_parts  = read_gpt_parts;
	this->write_parts = write_gpt_parts;
	this->extra_funcs = extra_gpt_funcs;
	
	return this->new_label(this);
}

int add_gpt_part(label_t *this, tinput_t * in)
{
	gpt_part_t * p = gpt_alloc_partition(this->data.gpt->parts);
	if (p == NULL) {
		return ENOMEM;
	}

	return set_gpt_partition(in, p);
}

int delete_gpt_part(label_t *this, tinput_t * in)
{
	size_t idx;

	printf("Number of the partition to delete (counted from 0): ");
	idx = get_input_size_t(in);

	if (gpt_remove_partition(this->data.gpt->parts, idx) == -1) {
		printf("Warning: running low on memory, not resizing...\n");
	}

	return EOK;
}

int destroy_gpt_label(label_t *this)
{
	return EOK;
}

int new_gpt_label(label_t *this)
{
	this->data.gpt->gpt = gpt_alloc_gpt_header();
	this->data.gpt->parts = gpt_alloc_partitions();
	return EOK;
}

int print_gpt_parts(label_t *this)
{
	//int rc;
	printf("Current partition scheme (GPT):\n");
	printf("\t\tStart:\tEnd:\tLength:\tType:\tName:\n");
	
	size_t i = 0;
	
	gpt_part_foreach(this->data.gpt->parts, iter) {
		//FIXMEE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if (gpt_get_part_type(iter) == 62)
			continue;
		
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

int read_gpt_parts(label_t *this, service_id_t dev_handle)
{
	return EOK;
}

int write_gpt_parts(label_t *this, service_id_t dev_handle)
{
	int rc;

	rc = gpt_write_partitions(this->data.gpt->parts, this->data.gpt->gpt, dev_handle);
	if (rc != EOK) {
		printf("Error: Writing partitions failed: %d (%s)\n", rc, str_error(rc));
		return rc;
	}

	rc = gpt_write_gpt_header(this->data.gpt->gpt, dev_handle);
	if (rc != EOK) {
		printf("Error: Writing partitions failed: %d (%s)\n", rc, str_error(rc));
		return rc;
	}

	return EOK;
}

int extra_gpt_funcs(label_t *this, tinput_t * in, service_id_t dev_handle)
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

