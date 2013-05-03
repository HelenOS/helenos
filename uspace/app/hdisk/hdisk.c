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

#include <ipc/bd.h>
#include <loc.h>
#include <async.h>
#include <stdio.h>
#include <ipc/services.h>
#include <block.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <str.h>
#include <libmbr.h>
#include <libgpt.h>
#include <tinput.h>

#include "hdisk.h"
#include "input.h"
#include "func_gpt.h"
#include "func_mbr.h"
#include "func_none.h"

int interact(service_id_t dev_handle);
void print_help(void);
void select_table_format(tinput_t * in);
void fill_table_funcs(void);
void free_table(void);

static table_t table;

int main(int argc, char ** argv)
{
	if (argc == 1) {
		printf("I'd like to have an argument, please.\n");
		return 1;
	}

	int rc;
	service_id_t dev_handle;

	rc = loc_service_get_id(argv[1], &dev_handle, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("Unknown device. Exiting.\n");
		return -1;
	}

	init_table();

	mbr_t * mbr = mbr_read_mbr(dev_handle);
	if(mbr == NULL) {
		printf("Failed to read the Master Boot Record.\n"	\
			   "Either memory allocation or disk access failed. Exiting.\n");
		return -1;
	}

	if(mbr_is_mbr(mbr)) {
		table.layout = LYT_MBR;
		set_table_mbr(mbr);
		mbr_partitions_t * parts = mbr_read_partitions(mbr);
		if(parts == NULL) {
			printf("Failed to read and parse partitions.\n"	\
				   "Creating new partition table.");
			parts = mbr_alloc_partitions();
		}
		set_table_mbr_parts(parts);
		fill_table_funcs();
		goto interact;
	}
	
	
	mbr_free_mbr(mbr);
	gpt_t * gpt = gpt_read_gpt_header(dev_handle);
	
	if(gpt != NULL) {
		table.layout = LYT_GPT;
		set_table_gpt(gpt);
		
		gpt_partitions_t * parts = gpt_read_partitions(gpt);
		
		if(parts == NULL) {
			printf("Failed to read and parse partitions.\n"	\
				   "Creating new partition table.");
			parts = gpt_alloc_partitions();
		}
		set_table_gpt_parts(parts);
		fill_table_funcs();
		goto interact;
	}
	printf("No partition table recognized. Create a new one.\n");
	table.layout = LYT_NONE;
	
interact:
	rc = interact(dev_handle);
	
	free_table();
	
	return rc;
}

/** Interact with user */
int interact(service_id_t dev_handle)
{
	int input;
	tinput_t * in;
	
	in = tinput_new();
	if (in == NULL) {
		printf("Failed initing input. Free some memory.\n");
		return ENOMEM;
	}
	tinput_set_prompt(in, "");
	
	printf("Welcome to hdisk.\nType 'h' for help.\n");
	
	while (1) {
		printf("# ");
		input = getchar();
		printf("%c\n", input);
		
		switch(input) {
			case 'a':
				table.add_part(in, &table.data);
				break;
			case 'd':
				table.delete_part(in, &table.data);
				break;
			case 'e':
				table.extra_funcs(in, dev_handle, &table.data);
				break;
			case 'f':
				free_table();
				select_table_format(in);
				break;
			case 'h':
				print_help();
				break;
			case 'n':
				free_table();
				table.new_table(in, &table.data);
				break;
			case 'p':
				table.print_parts(&table.data);
				break;
			case 'q':
				putchar('\n');
				goto end;
			case 'w':
				table.write_parts(dev_handle, &table.data);
				break;
			default:
				printf("Unknown command. Try 'h' for help.\n");
				break;
		}
	}
	
end:
	tinput_destroy(in);
	
	return EOK;
}

void print_help(void)
{
	printf(
		"\t 'a' \t\t Add partition.\n"
		"\t 'd' \t\t Delete partition.\n"
		"\t 'e' \t\t Extra functions (per table format).\n"
		"\t 'f' \t\t Switch the format of the partition table."
		"\t 'h' \t\t Prints help. See help for more.\n"
		"\t 'n' \t\t Create new table (discarding the old one).\n"
		"\t 'p' \t\t Prints the table contents.\n"
		"\t 'w' \t\t Write table to disk.\n"
		"\t 'q' \t\t Quit.\n"
		);

}

void select_table_format(tinput_t * in)
{
	printf("Available formats are: \n"
			"1) MBR\n"
			"2) GPT\n"
		);
	
	uint8_t val = get_input_uint8(in);
	switch(val) {
		case 0:
			table.layout = LYT_NONE;
			fill_table_funcs();
			break;
		case 1:
			table.layout = LYT_MBR;
			fill_table_funcs();
			break;
		case 2:
			table.layout = LYT_GPT;
			fill_table_funcs();
			break;
	}
}

void fill_table_funcs(void)
{
	switch(table.layout) {
		case LYT_MBR:
			table.add_part = add_mbr_part;
			table.delete_part = delete_mbr_part;
			table.new_table = new_mbr_table;
			table.print_parts = print_mbr_parts;
			table.write_parts = write_mbr_parts;
			table.extra_funcs = extra_mbr_funcs;
			break;
		case LYT_GPT:
			table.add_part = add_gpt_part;
			table.delete_part = delete_gpt_part;
			table.new_table	= new_gpt_table;
			table.print_parts = print_gpt_parts;
			table.write_parts = write_gpt_parts;
			table.extra_funcs = extra_gpt_funcs;
			break;
		default:
			table.add_part = add_none_part;
			table.delete_part = delete_none_part;
			table.new_table = new_none_table;
			table.print_parts = print_none_parts;
			table.write_parts = write_none_parts;
			table.extra_funcs = extra_none_funcs;
			break;
	}
}

void free_table(void)
{
	switch(table.layout) {
		case LYT_MBR:
			if (table.data.mbr.parts != NULL) {
				mbr_free_partitions(table.data.mbr.parts);
				table.data.mbr.parts = NULL;
			}
			if (table.data.mbr.mbr != NULL) {
				mbr_free_mbr(table.data.mbr.mbr);
				table.data.mbr.mbr = NULL;
			}
			break;
		case LYT_GPT:
			if (table.data.gpt.parts != NULL) {
				gpt_free_partitions(table.data.gpt.parts);
				table.data.gpt.parts = NULL;
			}
			if (table.data.gpt.gpt != NULL) {
				gpt_free_gpt(table.data.gpt.gpt);
				table.data.gpt.gpt = NULL;
			}
			break;
		default:
			break;
	}
}



