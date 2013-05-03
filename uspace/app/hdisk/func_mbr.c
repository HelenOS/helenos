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

#include "func_mbr.h"
#include "input.h"

static int set_mbr_partition(tinput_t * in, mbr_part_t * p);


int add_mbr_part(tinput_t * in, union table_data * data)
{
	int rc;
	
	mbr_part_t * part = mbr_alloc_partition();

	set_mbr_partition(in, part);

	rc = mbr_add_partition(data->mbr.parts, part);
	if (rc != EOK) {
		printf("Error adding partition.\n");
	}
	
	
	return rc;
}

int delete_mbr_part(tinput_t * in, union table_data * data)
{
	int rc;
	size_t idx;

	printf("Number of the partition to delete (counted from 0): ");
	idx = get_input_size_t(in);
	
	if (idx == 0 && errno != EOK)
		return errno;
	
	rc = mbr_remove_partition(data->mbr.parts, idx);
	if(rc != EOK) {
		printf("Error: something.\n");
	}

	return EOK;
}

int new_mbr_table(tinput_t * in, union table_data * data)
{
	data->mbr.mbr = mbr_alloc_mbr();
	data->mbr.parts = mbr_alloc_partitions();
	return EOK;
}

/** Print current partition scheme */
int print_mbr_parts(union table_data * data)
{
	int num = 0;

	printf("Current partition scheme:\n");
	//printf("\t\tBootable:\tStart:\tEnd:\tLength:\tType:\n");
	printf("\t\t%10s  %10s %10s %10s %7s\n", "Bootable:", "Start:", "End:", "Length:", "Type:");
	
	mbr_part_t * it;
	mbr_part_foreach(data->mbr.parts, it) {
		if (it->type == PT_UNUSED)
			continue;

		printf("\tP%d:\t", num);
		if (mbr_get_flag(it, ST_BOOT))
			printf("*");
		else
			printf(" ");

		printf("\t%10u %10u %10u %7u\n", it->start_addr, it->start_addr + it->length, it->length, it->type);

		++num;
	}

	printf("%d partitions found.\n", num);
	
	return EOK;
}

int write_mbr_parts(service_id_t dev_handle, union table_data * data)
{
	int rc = mbr_write_partitions(data->mbr.parts, data->mbr.mbr, dev_handle);
	if (rc != EOK) {
		printf("Error occured during writing: ERR: %d: %s\n", rc, str_error(rc));
	}
	
	return rc;
}

int extra_mbr_funcs(tinput_t * in, service_id_t dev_handle, union table_data * data)
{
	printf("Not implemented.\n");
	return EOK;
}

static int set_mbr_partition(tinput_t * in, mbr_part_t * p)
{
	int c;
	uint8_t type;
	
	printf("Primary (p) or logical (l): ");
	c = getchar();
	printf("%c\n", c);

	switch(c) {
		case 'p':
			mbr_set_flag(p, ST_LOGIC, false);
			break;
		case 'l':
			mbr_set_flag(p, ST_LOGIC, true);
			break;
		default:
			printf("Invalid type. Cancelled.");
			return EINVAL;
	}
	
	printf("Set type (0-255): ");
	type = get_input_uint8(in);
	if (type == 0 && errno != EOK)
		return errno;

	///TODO: there can only be one bootable partition; let's do it just like fdisk
	printf("Bootable? (y/n): ");
	c = getchar();
	if (c != 'y' && c != 'Y' && c != 'n' && c != 'N') {
		printf("Invalid value. Cancelled.");
		return EINVAL;
	}
	printf("%c\n", c);
	mbr_set_flag(p, ST_BOOT, (c == 'y' || c == 'Y') ? true : false);


	uint32_t sa, ea;

	printf("Set starting address (number): ");
	sa = get_input_uint32(in);
	if (sa == 0 && errno != EOK)
		return errno;

	printf("Set end addres (number): ");
	ea = get_input_uint32(in);
	if (ea == 0 && errno != EOK)
		return errno;
	
	if(ea < sa) {
		printf("Invalid value. Canceled.\n");
		return EINVAL;
	}

	p->type = type;
	p->start_addr = sa;
	p->length = ea - sa;

	return EOK;
}






