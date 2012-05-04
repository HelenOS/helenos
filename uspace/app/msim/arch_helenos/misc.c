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
#include "../../device/dprinter.h"
#include "../../debug/gdb.h"
#include "../../cmd.h"
#include "../../fault.h"
#include "../../device/machine.h"
#include "helenos.h"
#include <str.h>
#include <malloc.h>
#include <ctype.h>
#include <stdio.h>

/* Define when the dprinter device shall try to filter
 * out ANSI escape sequences.
 */
#define IGNORE_ANSI_ESCAPE_SEQUENCES
/* Define when you want the ANSI escape sequences to be dumped as
 * hex numbers.
 */
// #define DUMP_ANSI_ESCAPE_SEQUENCES

void interactive_control(void)
{
	tobreak = false;

	if (reenter) {
		mprintf("\n");
		reenter = false;
	}

	stepping = 0;

	while (interactive) {
		char *commline = helenos_input_get_next_command();
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


static void (*original_printer_write)(cpu_t *, device_s *, ptr_t, uint32_t);
static void helenos_printer_write(cpu_t *cpu, device_s *dev, ptr_t addr, uint32_t val)
{
#ifdef IGNORE_ANSI_ESCAPE_SEQUENCES
	static bool inside_ansi_escape = false;
	static bool just_ended_ansi_escape = false;

	if (inside_ansi_escape) {
#ifdef DUMP_ANSI_ESCAPE_SEQUENCES
		fprintf(stderr, "%02" PRIx32 "'%c' ", val, val >= 32 ? val : '?');
#endif
		if (isalpha((int) val)) {
			just_ended_ansi_escape = true;
			inside_ansi_escape = false;
#ifdef DUMP_ANSI_ESCAPE_SEQUENCES
			fprintf(stderr, " [END]\n");
#endif
		}

		return;
	}

	if (val == 0x1B) {
		inside_ansi_escape = true;

		if (!just_ended_ansi_escape) {
#ifdef DUMP_ANSI_ESCAPE_SEQUENCES
			fprintf(stderr, "\n");
#endif
		}
#ifdef DUMP_ANSI_ESCAPE_SEQUENCES
		fprintf(stderr, "ESC sequence: ");
#endif

		return;
	}

	just_ended_ansi_escape = false;
#endif

	(*original_printer_write)(cpu, dev, addr, val);
}

void helenos_dprinter_init(void)
{
	original_printer_write = dprinter.write;
	dprinter.write = helenos_printer_write;
}


