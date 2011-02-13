/*
 * Copyright (c) 2011 Martin Sucha
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

/** @addtogroup libext2
 * @{
 */
/**
 * @file
 */

#include "libext2.h"
#include <errno.h>
#include <libblock.h>
#include <malloc.h>

/**
 * Initialize an instance of filesystem on the device.
 * This function reads superblock from the device and
 * initializes libblock cache with appropriate logical block size.
 */
int ext2_filesystem_init(ext2_filesystem_t *fs, devmap_handle_t dev) {
	int rc;
	ext2_superblock_t *temp_superblock;
	
	fs->device = dev;
	
	rc = block_init(fs->device, 2048);
	if (rc != EOK) {
		return rc;
	}
	
	rc = ext2_superblock_read_direct(dev, &temp_superblock);
	if (rc != EOK) {
		block_fini(dev);
		return rc;
	}
	
	free(temp_superblock);
	
	return EOK; 
}

/**
 * Finalize an instance of filesystem
 */
void ext2_filesystem_fini(ext2_filesystem_t *fs) {
	block_fini(fs->device);
}


/** @}
 */
