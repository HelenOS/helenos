/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup launcher
 * @{
 */
/**
 * @file Launcher
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <display.h>
#include <ui/fixed.h>
#include <ui/image.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/window.h>

/** Launcher */
typedef struct {
	ui_t *ui;
	ui_window_t *window;
	ui_fixed_t *fixed;
	ui_image_t *image;
	ui_label_t *label;
	ui_pbutton_t *pb1;
	ui_pbutton_t *pb2;
	ui_pbutton_t *pb3;
	ui_pbutton_t *pb4;
	ui_pbutton_t *pb5;
	ui_pbutton_t *pb6;
} launcher_t;

#endif

/** @}
 */
