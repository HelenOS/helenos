/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libtbarcfg
 * @{
 */
/**
 * @file Taskbar configuration
 *
 */

#ifndef _TBARCFG_PRIVATE_TBARCFG_H
#define _TBARCFG_PRIVATE_TBARCFG_H

#include <adt/list.h>
#include <stdbool.h>
#include <types/tbarcfg/tbarcfg.h>

/** Taskbar configuration */
struct tbarcfg {
	/** Configuration file path */
	char *cfgpath;
	/** List of start menu entries (smenu_entry_t) */
	list_t entries;
};

/** Start menu entry */
struct smenu_entry {
	/** Containing start menu */
	struct tbarcfg *smenu;
	/** Link to @c smenu->entries */
	link_t lentries;
	/** Is this a separator entry */
	bool separator;
	/** Entry caption (with accelerator markup) */
	char *caption;
	/** Command to run */
	char *cmd;
	/** Start in terminal */
	bool terminal;
};

/** Taskbar configuration listener */
typedef struct tbarcfg_listener {
	/** Notification callback */
	void (*cb)(void *);
	/** Callback argument */
	void *arg;
} tbarcfg_listener_t;

#endif

/** @}
 */
