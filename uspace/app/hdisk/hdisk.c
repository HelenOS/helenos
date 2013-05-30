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

int interact(service_id_t);
void print_help(void);
void select_label_format(tinput_t *);
void fill_label_funcs(void);
void free_label(void);
int try_read(service_id_t);

int construct_none_label(void);

int construct_mbr_label(void);
int try_read_mbr(service_id_t);

int construct_gpt_label(void);
int try_read_gpt(service_id_t);


static label_t label;

int main(int argc, char ** argv)
{
	if (argc == 1) {
		printf("Missing argument. Please specify a device to operate on.\n");
		return -1;
	}
	
	int rc;
	service_id_t dev_handle;
	
	rc = loc_service_get_id(argv[1], &dev_handle, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("Unknown device. Exiting.\n");
		return -1;
	}
	
	printf("Init.\n");
	init_label();
	
	/*
	mbr_t * mbr = mbr_read_mbr(dev_handle);
	if(mbr == NULL) {
		printf("Failed to read the Master Boot Record.\n"	\
			   "Either memory allocation or disk access failed. Exiting.\n");
		return -1;
	}

	if(mbr_is_mbr(mbr)) {
		label.layout = LYT_MBR;
		set_label_mbr(mbr);
		mbr_partitions_t * parts = mbr_read_partitions(mbr);
		if(parts == NULL) {
			printf("Failed to read and parse partitions.\n"	\
				   "Creating new partition table.");
			parts = mbr_alloc_partitions();
		}
		set_label_mbr_parts(parts);
		fill_label_funcs();
		goto interact;
	}
	
	
	mbr_free_mbr(mbr);*/
	
	printf("Try MBR.\n");
	rc = try_read_mbr(dev_handle);
	if (rc == EOK)
		goto interact;
	
	/*
	gpt_t * gpt = gpt_read_gpt_header(dev_handle);
	
	if(gpt != NULL) {
		label.layout = LYT_GPT;
		set_label_gpt(gpt);
		
		gpt_partitions_t * parts = gpt_read_partitions(gpt);
		
		if(parts == NULL) {
			printf("Failed to read and parse partitions.\n"	\
				   "Creating new partition table.");
			parts = gpt_alloc_partitions();
		}
		set_label_gpt_parts(parts);
		fill_label_funcs();
		goto interact;
	}
	*/
	
	printf("Try GPT.\n");
	rc = try_read_gpt(dev_handle);
	if (rc == EOK)
		goto interact;
	
	printf("No label recognized. Create a new one.\n");
	label.layout = LYT_NONE;
	
interact:
	printf("interact.\n");
	rc = interact(dev_handle);
	
	free_label();
	
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
				label.add_part(in, &label.data);
				break;
			case 'b':
				label.add_part(in, &label.data);
				break;
			case 'd':
				label.delete_part(in, &label.data);
				break;
			case 'e':
				label.extra_funcs(in, dev_handle, &label.data);
				break;
			case 'f':
				free_label();
				select_label_format(in);
				break;
			case 'h':
				print_help();
				break;
			case 'n':
				free_label();
				label.new_label(&label.data);
				break;
			case 'p':
				label.print_parts(&label.data);
				break;
			case 'q':
				putchar('\n');
				goto end;
			case 'w':
				label.write_parts(dev_handle, &label.data);
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
		"\t 'e' \t\t Extra functions (per label format).\n"
		"\t 'f' \t\t Switch the format of the partition label."
		"\t 'h' \t\t Prints help. See help for more.\n"
		"\t 'l' \t\t Set alignment.\n"
		"\t 'n' \t\t Create new label (discarding the old one).\n"
		"\t 'p' \t\t Prints label contents.\n"
		"\t 'w' \t\t Write label to disk.\n"
		"\t 'q' \t\t Quit.\n"
		);

}

void select_label_format(tinput_t * in)
{
	printf("Available formats are: \n"
			"1) MBR\n"
			"2) GPT\n"
		);
	
	uint8_t val = get_input_uint8(in);
	switch(val) {
		case 0:
			free_label();
			label.layout = LYT_NONE;
			fill_label_funcs();
			break;
		case 1:
			free_label();
			label.layout = LYT_MBR;
			fill_label_funcs();
			break;
		case 2:
			free_label();
			label.layout = LYT_GPT;
			fill_label_funcs();
			break;
	}
}

void fill_label_funcs(void)
{
	switch(label.layout) {
		case LYT_MBR:
			construct_mbr_label();
			break;
		case LYT_GPT:
			construct_gpt_label();
			break;
		default:
			construct_none_label();
			break;
	}
}

void free_label(void)
{
	/*
	switch(label.layout) {
		case LYT_MBR:
			destroy_mbr_label(&label);
			break;
		case LYT_GPT:
			destroy_gpt_label(&label);
			break;
		default:
			break;
	}
	*/
	
	label.destroy_label(&label.data);
}

int try_read(service_id_t dev_handle)
{
	fill_label_funcs();
	printf("read_parts\n");
	return label.read_parts(dev_handle, &label.data);
}

int construct_none_label()
{
	label.add_part      = add_none_part;
	label.delete_part   = delete_none_part;
	label.destroy_label = destroy_none_label;
	label.new_label     = new_none_label;
	label.print_parts   = print_none_parts;
	label.read_parts    = read_none_parts;
	label.write_parts   = write_none_parts;
	label.extra_funcs   = extra_none_funcs;
	
	return EOK;
}

int construct_mbr_label()
{
	label.add_part      = add_mbr_part;
	label.delete_part   = delete_mbr_part;
	label.destroy_label = destroy_mbr_label;
	label.new_label     = new_mbr_label;
	label.print_parts   = print_mbr_parts;
	label.read_parts    = read_mbr_parts;
	label.write_parts   = write_mbr_parts;
	label.extra_funcs   = extra_mbr_funcs;
	
	return label.new_label(&label.data);
}

int try_read_mbr(service_id_t dev_handle)
{
	label.layout = LYT_MBR;
	return try_read(dev_handle);
}

int construct_gpt_label()
{
	label.add_part    = add_gpt_part;
	label.delete_part = delete_gpt_part;
	label.new_label   = new_gpt_label;
	label.print_parts = print_gpt_parts;
	label.read_parts  = read_gpt_parts;
	label.write_parts = write_gpt_parts;
	label.extra_funcs = extra_gpt_funcs;
	
	return label.new_label(&label.data);
}

int try_read_gpt(service_id_t dev_handle)
{
	label.layout = LYT_GPT;
	return try_read(dev_handle);
}


























