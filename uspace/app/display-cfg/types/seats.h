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

/** @addtogroup display-cfg
 * @{
 */
/**
 * @file Seat configuration tab
 */

#ifndef TYPES_SEATS_H
#define TYPES_SEATS_H

#include <loc.h>
#include <types/common.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/list.h>
#include <ui/pbutton.h>
#include <ui/promptdialog.h>
#include <ui/selectdialog.h>
#include <ui/tab.h>

/** Seat configuration tab */
typedef struct dcfg_seats {
	/** Containing display configuration */
	struct display_cfg *dcfg;
	/** UI tab */
	ui_tab_t *tab;
	/** Fixed layout */
	ui_fixed_t *fixed;
	/** 'Configured seats' label */
	ui_label_t *seats_label;
	/** List of configured seats */
	ui_list_t *seat_list;
	/** Add seat button */
	ui_pbutton_t *add_seat;
	/** Remove seat button */
	ui_pbutton_t *remove_seat;
	/** Add seat dialog */
	ui_prompt_dialog_t *add_seat_dlg;
	/** 'Devices assigned to xxx' label */
	ui_label_t *devices_label;
	/** List of assigned devices */
	ui_list_t *device_list;
	/** Add device button */
	ui_pbutton_t *add_device;
	/** Remove device button */
	ui_pbutton_t *remove_device;
	/** Add device dialog */
	ui_select_dialog_t *add_device_dlg;
} dcfg_seats_t;

/** Entry in seat list */
typedef struct {
	/** Containing seat configuration tab */
	struct dcfg_seats *seats;
	/** List entry */
	ui_list_entry_t *lentry;
	/** Seat ID */
	sysarg_t seat_id;
	/** Seat name */
	char *name;
} dcfg_seats_entry_t;

/** Entry in device list */
typedef struct {
	/** Containing seat configuration tab */
	struct dcfg_seats *seats;
	/** List entry */
	ui_list_entry_t *lentry;
	/** Service ID */
	service_id_t svc_id;
	/** Device name */
	char *name;
} dcfg_devices_entry_t;

#endif

/** @}
 */
