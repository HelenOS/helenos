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

#include "func_mbr.h"

int add_mbr_part(tinput_t * in, mbr_parts_t * parts)
{
	part_t * part = mbr_alloc_partition();

	printf("Primary (p) or logical (l): ");
	int input = getchar();

	switch(input) {
		case 'p':
			mbr_set_flag(parts, ST_LOGIC, false);
		case 'l':
			mbr_set_flag(parts, ST_LOGIC, false);
		default:
			printf("Invalid type. Cancelled.");
			return EINVAL;
	}

	set_mbr_partition(in, part);

	mbr_add_partition(parts, part);
}

int delete_mbr_part(tinput_t * in, part_t * parts)
{
	char * str;
	int rc;
	part_t * temp = NULL;
	part_t * p = parts;
	uint32_t i, input = 0;
	char * endptr;

	printf("Number of the partition to delete (counted from 0): ");

	if((rc = get_input(in, &str)) != EOK)
		return rc;

	//convert from string to base 10 number
	if(str_uint32_t(str, &endptr, 10, true, &input) != EOK) {
		printf("Invalid value. Canceled.\n");
		return EINVAL;
	}

	free(str);

	rc = mbr_remove_partition(parts, input);
	if(rc != EOK) {
		printf("Error: something.\n");
	}

	return EOK;
}

/** Print current partition scheme */
void print_mbr_partitions(mbr_parts_t * parts)
{
	int num = 0;

	printf("Current partition scheme:\n");
	printf("\t\tBootable:\tStart:\tEnd:\tLength:\tType:\n");

	mbr_part_foreach(parts, it) {
		if (it->type == PT_UNUSED)
			continue;

		printf("\t P%d:\t", i);
		if (p->bootable)
			printf("*");
		else
			printf(" ");

		printf("\t\t%llu\t%llu\t%llu\t%d\n", p->start_addr, p->start_addr + p->length, p->length, p->type);

		++num;
	}*/

	printf("%d partitions found.\n", num);
}

void write_mbr_parts(mbr_parts_t * parts, mbr_t * mbr, service_id_t dev_handle)
{
	int rc = mbr_write_partitions(parts, mbr, dev_handle);
	if (rc != EOK) {
		printf("Error occured during writing. (ERR: %d)\n", rc);
	}
}
