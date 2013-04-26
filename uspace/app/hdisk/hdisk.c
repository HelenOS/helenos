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
#include "func_mbr.h"
#include "func_gpt.h"

int interact(service_id_t dev_handle);
void print_help(void);
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
	} else {
		table.layout = LYT_GPT;
		mbr_free_mbr(mbr);
		gpt_t * gpt = gpt_read_gpt_header(dev_handle);
		printf("here3\n");
		if(gpt == NULL) {
			printf("Failed to read and parse GPT header. Exiting.\n");
			return -1;
		}
		set_table_gpt(gpt);
		printf("here4\n");
		gpt_partitions_t * parts = gpt_read_partitions(gpt);
		printf("here5\n");
		if(parts == NULL) {
			printf("Failed to read and parse partitions.\n"	\
				   "Creating new partition table.");
			//parts = gpt_alloc_partitions();
		}
		set_table_gpt_parts(parts);
		fill_table_funcs();
	}

	rc = interact(dev_handle);
	
	free_table();
	
	return rc;
}

/*
int get_input(tinput_t * in, char ** str)
{
	int c;
	size_tat

	 pos = 0;
	size_t size = 256;

	*str = malloc(size * sizeof(char));
	if (*str == NULL)
		return ENOMEM;

	while ((c = getchar()) != '\n') {
		if (c >= 32 && c <= 126) {	//a printable character

			(*str)[pos] = c;
			++pos;
			putchar(c);

			if (pos == size) {
				char * temp = malloc(2 * size * sizeof(char));
				memcpy(temp, *str, size);
				free(*str);
				*str = temp;
				size *= 2;
			}
		} else if (c == 8) {		//backspace
			(*str)[pos] = 0;
			--pos;
			putchar(c);
		}
	}

	putchar('\n');

	(*str)[pos] = 0;

	return EOK;
}
*/


/** Interact with user */
int interact(service_id_t dev_handle)
{
	//int rc;
	int input;
	tinput_t * in;

	in = tinput_new();
	if (in == NULL) {
		printf("Failed initing input. Free some memory.\n");
		return ENOMEM;
	}
	tinput_set_prompt(in, "");

	printf("Welcome to hdisk.\nType 'h' for help.\n");

	//printf("# ");
	//input = getchar();
	//printf("%c\n", input);

	while (1) {

		printf("# ");
		input = getchar();
		printf("%c\n", input);


		//rc = tinput_read(in, &str);
		//if (rc == ENOENT) {
			//// User requested exit
			//putchar('\n');
			//return rc;
		//}
		//if (rc != EOK) {
			//printf("Failed reading input. Exiting...\n");
			//return rc;
		//}
		//// Check for empty input.
		//if (str_cmp(str, "") == 0)
			//continue;

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
			case 'h':
				print_help();
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
		//printf("# ");
		//input = getchar();
		//printf("%c\n", input);

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
		"\t 'h' \t\t Prints help. See help for more.\n" \
		"\t 'p' \t\t Prints the table contents.\n" \
		"\t 'w' \t\t Write table to disk.\n" \
		"\t 'q' \t\t Quit.\n" \
		);

}

void fill_table_funcs(void)
{
	switch(table.layout) {
		case LYT_MBR:
			table.add_part = add_mbr_part;
			table.delete_part = delete_mbr_part;
			table.print_parts = print_mbr_parts;
			table.write_parts = write_mbr_parts;
			table.extra_funcs = extra_mbr_funcs;
			break;
		case LYT_GPT:
			table.add_part = add_gpt_part;
			table.delete_part = delete_gpt_part;
			table.print_parts = print_gpt_parts;
			table.write_parts = write_gpt_parts;
			table.extra_funcs = extra_gpt_funcs;
			break;
		default:
			break;
	}
}

void free_table(void)
{
	switch(table.layout) {
		case LYT_MBR:
			mbr_free_partitions(table.data.mbr.parts);
			mbr_free_mbr(table.data.mbr.mbr);
			break;
		case LYT_GPT:
			gpt_free_partitions(table.data.gpt.parts);
			gpt_free_gpt(table.data.gpt.gpt);
			break;
		default:
			break;
	}
}

