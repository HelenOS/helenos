/*
 * Copyright (c) 2008 Pavel Rimsky
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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */ 
/** @file
 * @brief	Niagara console keyboard port driver.
 */

#include <as.h>
#include <ddi.h>
#include <async.h>
#include <kbd.h>
#include <kbd_port.h>
#include <sysinfo.h>
#include <stdio.h>
#include <thread.h>
#include <bool.h>

#define POLL_INTERVAL		10000

static volatile bool polling_disabled = false;
//static void *niagara_thread_impl(void *arg);

/**
 * Initializes the SGCN driver.
 * Maps the physical memory (SRAM) and creates the polling thread. 
 */
int kbd_port_init(void)
{
	printf("****************** Niagara keyboard driver **********************\n");
	/*
	thread_id_t tid;
	int rc;

	rc = thread_create(niagara_thread_impl, NULL, "kbd_poll", &tid);
	if (rc != 0) {
		return rc;
	}
	*/
	return 0;
}

void kbd_port_yield(void)
{
	polling_disabled = true;
}

void kbd_port_reclaim(void)
{
	polling_disabled = false;
}

void kbd_port_write(uint8_t data)
{
	(void) data;
}

/**
 * Handler of the "key pressed" event. Reads codes of all the pressed keys from
 * the buffer. 
 */
static void niagara_key_pressed(void)
{
	printf("%s\n", "polling");
/*
	char c;
	
	uint32_t begin = SGCN_BUFFER_HEADER->in_begin;
	uint32_t end = SGCN_BUFFER_HEADER->in_end;
	uint32_t size = end - begin;
	
	volatile char *buf_ptr = (volatile char *)
		SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
	volatile uint32_t *in_wrptr_ptr = &(SGCN_BUFFER_HEADER->in_wrptr);
	volatile uint32_t *in_rdptr_ptr = &(SGCN_BUFFER_HEADER->in_rdptr);
	
	while (*in_rdptr_ptr != *in_wrptr_ptr) {
		c = *buf_ptr;
		*in_rdptr_ptr = (((*in_rdptr_ptr) - begin + 1) % size) + begin;
		buf_ptr = (volatile char *)
			SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
		kbd_push_scancode(c);
	}
*/
}

/**
 * Thread to poll SGCN for keypresses.
 */
/*
static void *niagara_thread_impl(void *arg)
{
	(void) arg;

	while (1) {
		if (polling_disabled == false)
			niagara_key_pressed();
		usleep(POLL_INTERVAL);
	}
	return 0;
}
*/
/** @}
 */
