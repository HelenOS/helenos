/*
 * Copyright (c) 2012 Vojtech Horky
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

/** @addtogroup msim
 * @{
 */
/** @file HelenOS specific functions for MSIM simulator.
 */
#include "../../io/input.h"
#include "../../io/output.h"
#include "../../fault.h"
#include "helenos.h"
#include <tinput.h>
#include <errno.h>

static tinput_t *input_prompt;

/** Terminal and readline initialization
 *
 */
void input_init(void)
{
	input_prompt = tinput_new();
	if (input_prompt == NULL) {
		die(1, "Failed to intialize input.");
	}
	helenos_dprinter_init();
}

void input_inter(void)
{
}

void input_shadow( void)
{
}

void input_back( void)
{
}

char *helenos_input_get_next_command(void)
{
	tinput_set_prompt(input_prompt, "[msim] ");

	char *commline = NULL;
	int rc = tinput_read(input_prompt, &commline);

	if (rc == ENOENT) {
		rc = asprintf(&commline, "quit");
		mprintf("Quit\n");
		if (rc != EOK) {
			exit(1);
		}
	}

	/* On error, it remains NULL. */
	return commline;
}


bool stdin_poll(char *key)
{
	kbd_event_t ev;
	suseconds_t timeout = 0;
	errno = EOK;
	console_flush(input_prompt->console);
	bool has_input = console_get_kbd_event_timeout(input_prompt->console, &ev, &timeout);
	if (!has_input) {
		return false;
	}

	if (ev.type != KEY_PRESS)
		return false;

	*key = ev.c;

	return true;
}

