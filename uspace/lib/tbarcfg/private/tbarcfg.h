/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <sif.h>
#include <types/tbarcfg/tbarcfg.h>

/** Taskbar configuration */
struct tbarcfg {
	/** Repository session */
	sif_sess_t *repo;
	/** List of start menu entries (smenu_entry_t) */
	list_t entries;
	/** Entries SIF node */
	sif_node_t *nentries;
};

/** Start menu entry */
struct smenu_entry {
	/** Containing start menu */
	struct tbarcfg *smenu;
	/** Link to @c smenu->entries */
	link_t lentries;
	/** SIF node (persistent storage) */
	sif_node_t *nentry;
	/** Entry caption (with accelerator markup) */
	char *caption;
	/** Command to run */
	char *cmd;
};

extern errno_t smenu_entry_new(tbarcfg_t *, sif_node_t *, const char *,
    const char *, smenu_entry_t **);
extern void smenu_entry_delete(smenu_entry_t *);

#endif

/** @}
 */
