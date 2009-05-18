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
 * @file	rd.c
 * @brief	Initial RAM disk for HelenOS.
 */

#include <ipc/ipc.h>
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
#include <futex.h>
#include <stdio.h>
#include <devmap.h>
#include <ipc/bd.h>

#define NAME "rd"

/** Pointer to the ramdisk's image. */
static void *rd_addr;
/** Size of the ramdisk. */
static size_t rd_size;

/**
 * This futex protects the ramdisk's data.
 * If we were to serve multiple requests (read + write or several writes)
 * concurrently (i.e. from two or more threads), each read and write needs to be
 * protected by this futex.
 */ 
atomic_t rd_futex = FUTEX_INITIALIZER;

/** Handle one connection to ramdisk.
 *
 * @param iid		Hash of the request that opened the connection.
 * @param icall		Call data of the request that opened the connection.
 */
static void rd_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;
	void *fs_va = NULL;
	off_t offset;
	size_t block_size;
	size_t maxblock_size;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	ipc_answer_0(iid, EOK);

	/*
	 * Now we wait for the client to send us its communication as_area.
	 */
	int flags;
	if (ipc_share_out_receive(&callid, &maxblock_size, &flags)) {
		fs_va = as_get_mappable_page(maxblock_size);
		if (fs_va) {
			(void) ipc_share_out_finalize(callid, fs_va);
		} else {
			ipc_answer_0(callid, EHANGUP);
			return;		
		}
	} else {
		/*
		 * The client doesn't speak the same protocol.
		 * At this point we can't handle protocol variations.
		 * Close the connection.
		 */
		ipc_answer_0(callid, EHANGUP);
		return;
	}
	
	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			/*
			 * The other side has hung up.
			 * Answer the message and exit the fibril.
			 */
			ipc_answer_0(callid, EOK);
			return;
		case BD_READ_BLOCK:
			offset = IPC_GET_ARG1(call);
			block_size = IPC_GET_ARG2(call);
			if (block_size > maxblock_size) {
				/*
				 * Maximum block size exceeded.
				 */
				retval = ELIMIT;
				break;
			}
			if (offset * block_size > rd_size - block_size) {
				/*
				 * Reading past the end of the device.
				 */
				retval = ELIMIT;
				break;
			}
			futex_down(&rd_futex);
			memcpy(fs_va, rd_addr + offset * block_size, block_size);
			futex_up(&rd_futex);
			retval = EOK;
			break;
		case BD_WRITE_BLOCK:
			offset = IPC_GET_ARG1(call);
			block_size = IPC_GET_ARG2(call);
			if (block_size > maxblock_size) {
				/*
				 * Maximum block size exceeded.
				 */
				retval = ELIMIT;
				break;
			}
			if (offset * block_size > rd_size - block_size) {
				/*
				 * Writing past the end of the device.
				 */
				retval = ELIMIT;
				break;
			}
			futex_up(&rd_futex);
			memcpy(rd_addr + offset * block_size, fs_va, block_size);
			futex_down(&rd_futex);
			retval = EOK;
			break;
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
		ipc_answer_0(callid, retval);
	}
}

/** Prepare the ramdisk image for operation. */
static bool rd_init(void)
{
	rd_size = sysinfo_value("rd.size");
	void *rd_ph_addr = (void *) sysinfo_value("rd.address.physical");
	
	if (rd_size == 0) {
		printf(NAME ": No RAM disk found\n");
		return false;
	}
	
	rd_addr = as_get_mappable_page(rd_size);
	
	int flags = AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE;
	int retval = physmem_map(rd_ph_addr, rd_addr,
	    ALIGN_UP(rd_size, PAGE_SIZE) >> PAGE_WIDTH, flags);
	
	if (retval < 0) {
		printf(NAME ": Error mapping RAM disk\n");
		return false;
	}
	
	printf(NAME ": Found RAM disk at %p, %d bytes\n", rd_ph_addr, rd_size);
	
	int driver_phone = devmap_driver_register(NAME, rd_connection);
	if (driver_phone < 0) {
		printf(NAME ": Unable to register driver\n");
		return false;
	}
	
	int dev_handle;
	if (devmap_device_register(driver_phone, "initrd", &dev_handle) != EOK) {
		ipc_hangup(driver_phone);
		printf(NAME ": Unable to register device\n");
		return false;
	}

	/*
	 * Create the second device.
	 * We need at least two devices for the sake of testing of non-root
	 * mounts. Of course it would be better to allow the second device
	 * be created dynamically...
	 */
	if (devmap_device_register(driver_phone, "spared", &dev_handle) != EOK) {
		ipc_hangup(driver_phone);
		printf(NAME ": Unable to register device\n");
		return false;
	}
	
	return true;
}

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS RAM disk server\n");
	
	if (!rd_init())
		return -1;
	
	printf(NAME ": Accepting connections\n");
	async_manager();

	/* Never reached */
	return 0;
}

/**
 * @}
 */ 
