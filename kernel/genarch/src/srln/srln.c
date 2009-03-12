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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Serial line processing.
 */

#include <genarch/srln/srln.h>
#include <console/chardev.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch.h>

static indev_t srlnout;

indev_operations_t srlnout_ops = {
	.poll = NULL
};

static void ksrln(void *arg)
{
	indev_t *in = (indev_t *) arg;
	bool cr = false;
	
	while (true) {
		uint8_t ch = _getc(in);
		
		if ((ch == '\n') && (cr)) {
			cr = false;
			continue;
		}
		
		if (ch == '\r') {
			ch = '\n';
			cr = true;
		} else
			cr = false;
		
		if (ch == 0x7f)
			ch = '\b';
		
		indev_push_character(stdin, ch);
	}
}

void srln_init(indev_t *devin)
{
	indev_initialize("srln", &srlnout, &srlnout_ops);
	thread_t *thread
	    = thread_create(ksrln, devin, TASK, 0, "ksrln", false);
	
	if (thread) {
		stdin = &srlnout;
		thread_ready(thread);
	}
}

/** @}
 */
