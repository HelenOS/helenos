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
#include <async.h>
#include <as.h>
#include <bd_srv.h>
#include <fibril_synch.h>
#include <loc.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <str_error.h>
#include <stdbool.h>
#include <task.h>
#include <macros.h>
#include <str.h>

#define NAME "file_bd"

#define DEFAULT_BLOCK_SIZE 512

static size_t block_size;
static aoff64_t num_blocks;
static FILE *img;

static service_id_t service_id;
static bd_srvs_t bd_srvs;
static fibril_mutex_t dev_lock;

static void print_usage(void);
static errno_t file_bd_init(const char *fname);
static void file_bd_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *);

static errno_t file_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t file_bd_close(bd_srv_t *);
static errno_t file_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static errno_t file_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *, size_t);
static errno_t file_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t file_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t file_bd_ops = {
	.open = file_bd_open,
	.close = file_bd_close,
	.read_blocks = file_bd_read_blocks,
	.write_blocks = file_bd_write_blocks,
	.get_block_size = file_bd_get_block_size,
	.get_num_blocks = file_bd_get_num_blocks
};

int main(int argc, char **argv)
{
	errno_t rc;
	char *image_name;
	char *device_name;
	category_id_t disk_cat;

	printf(NAME ": File-backed block device driver\n");

	block_size = DEFAULT_BLOCK_SIZE;

	++argv;
	--argc;
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
			++argv;
			--argc;
		} else {
			printf("Invalid option '%s'.\n", *argv);
			print_usage();
			return -1;
		}
		++argv;
		--argc;
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
		printf("%s: Unable to register device '%s': %s.\n",
		    NAME, device_name, str_error(rc));
		return rc;
	}

	rc = loc_category_get_id("disk", &disk_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Failed resolving category 'disk': %s\n", NAME, str_error(rc));
		return rc;
	}

	rc = loc_service_add_to_cat(service_id, disk_cat);
	if (rc != EOK) {
		printf("%s: Failed adding %s to category: %s",
		    NAME, device_name, str_error(rc));
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

static errno_t file_bd_init(const char *fname)
{
	bd_srvs_init(&bd_srvs);
	bd_srvs.ops = &file_bd_ops;

	async_set_fallback_port_handler(file_bd_connection, NULL);
	errno_t rc = loc_server_register(NAME);
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

static void file_bd_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	bd_conn(icall_handle, icall, &bd_srvs);
}

/** Open device. */
static errno_t file_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

/** Close device. */
static errno_t file_bd_close(bd_srv_t *bd)
{
	return EOK;
}

/** Read blocks from the device. */
static errno_t file_bd_read_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt, void *buf,
    size_t size)
{
	size_t n_rd;

	if (size < cnt * block_size)
		return EINVAL;

	/* Check whether access is within device address bounds. */
	if (ba + cnt > num_blocks) {
		printf(NAME ": Accessed blocks %" PRIuOFF64 "-%" PRIuOFF64 ", while "
		    "max block number is %" PRIuOFF64 ".\n", ba, ba + cnt - 1,
		    num_blocks - 1);
		return ELIMIT;
	}

	fibril_mutex_lock(&dev_lock);

	clearerr(img);
	if (fseek(img, ba * block_size, SEEK_SET) < 0) {
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
static errno_t file_bd_write_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	size_t n_wr;

	if (size < cnt * block_size)
		return EINVAL;

	/* Check whether access is within device address bounds. */
	if (ba + cnt > num_blocks) {
		printf(NAME ": Accessed blocks %" PRIuOFF64 "-%" PRIuOFF64 ", while "
		    "max block number is %" PRIuOFF64 ".\n", ba, ba + cnt - 1,
		    num_blocks - 1);
		return ELIMIT;
	}

	fibril_mutex_lock(&dev_lock);

	clearerr(img);
	if (fseek(img, ba * block_size, SEEK_SET) < 0) {
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

/** Get device block size. */
static errno_t file_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	*rsize = block_size;
	return EOK;
}

/** Get number of blocks on device. */
static errno_t file_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	*rnb = num_blocks;
	return EOK;
}

/**
 * @}
 */
