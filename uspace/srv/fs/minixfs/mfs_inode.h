/*
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup fs
 * @{
 */ 


#ifndef _MFS_INODE_H_
#define _MFS_INODE_H_

#include "mfs_const.h"

/*Declaration of the V2 inode as it is on disk.*/

struct mfs_v2_inode {
	uint16_t 	mode;		/*File type, protection, etc.*/
	uint16_t 	n_links;	/*How many links to this file.*/
	int16_t 	uid;		/*User id of the file owner.*/
	uint16_t 	gid;		/*Group number.*/
	int32_t 	size;		/*Current file size in bytes.*/
	int32_t		atime;		/*When was file data last accessed.*/
	int32_t		mtime;		/*When was file data last changed.*/
	int32_t		ctime;		/*When was inode data last changed.*/
	/*Block nums for direct, indirect, and double indirect zones.*/
	uint32_t	zone[V2_NR_TZONES];
} __attribute__ ((packed));

#endif

/**
 * @}
 */ 

