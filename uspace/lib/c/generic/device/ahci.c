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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <ipc/dev_iface.h>
#include <assert.h>
#include <device/ahci.h>
#include <errno.h>
#include <async.h>
#include <ipc/services.h>
#include <stdio.h>
#include <as.h>

typedef enum {
	IPC_M_AHCI_GET_SATA_DEVICE_NAME,
	IPC_M_AHCI_GET_NUM_BLOCKS,
	IPC_M_AHCI_GET_BLOCK_SIZE,
	IPC_M_AHCI_READ_BLOCKS,
	IPC_M_AHCI_WRITE_BLOCKS,
} ahci_iface_funcs_t;

#define MAX_NAME_LENGTH  1024

#define LO(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) & 0xffffffff))

#define HI(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) >> 32))

async_sess_t* ahci_get_sess(devman_handle_t funh, char **name)
{
	// FIXME: Use a better way than substring match
	
	*name = NULL;
	
	char devn[MAX_NAME_LENGTH];
	int rc = devman_fun_get_name(funh, devn, MAX_NAME_LENGTH);
	if (rc != EOK)
		return NULL;
	
	size_t devn_size = str_size(devn);
	
	if ((devn_size > 5) && (str_lcmp(devn, "ahci_", 5) == 0)) {
		async_sess_t *sess = devman_device_connect(EXCHANGE_PARALLEL,
		    funh, IPC_FLAG_BLOCKING);
		
		if (sess) {
			*name = str_dup(devn);
			return sess;
		}
	}
	
	return NULL;
}

int ahci_get_sata_device_name(async_sess_t *sess, size_t sata_dev_name_length,
    char *sata_dev_name)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;
	
	aid_t req = async_send_2(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_GET_SATA_DEVICE_NAME, sata_dev_name_length, NULL);
	
	async_data_read_start(exch, sata_dev_name, sata_dev_name_length);
	
	sysarg_t rc;
	async_wait_for(req, &rc);
	
	return rc;
}

int ahci_get_num_blocks(async_sess_t *sess, uint64_t *blocks)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;
	
	sysarg_t blocks_hi;
	sysarg_t blocks_lo;
	int rc = async_req_1_2(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_GET_NUM_BLOCKS, &blocks_hi, &blocks_lo);
	
	async_exchange_end(exch);
	
	if (rc == EOK) {
		*blocks = (((uint64_t) blocks_hi) << 32)
		    | (((uint64_t) blocks_lo) & 0xffffffff);
	}
	
	return rc;
}

int ahci_get_block_size(async_sess_t *sess, size_t *blocks_size)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;
	
	sysarg_t bs;
	int rc = async_req_1_1(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_GET_BLOCK_SIZE, &bs);
	
	async_exchange_end(exch);
	
	if (rc == EOK)
		*blocks_size = (size_t) bs;
	
	return rc;
}

int ahci_read_blocks(async_sess_t *sess, uint64_t blocknum, size_t count,
    void *buf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;
	
	aid_t req;
	req = async_send_4(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_READ_BLOCKS, HI(blocknum),  LO(blocknum), count, NULL);
	
	async_share_out_start(exch, buf, AS_AREA_READ | AS_AREA_WRITE);
	
	async_exchange_end(exch);
	
	sysarg_t rc;
	async_wait_for(req, &rc);
	
	return rc;
}

int ahci_write_blocks(async_sess_t *sess, uint64_t blocknum, size_t count,
    void* buf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;
	
	aid_t req = async_send_4(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_WRITE_BLOCKS, HI(blocknum),  LO(blocknum), count, NULL);
	
	async_share_out_start(exch, buf, AS_AREA_READ | AS_AREA_WRITE);
	
	async_exchange_end(exch);
	
	sysarg_t rc;
	async_wait_for(req, &rc);
	
	return rc;
}

/** @}
 */
