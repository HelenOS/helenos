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
#include <libblock.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <str.h>
#include <libmbr.h>
#include <tinput.h>

#include "func_mbr.h"
#include "func_gpt.h"

enum TABLES {
	TBL_MBR,
	TBL_GPT;
};

static TABLES table;

int get_input(tinput_t * in, char ** str);
void interact(mbr_parts_t * partitions, mbr_t * mbr, service_id_t dev_handle);
void print_help(void);


int set_primary(tinput_t * in, mbr_parts_t * parts);
int add_logical(tinput_t * in, mbr_parts_t * parts);
int set_mbr_partition(tinput_t * in, mbr_parts_t * p);

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

	mbr_parts_t * parts = NULL;
	gpt_header_t * g_hdr = NULL;
	gpt_parts_t * g_parts = NULL;

	mbr_t * mbr = mbr_read_mbr(dev_handle);
	if(mbr == NULL) {
		printf("Failed to read the Master Boot Record.\n"	\
			   "Either memory allocation or disk access failed.\n");
		return -1;
	}

	if(mbr_is_mbr(mbr)) {
		table = TBL_MBR;
		parts = mbr_read_partitions(mbr);
		if(parts == NULL) {
			printf("Failed to read and parse partitions.\n"	\
				   "Creating new partition table.")
		}
	} else {
		table = TBL_GPT;
		//g_hdr = gpt_read_header();
		//g_parts = gpt_read_partitions();
	}


	rc = interact(partitions, mbr, dev_handle);

	if(gpt) {
		gpt_free_partitions();
		gpt_free_header();
	} else {
		mbr_free_partitions(partitions);
		mbr_free_mbr(mbr);
	}

	return rc;
}

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

//int get_input(tinput_t * in, char ** str)
//{
//	int rc;
//	printf("help1\n");
//	rc = tinput_read(in, str);
//	if (rc == ENOENT) {
//		/* User requested exit */
//		return EINTR;
//	}
//	printf("help2\n");
//	fflush(stdout);
//	if (rc != EOK) {
//		printf("Failed reading input. Cancelling...\n");
//		return rc;
//	}
//	printf("help3\n");
//	fflush(stdout);
//	/* Check for empty input. */
//	if (str_cmp(*str, "") == 0) {
//		printf("Canceled.\n");
//		return EINVAL;
//	}
//	printf("help4\n");
//	fflush(stdout);
//
//	return EOK;
//}

/** Interact with user */
int interact_mbr(mbr_parts_t * partitions, mbr_t * mbr, service_id_t dev_handle)
{
	char * str;
	//int rc;
	tinput_t * in;

	in = tinput_new();
	if (in == NULL) {
		printf("Failed initing input. Free some memory.\n");
		return;
	}

	tinput_set_prompt(in, " ");

	printf("Welcome to hdisk.\nType 'h' for help.\n");

	//printf("# ");
	//int input = getchar();
	//printf("%c\n", input);

	while (1) {

		/*printf("# ");
		int input = getchar();
		printf("%c\n", input);
		*/

		rc = tinput_read(in, &str);
		if (rc == ENOENT) {
			// User requested exit
			putchar('\n');
			return rc;
		}
		if (rc != EOK) {
			printf("Failed reading input. Exiting...\n");
			return rc;
		}
		// Check for empty input.
		if (str_cmp(str, "") == 0)
			continue;

		switch(*str) {
			case 'a':
				add_part(in, partitions);
				break;
			case 'd':
				delete_part(in, partitions);
				break;
			case 'h':
				print_help();
				break;
			case 'p':
				print_MBR(partitions);
				break;
			case 'q':
				putchar('\n');
				return;
			case 'w':
				write_parts(partitions, mbr, dev_handle);
				break;
			default:
				printf("Unknown command. Try 'h' for help.\n");
				break;
		}
		//printf("# ");
		//input = getchar();
		//printf("%c\n", input);

	}

	tinput_destroy(in);

	return;
}

void print_help(void)
{
	printf(
		"\t 'a' \t\t Add partition.\n"
		"\t 'd' \t\t Delete partition.\n"
		"\t 'h' \t\t Prints help. See help for more.\n" \
		"\t 'p' \t\t Prints the MBR contents.\n" \
		"\t 'q' \t\t Quit.\n" \
		);

}

//other functions
//FIXME: need error checking after input
int set_primary(tinput_t * in, part_t * parts)
{
	char * str;
	int rc;
	uint32_t input = 5;
	part_t * p = parts;
	//char c_input[2];
	char * endptr;

//	while (input > 3) {
//		printf("Number of the primary partition to set (0-3): ");
//		str_uint64(fgets(c_input, 1, stdin), &endptr, 10, true, &input);
//		printf("%llu\n", input);
//	}

	if ((rc = get_input(in, &str)) != EOK)
		return rc;

	if (str_uint32_t(str, &endptr, 10, true, &input) != EOK || input > 3) {
		printf("Invalid value. Canceled.\n");
		return EINVAL;
	}

	free(str);

	for (; input > 0; --input) {
		p = p->next;
	}

	set_partition(in, p);

	return EOK;
}

int add_logical(tinput_t * in, part_t * parts)
{
	int i;
	part_t * p = parts;
	part_t * ext = NULL;

	//checking primary partitions for extended partition
	for (i = 0; i < N_PRIMARY; ++i) {
		if (p->type == PT_EXTENDED) {
			ext = p;
			break;
		}
		p = p->next;
	}

	if (ext == NULL) {
		printf("Error: no extended partition.\n");
		return EINVAL;
	}

	while (p->next != NULL) {
		p = p->next;
	}

	p->next = malloc(sizeof(part_t));
	if (p->next == NULL) {
		printf("Error: not enough memory.\n");
		return ENOMEM;
	}

	p->next->ebr = malloc(sizeof(br_block_t));
	if (p->next->ebr == NULL) {
		printf("Error: not enough memory.\n");
		return ENOMEM;
	}

	set_partition(in, p->next);

	return EOK;
}

int set_mbr_partition(tinput_t * in, part_t * p)
{
	char * str;
	int rc;
	uint8_t type;
	char * endptr;

//	while (type > 255) {
//		printf("Set type (0-255): ");
//		str_uint64(fgets(c_type, 3, stdin), &endptr, 10, true, &type);
//	}

	if ((rc = get_input(in, &str)) != EOK)
		return rc;

	if (str_uint8_t(str, &endptr, 10, true, &type) != EOK) {
		printf("Invalid value. Canceled.\n");
		return EINVAL;
	}

	free(str);

	///TODO: there can only be one bootable partition; let's do it just like fdisk
	printf("Bootable? (y/n): ");
	int c = getchar();
	printf("%c\n", c);
	if (c != 'y' && c != 'Y' && c != 'n' && c != 'N') {
		printf("Invalid value. Cancelled.");
		return EINVAL;
	}

	uint32_t sa, ea;

	printf("Set starting address (number): ");
	//str_uint64(fgets(c_sa, 21, stdin), &endptr, 10, true, &sa);

	if ((rc = get_input(in, &str)) != EOK)
		return rc;

	if(str_uint32_t(str, &endptr, 10, true, &sa) != EOK) {
		printf("Invalid value. Canceled.\n");
		return EINVAL;
	}

	free(str);

	printf("Set end addres (number): ");
	//str_uint64(fgets(c_len, 21, stdin), &endptr, 10, true, &len);

	if ((rc = get_input(in, &str)) != EOK)
		return rc;

	if(str_uint32_t(str, &endptr, 10, true, &ea) != EOK || ea < sa) {
		printf("Invalid value. Canceled.\n");
		return EINVAL;
	}

	free(str);

	p->type = type;
	p->bootable = (c == 'y' || c == 'Y') ? true : false;
	p->start_addr = sa;
	p->length = ea - sa;

	return EOK;
}


