/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hello
 * @{
 */
/**
 * @file Hello world (in UI)
 */

#ifndef HELLO_H
#define HELLO_H

#include <display.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/ui.h>
#include <ui/window.h>

/** Hello world UI application */
typedef struct {
	ui_t *ui;
	ui_window_t *window;
	ui_fixed_t *fixed;
	ui_label_t *label;
} hello_t;

#endif

/** @}
 */
