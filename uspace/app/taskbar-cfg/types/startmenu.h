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

/** @addtogroup taskbar-cfg
 * @{
 */
/**
 * @file Start menu configuration tab
 */

#ifndef TYPES_STARTMENU_H
#define TYPES_STARTMENU_H

#include <tbarcfg/tbarcfg.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/list.h>
#include <ui/pbutton.h>
#include <ui/tab.h>

/** Start menu configuration tab */
typedef struct startmenu {
	/** Containing taskbar configuration */
	struct taskbar_cfg *tbarcfg;
	/** UI tab */
	ui_tab_t *tab;
	/** Fixed layout */
	ui_fixed_t *fixed;
	/** 'Start menu entries' label */
	ui_label_t *entries_label;
	/** List of start menu entries */
	ui_list_t *entries_list;
	/** New entry button */
	ui_pbutton_t *new_entry;
	/** Delete entry button */
	ui_pbutton_t *delete_entry;
	/** Edit entry button */
	ui_pbutton_t *edit_entry;
	/** Separator entry button */
	ui_pbutton_t *sep_entry;
	/** Move entry up button */
	ui_pbutton_t *up_entry;
	/** Move entry down button */
	ui_pbutton_t *down_entry;
} startmenu_t;

/** Start menu entry */
typedef struct startmenu_entry {
	/** Containing start menu configuration tab */
	struct startmenu *startmenu;
	/** Backing entry */
	smenu_entry_t *entry;
	/** List entry */
	ui_list_entry_t *lentry;
} startmenu_entry_t;

#endif

/** @}
 */
