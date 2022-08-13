/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file File list
 */

#ifndef _UI_TYPES_FILELIST_H
#define _UI_TYPES_FILELIST_H

struct ui_file_list;
typedef struct ui_file_list ui_file_list_t;

struct ui_file_list_entry;
typedef struct ui_file_list_entry ui_file_list_entry_t;

struct ui_file_list_entry_attr;
typedef struct ui_file_list_entry_attr ui_file_list_entry_attr_t;

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
