/*
 * Copyright (c) 2025 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file File list
 */

#ifndef _UI_TYPES_FILELIST_H
#define _UI_TYPES_FILELIST_H

#include <loc.h>
#include <stdbool.h>
#include <stdint.h>

struct ui_file_list;
typedef struct ui_file_list ui_file_list_t;

struct ui_file_list_entry;
typedef struct ui_file_list_entry ui_file_list_entry_t;

/** File list entry attributes */
typedef struct ui_file_list_entry_attr {
	/** File name */
	const char *name;
	/** File size */
	uint64_t size;
	/** @c true iff entry is a directory */
	bool isdir;
	/** Service number for service special entries */
	service_id_t svc;
} ui_file_list_entry_attr_t;

/** File list callbacks */
typedef struct ui_file_list_cb {
	/** Request file list activation */
	void (*activate_req)(ui_file_list_t *, void *);
	/** File was selected */
	void (*selected)(ui_file_list_t *, void *, const char *);
} ui_file_list_cb_t;

#endif

/** @}
 */
