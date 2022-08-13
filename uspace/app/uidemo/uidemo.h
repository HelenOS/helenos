/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uidemo
 * @{
 */
/**
 * @file User interface demo
 */

#ifndef UIDEMO_H
#define UIDEMO_H

#include <display.h>
#include <ui/checkbox.h>
#include <ui/entry.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/pbutton.h>
#include <ui/rbutton.h>
#include <ui/scrollbar.h>
#include <ui/slider.h>
#include <ui/ui.h>
#include <ui/window.h>

/** User interface demo */
typedef struct {
	ui_t *ui;
	ui_window_t *window;
	ui_fixed_t *fixed;
	ui_menu_bar_t *mbar;
	ui_menu_t *mfile;
	ui_menu_t *medit;
	ui_menu_t *mpreferences;
	ui_menu_t *mhelp;
	ui_entry_t *entry;
	ui_image_t *image;
	ui_label_t *label;
	ui_pbutton_t *pb1;
	ui_pbutton_t *pb2;
	ui_checkbox_t *checkbox;
	ui_rbutton_group_t *rbgroup;
	ui_rbutton_t *rbleft;
	ui_rbutton_t *rbcenter;
	ui_rbutton_t *rbright;
	ui_slider_t *slider;
	ui_scrollbar_t *hscrollbar;
	ui_scrollbar_t *vscrollbar;
} ui_demo_t;

#endif

/** @}
 */
