/*
 * Copyright (c) 2012 Petr Jerman
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
 * @brief SATA disk driver
 *
 */

#include <stddef.h>
#include <bd_srv.h>
#include <devman.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <str.h>
#include <loc.h>
#include <macros.h>
#include <task.h>

#include <ahci_iface.h>
#include "sata_bd.h"

#define NAME       "sata_bd"
#define NAMESPACE  "bd"

/** Maximum number of disks handled */
#define MAXDISKS  256

static sata_bd_dev_t disk[MAXDISKS];
static int disk_count;

static errno_t sata_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t sata_bd_close(bd_srv_t *);
static errno_t sata_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static errno_t sata_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *, size_t);
static errno_t sata_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t sata_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t sata_bd_ops = {
	.open = sata_bd_open,
	.close = sata_bd_close,
	.read_blocks = sata_bd_read_blocks,
	.write_blocks = sata_bd_write_blocks,
	.get_block_size = sata_bd_get_block_size,
	.get_num_blocks = sata_bd_get_num_blocks
};

static sata_bd_dev_t *bd_srv_sata(bd_srv_t *bd)
{
	return (sata_bd_dev_t *) bd->srvs->sarg;
}

/** Find SATA devices in device tree.
 *
 *  @param Device manager handle describing container for searching.  
 *
 *  @return EOK if succeed, error code otherwise.
 *
 */
static errno_t scan_device_tree(devman_handle_t funh)
{
	devman_handle_t devh;
	devman_handle_t *cfuns;
	size_t count, i;
	errno_t rc;
		
	/* If device is SATA, add device to the disk array. */
	disk[disk_count].sess = ahci_get_sess(funh, &disk[disk_count].dev_name);
	if(disk[disk_count].sess != NULL) {
		
		ahci_get_sata_device_name(disk[disk_count].sess,
		    SATA_DEV_NAME_LENGTH, disk[disk_count].sata_dev_name);
		
		ahci_get_block_size(disk[disk_count].sess,
		    &disk[disk_count].block_size);
		
		ahci_get_num_blocks(disk[disk_count].sess, &disk[disk_count].blocks);
		
		bd_srvs_init(&disk[disk_count].bds);
		disk[disk_count].bds.ops = &sata_bd_ops;
		disk[disk_count].bds.sarg = &disk[disk_count];
		
		printf("Device %s - %s , blocks: %lu, block_size: %lu\n", 
		    disk[disk_count].dev_name, disk[disk_count].sata_dev_name,
			    (long unsigned int) disk[disk_count].blocks,
				(long unsigned int) disk[disk_count].block_size);

		++disk_count;
	}
	
	/* search children */
	rc = devman_fun_get_child(funh, &devh);
	if (rc == ENOENT)
		return EOK;

	if (rc != EOK) {
		printf(NAME ": Failed getting child device for function %s.\n", "xxx");
		return rc;
	}

	rc = devman_dev_get_functions(devh, &cfuns, &count);
	if (rc != EOK) {
		printf(NAME ": Failed getting list of functions for device %s.\n",
		    "xxx");
		return rc;
	}

	for (i = 0; i < count; i++)
		scan_device_tree(cfuns[i]);

	free(cfuns);
	return EOK;
}

/** Find sata devices in device tree from root.
 *
 *  @return EOK if succeed, error code otherwise.
 *
 */
static errno_t get_sata_disks(void)
{
	devman_handle_t root_fun;
	errno_t rc;
	
	disk_count = 0;

	rc = devman_fun_get_handle("/", &root_fun, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving root function.\n");
		return EIO;
	}
	
	scan_device_tree(root_fun);
	
	return EOK;
}

/** Block device connection handler. */
static void sata_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	service_id_t dsid;
	int disk_id, i;

	/* Get the device service ID. */
	dsid = IPC_GET_ARG2(*icall);

	/* Determine which disk device is the client connecting to. */
	disk_id = -1;
	for (i = 0; i < MAXDISKS; i++)
		if (disk[i].service_id == dsid)
			disk_id = i;

	if (disk_id < 0) {
		async_answer_0(iid, EINVAL);
		return;
	}

	bd_conn(iid, icall, &disk[disk_id].bds);
}

/** Open device. */
static errno_t sata_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

/** Close device. */
static errno_t sata_bd_close(bd_srv_t *bd)
{
	return EOK;
}

/** Read blocks from partition. */
static errno_t sata_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt, void *buf,
    size_t size)
{
	sata_bd_dev_t *sbd = bd_srv_sata(bd);

	if (size < cnt * sbd->block_size)
		return EINVAL;

	return ahci_read_blocks(sbd->sess, ba, cnt, buf);
}

/** Write blocks to partition. */
static errno_t sata_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	sata_bd_dev_t *sbd = bd_srv_sata(bd);

	if (size < cnt * sbd->block_size)
		return EINVAL;

	return ahci_write_blocks(sbd->sess, ba, cnt, (void *)buf);
}

/** Get device block size. */
static errno_t sata_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	sata_bd_dev_t *sbd = bd_srv_sata(bd);

	*rsize = sbd->block_size;
	return EOK;
}

/** Get number of blocks on device. */
static errno_t sata_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	sata_bd_dev_t *sbd = bd_srv_sata(bd);

	*rnb = sbd->blocks;
	return EOK;
}


int main(int argc, char **argv)
{
	errno_t rc;
	category_id_t disk_cat;
	
	async_set_fallback_port_handler(sata_bd_connection, NULL);
	rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf(NAME ": Unable to register driver: %s.\n", str_error(rc));
		return rc;
	}
	
	rc = get_sata_disks();
	if (rc != EOK) {
		// TODO: log the error
		return rc;
	}

	rc = loc_category_get_id("disk", &disk_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Failed resolving category 'disk': %s.\n", NAME, str_error(rc));
		return rc;
	}

	for(int i = 0; i < disk_count; i++) {
		char name[1024];
		snprintf(name, 1024, "%s/%s", NAMESPACE, disk[i].dev_name);
		rc = loc_service_register(name, &disk[i].service_id);
		if (rc != EOK) {
			printf(NAME ": Unable to register device %s: %s\n", name, str_error(rc));
			return rc;
		}

		rc = loc_service_add_to_cat(disk[i].service_id, disk_cat);
		if (rc != EOK) {
			printf("%s: Failed adding %s to category: %s.",
			    NAME, disk[i].dev_name, str_error(rc));
			return rc;
		}
	}

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/**
 * @}
 */
