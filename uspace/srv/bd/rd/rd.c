/*
 * Copyright (c) 2007 Michal Konopa
 * Copyright (c) 2007 Martin Jelen
 * Copyright (c) 2007 Peter Majer
 * Copyright (c) 2007 Jakub Jermar
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

/** @addtogroup rd
 * @{
 */

/**
 * @file rd.c
 * @brief Initial RAM disk for HelenOS.
 */

#include <ipc/services.h>
#include <ipc/ns.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <align.h>
#include <bool.h>
#include <errno.h>
#include <async.h>
#include <align.h>
#include <async.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <loc.h>
#include <ipc/bd.h>
#include <macros.h>
#include <inttypes.h>

#define NAME  "rd"

/** Pointer to the ramdisk's image */
static void *rd_addr;

/** Size of the ramdisk */
static size_t rd_size;

/** Block size */
static const size_t block_size = 512;

static int rd_read_blocks(uint64_t ba, size_t cnt, void *buf);
static int rd_write_blocks(uint64_t ba, size_t cnt, const void *buf);

/** This rwlock protects the ramdisk's data.
 *
 * If we were to serve multiple requests (read + write or several writes)
 * concurrently (i.e. from two or more threads), each read and write needs to
 * be protected by this rwlock.
 *
 */
fibril_rwlock_t rd_lock;

/** Handle one connection to ramdisk.
 *
 * @param iid   Hash of the request that opened the connection.
 * @param icall Call data of the request that opened the connection.
 */
static void rd_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;
	void *fs_va = NULL;
	uint64_t ba;
	size_t cnt;
	size_t comm_size;
	
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);
	
	/*
	 * Now we wait for the client to send us its communication as_area.
	 */
	unsigned int flags;
	if (async_share_out_receive(&callid, &comm_size, &flags)) {
		(void) async_share_out_finalize(callid, &fs_va);
		if (fs_va == AS_MAP_FAILED) {
			async_answer_0(callid, EHANGUP);
			return;
		}
	} else {
		/*
		 * The client doesn't speak the same protocol.
		 * At this point we can't handle protocol variations.
		 * Close the connection.
		 */
		async_answer_0(callid, EHANGUP);
		return;
	}
	
	while (true) {
		callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/*
			 * The other side has hung up.
			 * Exit the fibril.
			 */
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case BD_READ_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = rd_read_blocks(ba, cnt, fs_va);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = rd_write_blocks(ba, cnt, fs_va);
			break;
		case BD_GET_BLOCK_SIZE:
			async_answer_1(callid, EOK, block_size);
			continue;
		case BD_GET_NUM_BLOCKS:
			async_answer_2(callid, EOK, LOWER32(rd_size / block_size),
			    UPPER32(rd_size / block_size));
			continue;
		default:
			/*
			 * The client doesn't speak the same protocol.
			 * Instead of closing the connection, we just ignore
			 * the call. This can be useful if the client uses a
			 * newer version of the protocol.
			 */
			retval = EINVAL;
			break;
		}
		async_answer_0(callid, retval);
	}
}

/** Read blocks from the device. */
static int rd_read_blocks(uint64_t ba, size_t cnt, void *buf)
{
	if ((ba + cnt) * block_size > rd_size) {
		/* Reading past the end of the device. */
		return ELIMIT;
	}
	
	fibril_rwlock_read_lock(&rd_lock);
	memcpy(buf, rd_addr + ba * block_size, block_size * cnt);
	fibril_rwlock_read_unlock(&rd_lock);
	
	return EOK;
}

/** Write blocks to the device. */
static int rd_write_blocks(uint64_t ba, size_t cnt, const void *buf)
{
	if ((ba + cnt) * block_size > rd_size) {
		/* Writing past the end of the device. */
		return ELIMIT;
	}
	
	fibril_rwlock_write_lock(&rd_lock);
	memcpy(rd_addr + ba * block_size, buf, block_size * cnt);
	fibril_rwlock_write_unlock(&rd_lock);
	
	return EOK;
}

/** Prepare the ramdisk image for operation. */
static bool rd_init(void)
{
	sysarg_t size;
	int ret = sysinfo_get_value("rd.size", &size);
	if ((ret != EOK) || (size == 0)) {
		printf("%s: No RAM disk found\n", NAME);
		return false;
	}
	
	sysarg_t addr_phys;
	ret = sysinfo_get_value("rd.address.physical", &addr_phys);
	if ((ret != EOK) || (addr_phys == 0)) {
		printf("%s: Invalid RAM disk physical address\n", NAME);
		return false;
	}
	
	rd_size = ALIGN_UP(size, block_size);
	unsigned int flags =
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE;
	
	ret = physmem_map((void *) addr_phys,
	    ALIGN_UP(rd_size, PAGE_SIZE) >> PAGE_WIDTH, flags, &rd_addr);
	if (ret != EOK) {
		printf("%s: Error mapping RAM disk\n", NAME);
		return false;
	}
	
	printf("%s: Found RAM disk at %p, %" PRIun " bytes\n", NAME,
	    (void *) addr_phys, size);
	
	async_set_client_connection(rd_connection);
	ret = loc_server_register(NAME);
	if (ret != EOK) {
		printf("%s: Unable to register driver (%d)\n", NAME, ret);
		return false;
	}
	
	service_id_t service_id;
	ret = loc_service_register("bd/initrd", &service_id);
	if (ret != EOK) {
		printf("%s: Unable to register device service\n", NAME);
		return false;
	}
	
	fibril_rwlock_initialize(&rd_lock);
	
	return true;
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS RAM disk server\n", NAME);
	
	if (!rd_init())
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	async_manager();
	
	/* Never reached */
	return 0;
}

/**
 * @}
 */
