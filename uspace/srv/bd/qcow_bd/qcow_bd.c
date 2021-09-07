/*
 * Copyright (c) 2021 Erik Kučák
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

/** @addtogroup qcow_bd
 * @{
 */

/**
 * @file
 * @brief QCOW file block device driver
 *
 * Allows accessing a file as a block device in QCOW format. Useful for, e.g., mounting
 * a disk image.
 */

#include "qcow_bd.h"

static FILE *img;
static QCowHeader header;
static QcowState state;

static service_id_t service_id;
static bd_srvs_t bd_srvs;
static fibril_mutex_t dev_lock;
static void print_usage(void);
static errno_t qcow_bd_init(const char *fname);
static void qcow_bd_connection(ipc_call_t *icall, void *);

static errno_t qcow_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t qcow_bd_close(bd_srv_t *);
static errno_t qcow_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static errno_t qcow_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *, size_t);
static errno_t qcow_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t qcow_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t qcow_bd_ops = {
	.open = qcow_bd_open,
	.close = qcow_bd_close,
	.read_blocks = qcow_bd_read_blocks,
	.write_blocks = qcow_bd_write_blocks,
	.get_block_size = qcow_bd_get_block_size,
	.get_num_blocks = qcow_bd_get_num_blocks
};

int main(int argc, char **argv)
{
	errno_t rc;
	char *image_name;
	char *device_name;
	category_id_t disk_cat;

	printf(NAME ": File-backed block device driver in QCOW format\n");

	state.block_size = DEFAULT_BLOCK_SIZE;

	++argv;
	--argc;
	while (*argv != NULL && (*argv)[0] == '-') {
		/* Option */
		if (str_cmp(*argv, "-b") == 0) {
			if (argc < 2) {
				fprintf(stderr, "Argument missing.\n");
				print_usage();
				return -1;
			}

			rc = str_size_t(argv[1], NULL, 10, true, &state.block_size);
			if (rc != EOK || state.block_size == 0) {
				fprintf(stderr, "Invalid block size '%s'.\n", argv[1]);
				print_usage();
				return -1;
			}
			++argv;
			--argc;
		} else {
			fprintf(stderr, "Invalid option '%s'.\n", *argv);
			print_usage();
			return -1;
		}
		++argv;
		--argc;
	}

	if (argc < 2) {
		fprintf(stderr, "Missing arguments.\n");
		print_usage();
		return -1;
	}

	image_name = argv[0];
	device_name = argv[1];

	if (qcow_bd_init(image_name) != EOK)
		return -1;

	rc = loc_service_register(device_name, &service_id);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to register device '%s': %s.\n",
		    NAME, device_name, str_error(rc));
		return rc;
	}

	rc = loc_category_get_id("disk", &disk_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		fprintf(stderr, "%s: Failed resolving category 'disk': %s\n", NAME, str_error(rc));
		return rc;
	}

	rc = loc_service_add_to_cat(service_id, disk_cat);
	if (rc != EOK) {
		fprintf(stderr, "%s: Failed adding %s to category: %s",
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

static errno_t compute_table_sizes()
{
	/* Computing sizes in bytes */
	state.cluster_size = 1 << header.cluster_bits;
	state.l2_size = (1 << header.l2_bits) * 8;

	/* Computing size of L1 table in bytes */
	state.l1_size = state.cluster_size * (1 << header.l2_bits);
	if (header.size % state.l1_size != 0)
		state.l1_size = (header.size / state.l1_size) + 1;
	else
		state.l1_size = header.size / state.l1_size;

	state.l1_size *= 8;

	/* Computing number of blocks */
	if (header.size % state.block_size != 0)
		state.num_blocks = (header.size / state.block_size) + 1;
	else
		state.num_blocks = (header.size / state.block_size);

	return EOK;
}

static errno_t read_l1_table()
{
	/* Allocating memory for l1_table */
	state.l1_table_offset = header.l1_table_offset;
	state.l2_references = (uint64_t*)malloc(state.l1_size);

	/* Reading all L2 table references from L1 table */
	clearerr(img);
	printf("READING 2,5\n");
	if (fseek(img, state.l1_table_offset, SEEK_SET) < 0)
		return EIO;

	size_t n_rd = fread(state.l2_references, state.l1_size, 1, img);

	if (n_rd < 1)
		return EINVAL;

	/* Clearing last bit in all references which tells us whether cluster is compressed */
	for (uint64_t i = 0; i < state.l1_size / sizeof(uint64_t); i++) {
		state.l2_references[i] = __builtin_bswap64(state.l2_references[i]);
		if (state.l2_references[i] & QCOW_OFLAG_COMPRESSED) {
			fprintf(stderr, "Compression is not supported!\n");
			return ENOTSUP;
		}
	}

	if (ferror(img))
		return EIO;	/* Read error */
	return EOK;
}

static errno_t qcow_bd_init(const char *fname)
{
	/* Register driver */
	bd_srvs_init(&bd_srvs);
	bd_srvs.ops = &qcow_bd_ops;

	async_set_fallback_port_handler(qcow_bd_connection, NULL);
	errno_t rc = loc_server_register(NAME);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to register driver.\n", NAME);
		return rc;
	}

	/* Try to open file */
	img = fopen(fname, "rb+");
	if (img == NULL)
		return EINVAL;

	if (fseek(img, 0, SEEK_END) != 0) {
		fclose(img);
		return EIO;
	}

	if (fseek(img, 0, SEEK_SET) < 0)
		return EIO;

	/* Read the file header */
	size_t n_rd = fread(&header, sizeof(header), 1, img);

	if (n_rd < 1)
		return EINVAL;

	/* Swap values to big-endian */
 	header.magic = __builtin_bswap32(header.magic);
	header.version = __builtin_bswap32(header.version);
	header.backing_file_offset = __builtin_bswap64(header.backing_file_offset);
	header.backing_file_size = __builtin_bswap32(header.backing_file_size);
	header.mtime = __builtin_bswap32(header.mtime);
	header.size = __builtin_bswap64(header.size);
	header.crypt_method = __builtin_bswap32(header.crypt_method);
	header.l1_table_offset = __builtin_bswap64(header.l1_table_offset);

	/* Verify all values from file header */
	if (ferror(img))
		return EIO;

	if (header.magic == QCOW_MAGIC) {
		errno_t error = compute_table_sizes();
		if (error != EOK) {
			return error;
		}
		error = read_l1_table();
		if (error != EOK) {
			return error;
		}
	}

	if (header.version != QCOW_VERSION) {
		fprintf(stderr, "Version: %d is not supported!\n", header.version);
		return ENOTSUP;
	}

	if (header.crypt_method != QCOW_CRYPT_NONE) {
		fprintf(stderr, "Encryption is not supported!\n");
		return ENOTSUP;
	}

	/* Initialize mutex variable */
	fibril_mutex_initialize(&dev_lock);

	return EOK;
}

static void qcow_bd_connection(ipc_call_t *icall, void *arg)
{
	bd_conn(icall, &bd_srvs);
}

/** Open device. */
static errno_t qcow_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

/** Close device. */
static errno_t qcow_bd_close(bd_srv_t *bd)
{
	free(state.l2_references);
	fclose(img);
	return EOK;
}

/** From the offset of the given block compute its offset which is relative from the start of qcow file. */
static uint64_t get_block_offset(uint64_t offset, errno_t *error)
{
	/* Compute l1 table index from the offset  */
	uint64_t l1_table_index_bit_shift =  header.cluster_bits + header.l2_bits;
	uint64_t l1_table_index = (offset & 0x7fffffffffffffffULL) >> l1_table_index_bit_shift;

	/* Reading l2 reference from the l1 table */
	uint64_t l2_table_reference = state.l2_references[l1_table_index];

	if (l2_table_reference == QCOW_UNALLOCATED_REFERENCE)
		return QCOW_UNALLOCATED_REFERENCE;

	/* Compute l2 table index from the offset  */
	uint64_t l2_table_index = (offset >> header.cluster_bits) & (state.l2_size - 1);

	/* Reading cluster reference from the l2 table */
	if (fseek(img, l2_table_reference + l2_table_index * sizeof(uint64_t), SEEK_SET) < 0){
		*error = EIO;
		return EIO;
	}

	uint64_t cluster_reference;
	size_t n_rd = fread(&cluster_reference, sizeof(uint64_t), 1, img);

	if (n_rd < 1) {
		*error = EINVAL;
		return EINVAL;
	}

	cluster_reference = __builtin_bswap64(cluster_reference);

	if (cluster_reference & QCOW_OFLAG_COMPRESSED) {
		fprintf(stderr, "Compression is not supported!\n");
		*error = ENOTSUP;
		return ENOTSUP;
	}

	if (cluster_reference == QCOW_UNALLOCATED_REFERENCE)
		return QCOW_UNALLOCATED_REFERENCE;

	/* Compute cluster block offset from the offset  */
	uint64_t cluster_block_bit_mask = ~(0xffffffffffffffffULL <<  header.cluster_bits);
	uint64_t cluster_block_offset = offset & cluster_block_bit_mask;

	return cluster_reference + cluster_block_offset;
}

/** Read blocks from the device. */
static errno_t qcow_bd_read_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt, void *buf,
    size_t size)
{
	if (size < cnt * state.block_size)
		return EINVAL;

	/* Check whether access is within device address bounds. */
	if (ba + cnt > state.num_blocks) {
		fprintf(stderr, NAME ": Accessed blocks %" PRIuOFF64 "-%" PRIuOFF64 ", while "
		    "max block number is %" PRIuOFF64 ".\n", ba, ba + cnt - 1,
		    state.num_blocks - 1);
		return ELIMIT;
	}

	fibril_mutex_lock(&dev_lock);

	for (uint64_t i = 0; i < cnt; i++) {
		/* Compute block offset which is relative from the start of qcow file */
		errno_t error = EOK;
		uint64_t block_offset = get_block_offset((ba + i) * state.block_size, &error);
		if (error != EOK)
			return error;

		/* If there is empty reference then continue */
		if (block_offset == QCOW_UNALLOCATED_REFERENCE)
			continue;

		clearerr(img);
		if (fseek(img, block_offset, SEEK_SET) < 0) {
			fibril_mutex_unlock(&dev_lock);
			return EIO;
		}

		size_t n_rd = fread(buf + i * state.block_size, state.block_size, 1, img);

		if (n_rd < 1) {
			fibril_mutex_unlock(&dev_lock);
			return EINVAL;
		}

		if (ferror(img)) {
			fibril_mutex_unlock(&dev_lock);
			return EIO;
		}
	}

	fibril_mutex_unlock(&dev_lock);
	return EOK;
}

/** Write blocks to the device. */
static errno_t qcow_bd_write_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	// TODO
	return EOK;
}

/** Get device block size. */
static errno_t qcow_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	*rsize = state.block_size;
	return EOK;
}

/** Get number of blocks on device. */
static errno_t qcow_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	*rnb = state.num_blocks;
	return EOK;
}

/**
 * @}
 */