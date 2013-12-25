/*
 * Copyright (c) 2012-2013 Dominik Taborsky
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
#include <str_error.h>
#include "hdisk.h"
#include "input.h"
#include "func_gpt.h"
#include "func_mbr.h"
#include "func_none.h"

static int interact(void);
static void print_help(void);
static void select_label_format(tinput_t *);
static void construct_label(layouts_t);
static void free_label(void);
static int try_read(void);
static int try_read_mbr(void);
static int try_read_gpt(void);
static void set_alignment(tinput_t *);

static label_t label;

int main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("Missing argument. Please specify a device to operate on.\n");
		return 1;
	}
	
	service_id_t dev_handle;
	int rc = loc_service_get_id(argv[1], &dev_handle, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("Unknown device. Exiting.\n");
		return 2;
	}
	
	init_label();
	label.device = dev_handle;
	
	rc = block_init(EXCHANGE_ATOMIC, dev_handle, 512);
	if (rc != EOK) {
		printf("Error during libblock init: %d - %s.\n", rc, str_error(rc));
		return 3;
	}
	
	aoff64_t blocks;
	rc = block_get_nblocks(dev_handle, &blocks);
	block_fini(dev_handle);
	if (rc != EOK) {
		printf("Error while getting number of blocks: %d - %s.\n",
		    rc, str_error(rc));
		return 4;
	}
	
	label.blocks = blocks;
	
	rc = try_read_mbr();
	if (rc == EOK)
		goto interact;
	
	free_label();
	
	rc = try_read_gpt();
	if (rc == EOK)
		goto interact;
	
	printf("No label recognized. Create a new one.\n");
	construct_label(LYT_NONE);
	
interact:
	return interact();
}

/** Interact with user */
int interact(void)
{
	tinput_t *in = tinput_new();
	if (in == NULL) {
		printf("Failed initing input. Free some memory.\n");
		return ENOMEM;
	}
	tinput_set_prompt(in, "");
	
	printf("Welcome to hdisk.\nType 'h' for help.\n");
	
	while (true) {
		printf("# ");
		int input = getchar();
		printf("%c\n", input);
		
		switch (input) {
		case 'a':
			label.add_part(&label, in);
			break;
		case 'd':
			label.delete_part(&label, in);
			break;
		case 'e':
			label.extra_funcs(&label, in);
			break;
		case 'f':
			free_label();
			select_label_format(in);
			break;
		case 'h':
			print_help();
			break;
		case 'l':
			set_alignment(in);
			break;
		case 'n':
			printf("Discarding label...\n");
			free_label();
			label.new_label(&label);
			break;
		case 'p':
			label.print_parts(&label);
			break;
		case 'q':
			putchar('\n');
			free_label();
			goto end;
		case 'r':
			label.read_parts(&label);
		case 'w':
			label.write_parts(&label);
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
	    "\t 'f' \t\t Switch the format of the partition label.\n"
	    "\t 'h' \t\t Prints help. See help for more.\n"
	    "\t 'l' \t\t Set alignment.\n"
	    "\t 'n' \t\t Create new label (discarding the old one).\n"
	    "\t 'p' \t\t Prints label contents.\n"
	    "\t 'q' \t\t Quit.\n"
	    "\t 'r' \t\t Read label from disk.\n"
	    "\t 'w' \t\t Write label to disk.\n");
}

void select_label_format(tinput_t *in)
{
	printf("Available formats are: \n"
	    "1) MBR\n"
	    "2) GPT\n");
	
	uint8_t val = get_input_uint8(in);
	switch (val) {
	case 1:
		construct_label(LYT_MBR);
		break;
	case 2:
		construct_label(LYT_GPT);
		break;
	default:
		construct_label(LYT_NONE);
		break;
	}
}

void construct_label(layouts_t layout)
{
	switch (layout) {
	case LYT_MBR:
		label.layout = LYT_MBR;
		construct_mbr_label(&label);
		break;
	case LYT_GPT:
		label.layout = LYT_GPT;
		construct_gpt_label(&label);
		break;
	default:
		label.layout = LYT_NONE;
		construct_none_label(&label);
		break;
	}
}

void free_label(void)
{
	label.destroy_label(&label);
}

int try_read(void)
{
	return label.read_parts(&label);
}

int try_read_mbr(void)
{
	construct_label(LYT_MBR);
	return try_read();
}

int try_read_gpt(void)
{
	construct_label(LYT_GPT);
	return try_read();
}

void set_alignment(tinput_t *in)
{
	printf("Set alignment to sectors: ");
	label.alignment = get_input_uint32(in);
	printf("Alignment set to %u sectors.\n", label.alignment);
}
