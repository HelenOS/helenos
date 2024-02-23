/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Serial line processing.
 */

#include <assert.h>
#include <genarch/srln/srln.h>
#include <console/chardev.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch.h>
#include <str.h>
#include <stdlib.h>

static indev_operations_t srln_raw_ops = {
	.poll = NULL,
	.signal = NULL
};

static void ksrln(void *arg)
{
	srln_instance_t *instance = (srln_instance_t *) arg;
	bool cr = false;
	uint32_t escape = 0;

	while (true) {
		char32_t ch = indev_pop_character(&instance->raw);

		/* ANSI escape sequence processing */
		if (escape != 0) {
			escape <<= 8;
			escape |= ch & 0xff;

			if ((escape == 0x1b4f) || (escape == 0x1b5b) || (escape == 0x1b5b33))
				continue;

			switch (escape) {
			case 0x1b4f46:
			case 0x1b5b46:
				ch = U_END_ARROW;
				escape = 0;
				break;
			case 0x1b4f48:
			case 0x1b5b48:
				ch = U_HOME_ARROW;
				escape = 0;
				break;
			case 0x1b5b41:
				ch = U_UP_ARROW;
				escape = 0;
				break;
			case 0x1b5b42:
				ch = U_DOWN_ARROW;
				escape = 0;
				break;
			case 0x1b5b43:
				ch = U_RIGHT_ARROW;
				escape = 0;
				break;
			case 0x1b5b44:
				ch = U_LEFT_ARROW;
				escape = 0;
				break;
			case 0x1b5b337e:
				ch = U_DELETE;
				escape = 0;
				break;
			default:
				escape = 0;
			}
		}

		if (ch == 0x1b) {
			escape = ch & 0xff;
			continue;
		}

		/*
		 * Replace carriage return with line feed
		 * and suppress any following line feed
		 */
		if ((ch == '\n') && (cr)) {
			cr = false;
			continue;
		}

		if (ch == '\r') {
			ch = '\n';
			cr = true;
		} else
			cr = false;

		/* Backspace */
		if (ch == 0x7f)
			ch = '\b';

		indev_push_character(instance->sink, ch);
	}
}

srln_instance_t *srln_init(void)
{
	srln_instance_t *instance =
	    malloc(sizeof(srln_instance_t));
	if (instance) {
		instance->thread = thread_create(ksrln, (void *) instance,
		    TASK, THREAD_FLAG_NONE, "ksrln");

		if (!instance->thread) {
			free(instance);
			return NULL;
		}

		instance->sink = NULL;
		indev_initialize("srln", &instance->raw, &srln_raw_ops);
	}

	return instance;
}

indev_t *srln_wire(srln_instance_t *instance, indev_t *sink)
{
	assert(instance);
	assert(sink);

	instance->sink = sink;
	thread_start(instance->thread);

	return &instance->raw;
}

/** @}
 */
