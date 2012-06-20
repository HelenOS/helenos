/*
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup bd
 * @{
 */

/**
 * @file
 * @brief File-backed block device driver
 *
 * Allows accessing a file as a block device. Useful for, e.g., mounting
 * a disk image.
 */

#include <stdio.h>
#include <unistd.h>
#include <ipc/bd.h>
#include <async.h>
#include <as.h>
#include <fibril_synch.h>
#include <loc.h>
#include <sys/types.h>
#include <sys/typefmt.h>
#include <errno.h>
#include <bool.h>
#include <task.h>
#include <macros.h>

#define NAME "file_bd"

#define DEFAULT_BLOCK_SIZE 512

static size_t block_size;
static aoff64_t num_blocks;
static FILE *img;

static service_id_t service_id;
static fibril_mutex_t dev_lock;

static void print_usage(void);
static int file_bd_init(const char *fname);
static void file_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *);
static int file_bd_read_blocks(uint64_t ba, size_t cnt, void *buf);
static int file_bd_write_blocks(uint64_t ba, size_t cnt, const void *buf);

int main(int argc, char **argv)
{
	int rc;
	char *image_name;
	char *device_name;

	printf(NAME ": File-backed block device driver\n");

	block_size = DEFAULT_BLOCK_SIZE;

	++argv; --argc;
	while (*argv != NULL && (*argv)[0] == '-') {
		/* Option */
		if (str_cmp(*argv, "-b") == 0) {
			if (argc < 2) {
				printf("Argument missing.\n");
				print_usage();
				return -1;
			}

			rc = str_size_t(argv[1], NULL, 10, true, &block_size);
			if (rc != EOK || block_size == 0) {
				printf("Invalid block size '%s'.\n", argv[1]);
				print_usage();
				return -1;
			}
			++argv; --argc;
		} else {
			printf("Invalid option '%s'.\n", *argv);
			print_usage();
			return -1;
		}
		++argv; --argc;
	}

	if (argc < 2) {
		printf("Missing arguments.\n");
		print_usage();
		return -1;
	}

	image_name = argv[0];
	device_name = argv[1];

	if (file_bd_init(image_name) != EOK)
		return -1;

	rc = loc_service_register(device_name, &service_id);
	if (rc != EOK) {
		printf("%s: Unable to register device '%s'.\n",
		    NAME, device_name);
		return rc;
	}
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Not reached */
	return 0;
}

static void print_usage(void)
{
	printf("Usage: " NAME " [-b <block_size>] <image_file> <device_name>\n");
}

static int file_bd_init(const char *fname)
{
	async_set_client_connection(file_bd_connection);
	int rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register driver.\n", NAME);
		return rc;
	}
	
	img = fopen(fname, "rb+");
	if (img == NULL)
		return EINVAL;
	
	if (fseek(img, 0, SEEK_END) != 0) {
		fclose(img);
		return EIO;
	}
	
	off64_t img_size = ftell(img);
	if (img_size < 0) {
		fclose(img);
		return EIO;
	}
	
	num_blocks = img_size / block_size;
	
	fibril_mutex_initialize(&dev_lock);
	
	return EOK;
}

static void file_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	size_t comm_size;
	unsigned int flags;
	int retval;
	uint64_t ba;
	size_t cnt;

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	if (!async_share_out_receive(&callid, &comm_size, &flags)) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	(void) async_share_out_finalize(callid, &fs_va);
	if (fs_va == AS_MAP_FAILED) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	while (true) {
		callid = async_get_call(&call);
		method = IPC_GET_IMETHOD(call);
		
		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (method) {
		case BD_READ_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = file_bd_read_blocks(ba, cnt, fs_va);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = file_bd_write_blocks(ba, cnt, fs_va);
			break;
		case BD_GET_BLOCK_SIZE:
			async_answer_1(callid, EOK, block_size);
			continue;
		case BD_GET_NUM_BLOCKS:
			async_answer_2(callid, EOK, LOWER32(num_blocks),
			    UPPER32(num_blocks));
			continue;
		default:
			retval = EINVAL;
			break;
		}
		async_answer_0(callid, retval);
	}
}

/** Read blocks from the device. */
static int file_bd_read_blocks(uint64_t ba, size_t cnt, void *buf)
{
	size_t n_rd;
	int rc;

	/* Check whether access is within device address bounds. */
	if (ba + cnt > num_blocks) {
		printf(NAME ": Accessed blocks %" PRIuOFF64 "-%" PRIuOFF64 ", while "
		    "max block number is %" PRIuOFF64 ".\n", ba, ba + cnt - 1,
		    num_blocks - 1);
		return ELIMIT;
	}

	fibril_mutex_lock(&dev_lock);

	clearerr(img);
	rc = fseek(img, ba * block_size, SEEK_SET);
	if (rc < 0) {
		fibril_mutex_unlock(&dev_lock);
		return EIO;
	}

	n_rd = fread(buf, block_size, cnt, img);

	if (ferror(img)) {
		fibril_mutex_unlock(&dev_lock);
		return EIO;	/* Read error */
	}

	fibril_mutex_unlock(&dev_lock);

	if (n_rd < cnt)
		return EINVAL;	/* Read beyond end of device */

	return EOK;
}

/** Write blocks to the device. */
static int file_bd_write_blocks(uint64_t ba, size_t cnt, const void *buf)
{
	size_t n_wr;
	int rc;

	/* Check whether access is within device address bounds. */
	if (ba + cnt > num_blocks) {
		printf(NAME ": Accessed blocks %" PRIuOFF64 "-%" PRIuOFF64 ", while "
		    "max block number is %" PRIuOFF64 ".\n", ba, ba + cnt - 1,
		    num_blocks - 1);
		return ELIMIT;
	}

	fibril_mutex_lock(&dev_lock);

	clearerr(img);
	rc = fseek(img, ba * block_size, SEEK_SET);
	if (rc < 0) {
		fibril_mutex_unlock(&dev_lock);
		return EIO;
	}

	n_wr = fwrite(buf, block_size, cnt, img);

	if (ferror(img) || n_wr < cnt) {
		fibril_mutex_unlock(&dev_lock);
		return EIO;	/* Write error */
	}

	if (fflush(img) != 0) {
		fibril_mutex_unlock(&dev_lock);
		return EIO;
	}

	fibril_mutex_unlock(&dev_lock);

	return EOK;
}

/**
 * @}
 */
