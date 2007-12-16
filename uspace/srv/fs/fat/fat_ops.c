/*
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

/** @addtogroup fs
 * @{
 */ 

/**
 * @file	fat_ops.c
 * @brief	Implementation of VFS operations for the FAT file system server.
 */

#include "fat.h"
#include "../../vfs/vfs.h"
#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>

#define PLB_GET_CHAR(i)		(fat_reg.plb_ro[(i) % PLB_SIZE])

#define FAT_NAME_LEN		8
#define FAT_EXT_LEN		3

#define FAT_PAD			' ' 

#define FAT_DENTRY_UNUSED	0x00
#define FAT_DENTRY_E5_ESC	0x05
#define FAT_DENTRY_DOT		0x2e
#define FAT_DENTRY_ERASED	0xe5

/** Compare one component of path to a directory entry.
 *
 * @param dentry	Directory entry to compare the path component with.
 * @param start		Index into PLB where the path component starts.
 * @param last		Index of the last character of the path in PLB.
 *
 * @return		Zero on failure or delta such that (index + delta) %
 *			PLB_SIZE points	to a new path component in PLB.
 */
static unsigned match_path_component(fat_dentry_t *dentry, unsigned start,
    unsigned last)
{
	unsigned cur;	/* current position in PLB */
	int pos;	/* current position in dentry->name or dentry->ext */
	bool name_processed = false;
	bool dot_processed = false;
	bool ext_processed = false;

	if (last < start)
		last += PLB_SIZE;
	for (pos = 0, cur = start; (cur <= last) && (PLB_GET_CHAR(cur) != '/');
	    pos++, cur++) {
		if (!name_processed) {
			if ((pos == FAT_NAME_LEN - 1) ||
			    (dentry->name[pos + 1] == FAT_PAD)) {
			    	/* this is the last character in name */
				name_processed = true;
			}
			if (dentry->name[0] == FAT_PAD) {
				/* name is empty */
				name_processed = true;
			} else if ((pos == 0) && (dentry->name[pos] ==
			    FAT_DENTRY_E5_ESC)) {
				if (PLB_GET_CHAR(cur) == 0xe5)
					continue;
				else
					return 0;	/* character mismatch */
			} else {
				if (PLB_GET_CHAR(cur) == dentry->name[pos])
					continue;
				else
					return 0;	/* character mismatch */
			}
		}
		if (!dot_processed) {
			dot_processed = true;
			pos = -1;
			if (PLB_GET_CHAR(cur) != '.')
				return 0;
			continue;
		}
		if (!ext_processed) {
			if ((pos == FAT_EXT_LEN - 1) ||
			    (dentry->ext[pos + 1] == FAT_PAD)) {
				/* this is the last character in ext */
				ext_processed = true;
			}
			if (dentry->ext[0] == FAT_PAD) {
				/* ext is empty; the match will fail */
				ext_processed = true;
			} else if (PLB_GET_CHAR(cur) == dentry->ext[pos]) {
				continue;
			} else {
				/* character mismatch */
				return 0;
			}
		}
		return 0;	/* extra characters in the component */
	}
	if (ext_processed || (name_processed && dentry->ext[0] == FAT_PAD))
		return cur - start;
	else
		return 0;
}

void fat_lookup(ipc_callid_t rid, ipc_call_t *request)
{
	int first = IPC_GET_ARG1(*request);
	int second = IPC_GET_ARG2(*request);
	int dev_handle = IPC_GET_ARG3(*request);

	
}

/**
 * @}
 */ 
