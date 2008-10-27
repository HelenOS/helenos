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
#include <errno.h>
#include <async.h>
#include <ipc/ipc.h>
#include <as.h>
#include <assert.h>

/** Read data from a block device.
 *
 * @param phone		Phone to be used to communicate with the device.
 * @param buffer	Communication buffer shared with the device.
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
bool blockread(int phone, void *buffer, off_t *bufpos, size_t *buflen,
    off_t *pos, void *dst, size_t size, size_t block_size)
{
	off_t offset = 0;
	size_t left = size;
	
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
			memcpy(dst + offset, buffer + *bufpos, rd);
			offset += rd;
			*bufpos += rd;
			*pos += rd;
			left -= rd;
		}
		
		if (*bufpos == *buflen) {
			/* Refill the communication buffer with a new block. */
			ipcarg_t retval;
			int rc = async_req_2_1(phone, RD_READ_BLOCK,
			    *pos / block_size, block_size, &retval);
			if ((rc != EOK) || (retval != EOK))
				return false;
			
			*bufpos = 0;
			*buflen = block_size;
		}
	}
	
	return true;
}

int dev_phone = -1;		/* FIXME */
void *dev_buffer = NULL;	/* FIXME */

block_t *block_get(dev_handle_t dev_handle, off_t offset, size_t bs)
{
	/* FIXME */
	block_t *b;
	off_t bufpos = 0;
	size_t buflen = 0;
	off_t pos = offset * bs;

	assert(dev_phone != -1);
	assert(dev_buffer);

	b = malloc(sizeof(block_t));
	if (!b)
		return NULL;
	
	b->data = malloc(bs);
	if (!b->data) {
		free(b);
		return NULL;
	}
	b->size = bs;

	if (!blockread(dev_phone, dev_buffer, &bufpos, &buflen, &pos, b->data,
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
