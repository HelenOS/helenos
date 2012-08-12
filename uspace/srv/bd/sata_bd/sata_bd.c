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

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <ipc/bd.h>
#include <str.h>
#include <loc.h>
#include <macros.h>

#include <device/ahci.h>
#include "sata_bd.h"

#define NAME       "sata_bd"
#define NAMESPACE  "bd"

/** Maximum number of disks handled */
#define MAXDISKS  256

static sata_bd_dev_t disk[MAXDISKS];
static int disk_count;

/** Find SATA devices in device tree.
 *
 *  @param Device manager handle describing container for searching.  
 *
 *  @return EOK if succeed, error code otherwise.
 *
 */
static int scan_device_tree(devman_handle_t funh)
{
	devman_handle_t devh;
	devman_handle_t *cfuns;
	size_t count, i;
	int rc;
		
	/* If device is SATA, add device to the disk array. */
	disk[disk_count].sess = ahci_get_sess(funh, &disk[disk_count].dev_name);
	if(disk[disk_count].sess != NULL) {
		
		ahci_get_sata_device_name(disk[disk_count].sess,
		    SATA_DEV_NAME_LENGTH, disk[disk_count].sata_dev_name);
		
		ahci_get_block_size(disk[disk_count].sess,
		    &disk[disk_count].block_size);
		
		ahci_get_num_blocks(disk[disk_count].sess, &disk[disk_count].blocks);
				
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
static int get_sata_disks()
{
	devman_handle_t root_fun;
	int rc;
	
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
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	service_id_t dsid;
	/* Size of the communication area. */
	size_t comm_size;	
	unsigned int flags;
	int retval = 0;
	uint64_t ba;
	size_t cnt;
	int disk_id, i;

	/* Get the device service ID. */
	dsid = IPC_GET_ARG1(*icall);

	/* Determine which disk device is the client connecting to. */
	disk_id = -1;
	for (i = 0; i < MAXDISKS; i++)
		if (disk[i].service_id == dsid)
			disk_id = i;

	if (disk_id < 0) {
		async_answer_0(iid, EINVAL);
		return;
	}

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	if (!async_share_out_receive(&callid, &comm_size, &flags)) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	(void) async_share_out_finalize(callid, &fs_va);
	if (fs_va == (void *) -1) {
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
				ba = MERGE_LOUP32(IPC_GET_ARG1(call), IPC_GET_ARG2(call));
				cnt = IPC_GET_ARG3(call);
				if (cnt * disk[disk_id].block_size > comm_size) {
					retval = ELIMIT;
					break;
				}
				retval = ahci_read_blocks(disk[disk_id].sess, ba, cnt, fs_va);
				break;
			case BD_WRITE_BLOCKS:
				ba = MERGE_LOUP32(IPC_GET_ARG1(call), IPC_GET_ARG2(call));
				cnt = IPC_GET_ARG3(call);
				if (cnt * disk[disk_id].block_size > comm_size) {
					retval = ELIMIT;
					break;
				}
				retval = ahci_write_blocks(disk[disk_id].sess, ba, cnt, fs_va);
				break;
			case BD_GET_BLOCK_SIZE:
				async_answer_1(callid, EOK, disk[disk_id].block_size);
				continue;
			case BD_GET_NUM_BLOCKS:
				async_answer_2(callid, EOK, LOWER32(disk[disk_id].blocks),
				    UPPER32(disk[disk_id].blocks));
				break;
			default:
				retval = EINVAL;
				break;
			}
		async_answer_0(callid, retval);
	}
}

int main(int argc, char **argv)
{
	int rc;
	
	async_set_client_connection(sata_bd_connection);
	rc = loc_server_register(NAME);
	if (rc < 0) {
		printf(NAME ": Unable to register driver.\n");
		return rc;
	}
	
	rc = get_sata_disks();
	if (rc != EOK) {
		return rc;
	}

	for(int i=0; i < disk_count; i++) {
		char name[1024];
		snprintf(name, 1024, "%s/%s", NAMESPACE, disk[i].dev_name);
		rc = loc_service_register(name, &disk[i].service_id);
		if (rc != EOK) {
			printf(NAME ": Unable to register device %s.\n", name);
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
