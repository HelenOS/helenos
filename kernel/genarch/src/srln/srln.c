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
 * @brief	Serial line processing.
 */

#include <genarch/srln/srln.h>
#include <console/chardev.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch.h>

chardev_t srlnin;
static chardev_t *srlnout;

static void srlnin_suspend(chardev_t *d)
{
}

static void srlnin_resume(chardev_t *d)
{
}

chardev_operations_t srlnin_ops = {
	.suspend = srlnin_suspend,
	.resume = srlnin_resume,
};

static void ksrln(void *arg)
{
	chardev_t *in = (chardev_t *) arg;
	uint8_t ch;

	while (1) {
		ch = _getc(in);
		
		if (ch == '\r')
			continue;
		
		chardev_push_character(srlnout, ch);
	}
}


void srln_init(chardev_t *devout)
{
	thread_t *t;

	chardev_initialize("srln", &srlnin, &srlnin_ops);
	srlnout = devout;
	
	t = thread_create(ksrln, &srlnin, TASK, 0, "ksrln", false);
	ASSERT(t);
	thread_ready(t);
}

/** @}
 */

