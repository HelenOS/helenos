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
 * @file Display configuration utility (UI) types
 */

#ifndef TYPES_DISPLAY_CFG_H
#define TYPES_DISPLAY_CFG_H

#include <dispcfg.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/tabset.h>
#include <ui/ui.h>
#include <ui/window.h>

/** Display configuration utility (UI) */
typedef struct display_cfg {
	/** Display configuration session */
	dispcfg_t *dispcfg;
	/** UI */
	ui_t *ui;
	/** Containing window */
	ui_window_t *window;
	/** Fixed layout */
	ui_fixed_t *fixed;
	/** Tab set */
	ui_tab_set_t *tabset;
	/** Seat configuration tab */
	struct dcfg_seats *seats;
} display_cfg_t;

#endif

/** @}
 */
