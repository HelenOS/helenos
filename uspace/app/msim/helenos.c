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
#include "io/input.h"
#include "io/output.h"
#include "debug/gdb.h"
#include "cmd.h"
#include "fault.h"
#include "device/machine.h"
#include <str.h>
#include <malloc.h>

extern char *input_helenos_get_next_command(void);

void interactive_control(void)
{
	tobreak = false;

	if (reenter) {
		mprintf("\n");
		reenter = false;
	}

	stepping = 0;

	while (interactive) {
		char *commline = input_helenos_get_next_command();
		if (commline == NULL) {
			mprintf("Quit\n");
			input_back();
			exit(1);
		}

		/* Check for empty input. */
		if (str_cmp(commline, "") == 0) {
			interpret("step");
		} else {
			interpret(commline);
		}

		free(commline);
	}
}

bool gdb_remote_init(void)
{
	return false;
}

void gdb_session(void)
{
}

void gdb_handle_event(gdb_event_t event)
{
}

bool stdin_poll(char *key)
{
	// TODO: implement reading from keyboard
	return false;
}

