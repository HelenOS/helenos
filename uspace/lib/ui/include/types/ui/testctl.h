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

/** @addtogroup libui
 * @{
 */
/**
 * @file Label
 */

#ifndef _UI_TYPES_TESTCTL_H
#define _UI_TYPES_TESTCTL_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/event.h>

struct ui_test_ctl;
typedef struct ui_test_ctl ui_test_ctl_t;

/** Test control response */
typedef struct {
	/** Claim to return */
	ui_evclaim_t claim;
	/** Result code to return */
	errno_t rc;

	/** @c true iff destroy was called */
	bool destroy;

	/** @c true iff paint was called */
	bool paint;

	/** @c true iff kbd_event was called */
	bool kbd;
	/** Keyboard event that was sent */
	kbd_event_t kevent;

	/** @c true iff pos_event was called */
	bool pos;
	/** Position event that was sent */
	pos_event_t pevent;

	/** @c true iff unfocus was called */
	bool unfocus;
	/** Number of remaining foci that was sent */
	unsigned unfocus_nfocus;
} ui_tc_resp_t;

#endif

/** @}
 */
