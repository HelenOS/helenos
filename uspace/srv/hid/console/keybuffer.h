/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup kbdgen
 * @brief HelenOS generic uspace keyboard handler.
 * @ingroup kbd
 * @{
 */
/** @file
 */

#ifndef __KEYBUFFER_H__
#define __KEYBUFFER_H__

#include <sys/types.h>
#include <io/console.h>
#include <bool.h>

/** Size of buffer for pressed keys */
#define KEYBUFFER_SIZE  128

typedef struct {
	console_event_t fifo[KEYBUFFER_SIZE];
	size_t head;
	size_t tail;
	size_t items;
} keybuffer_t;

extern void keybuffer_free(keybuffer_t *);
extern void keybuffer_init(keybuffer_t *);
extern size_t keybuffer_available(keybuffer_t *);
extern bool keybuffer_empty(keybuffer_t *);
extern void keybuffer_push(keybuffer_t *, const console_event_t *);
extern bool keybuffer_pop(keybuffer_t *, console_event_t *);

#endif

/**
 * @}
 */
