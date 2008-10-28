/*
 * Copyright (c) 2008 Jakub Jermar 
 * Copyright (c) 2008 Martin Decky 
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

/** @addtogroup libblock 
 * @{
 */ 
/**
 * @file
 * @brief
 */

#include "libblock.h" 
#include "../../srv/vfs/vfs.h"
#include "../../srv/rd/rd.h"
#include <ipc/devmap.h>
#include <ipc/services.h>
#include <errno.h>
#include <sys/mman.h>
#include <async.h>
#include <ipc/ipc.h>
#include <as.h>
#include <assert.h>
#include <futex.h>
#include <libadt/list.h>

/** Lock protecting the device connection list */
static futex_t dcl_lock = FUTEX_INITIALIZER;
/** Device connection list head. */
static LIST_INITIALIZE(dcl_head);

typedef struct {
	link_t link;
	int dev_handle;
	int dev_phone;
	void *com_area;
	size_t com_size;
	void *bb_buf;
	off_t bb_off;
	size_t bb_size;
} devcon_t;

static devcon_t *devcon_search(dev_handle_t dev_handle)
{
	link_t *cur;

	futex_down(&dcl_lock);
	for (cur = dcl_head.next; cur != &dcl_head; cur = cur->next) {
		devcon_t *devcon = list_get_instance(cur, devcon_t, link);
		if (devcon->dev_handle == dev_handle) {
			futex_up(&dcl_lock);
			return devcon;
		}
	}
	futex_up(&dcl_lock);
	return NULL;
}

static int devcon_add(dev_handle_t dev_handle, int dev_phone, void *com_area,
   size_t com_size, void *bb_buf, off_t bb_off, size_t bb_size)
{
	link_t *cur;
	devcon_t *devcon;

	devcon = malloc(sizeof(devcon_t));
	if (!devcon)
		return ENOMEM;
	
	link_initialize(&devcon->link);
	devcon->dev_handle = dev_handle;
	devcon->dev_phone = dev_phone;
	devcon->com_area = com_area;
	devcon->com_size = com_size;
	devcon->bb_buf = bb_buf;
	devcon->bb_off = bb_off;
	devcon->bb_size = bb_size;

	futex_down(&dcl_lock);
	for (cur = dcl_head.next; cur != &dcl_head; cur = cur->next) {
		devcon_t *d = list_get_instance(cur, devcon_t, link);
		if (d->dev_handle == dev_handle) {
			futex_up(&dcl_lock);
			free(devcon);
			return EEXIST;
		}
	}
	list_append(&devcon->link, &dcl_head);
	futex_up(&dcl_lock);
	return EOK;
}

static void devcon_remove(devcon_t *devcon)
{
	futex_down(&dcl_lock);
	list_remove(&devcon->link);
	futex_up(&dcl_lock);
}

int
block_init(dev_handle_t dev_handle, size_t com_size, off_t bb_off,
    size_t bb_size)
{
	int rc;
	int dev_phone;
	void *com_area;
	void *bb_buf;
	
	bb_buf = malloc(bb_size);
	if (!bb_buf)
		return ENOMEM;
	
	com_area = mmap(NULL, com_size, PROTO_READ | PROTO_WRITE,
	    MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if (!com_area) {
		free(bb_buf);
		return ENOMEM;
	}
	dev_phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP,
	    DEVMAP_CONNECT_TO_DEVICE, dev_handle);

	if (dev_phone < 0) {
		free(bb_buf);
		munmap(com_area, com_size);
		return dev_phone;
	}

	rc = ipc_share_out_start(dev_phone, com_area,
	    AS_AREA_READ | AS_AREA_WRITE);
	if (rc != EOK) {
		free(bb_buf);
	    	munmap(com_area, com_size);
		ipc_hangup(dev_phone);
		return rc;
	}
	
	rc = devcon_add(dev_handle, dev_phone, com_area, com_size, bb_buf,
	    bb_off, bb_size);
	if (rc != EOK) {
		free(bb_buf);
		munmap(com_area, com_size);
		ipc_hangup(dev_phone);
		return rc;
	}

	off_t bufpos = 0;
	size_t buflen = 0;
	if (!block_read(dev_handle, &bufpos, &buflen, &bb_off,
	    bb_buf, bb_size, bb_size)) {
		block_fini(dev_handle);
		return EIO;	/* XXX real error code */
	}
	
	return EOK;
}

void block_fini(dev_handle_t dev_handle)
{
	devcon_t *devcon = devcon_search(dev_handle);
	assert(devcon);
	
	devcon_remove(devcon);

	free(devcon->bb_buf);
	munmap(devcon->com_area, devcon->com_size);
	ipc_hangup(devcon->dev_phone);

	free(devcon);	
}

void *block_bb_get(dev_handle_t dev_handle)
{
	devcon_t *devcon = devcon_search(dev_handle);
	assert(devcon);
	return devcon->bb_buf;
}

/** Read data from a block device.
 *
 * @param dev_handle	Device handle of the block device.
 * @param bufpos	Pointer to the first unread valid offset within the
 * 			communication buffer.
 * @param buflen	Pointer to the number of unread bytes that are ready in
 * 			the communication buffer.
 * @param pos		Device position to be read.
 * @param dst		Destination buffer.
 * @param size		Size of the destination buffer.
 * @param block_size	Block size to be used for the transfer.
 *
 * @return		True on success, false on failure.
 */
bool
block_read(int dev_handle, off_t *bufpos, size_t *buflen, off_t *pos, void *dst,
    size_t size, size_t block_size)
{
	off_t offset = 0;
	size_t left = size;
	devcon_t *devcon = devcon_search(dev_handle);
	assert(devcon);
	
	while (left > 0) {
		size_t rd;
		
		if (*bufpos + left < *buflen)
			rd = left;
		else
			rd = *buflen - *bufpos;
		
		if (rd > 0) {
			/*
			 * Copy the contents of the communication buffer to the
			 * destination buffer.
			 */
			memcpy(dst + offset, devcon->com_area + *bufpos, rd);
			offset += rd;
			*bufpos += rd;
			*pos += rd;
			left -= rd;
		}
		
		if (*bufpos == *buflen) {
			/* Refill the communication buffer with a new block. */
			ipcarg_t retval;
			int rc = async_req_2_1(devcon->dev_phone, RD_READ_BLOCK,
			    *pos / block_size, block_size, &retval);
			if ((rc != EOK) || (retval != EOK))
				return false;
			
			*bufpos = 0;
			*buflen = block_size;
		}
	}
	
	return true;
}

block_t *block_get(dev_handle_t dev_handle, off_t offset, size_t bs)
{
	/* FIXME */
	block_t *b;
	off_t bufpos = 0;
	size_t buflen = 0;
	off_t pos = offset * bs;

	b = malloc(sizeof(block_t));
	if (!b)
		return NULL;
	
	b->data = malloc(bs);
	if (!b->data) {
		free(b);
		return NULL;
	}
	b->size = bs;

	if (!block_read(dev_handle, &bufpos, &buflen, &pos, b->data,
	    bs, bs)) {
		free(b->data);
		free(b);
		return NULL;
	}

	return b;
}

void block_put(block_t *block)
{
	/* FIXME */
	free(block->data);
	free(block);
}

/** @}
 */
