/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usbvirtkbd
 * @{
 */
/**
 * @file
 * @brief Keyboard keys related structures.
 */
#include "keys.h"

/** Initializes keyboard status. */
void kb_init(kb_status_t *status)
{
	status->modifiers = 0;
	size_t i;
	for (i = 0; i < KB_MAX_KEYS_AT_ONCE; i++) {
		status->pressed_keys[i] = 0;
	}
}

/** Change pressed modifiers. */
void kb_change_modifier(kb_status_t *status, kb_key_action_t action,
    kb_modifier_t modifier)
{
	if (action == KB_KEY_DOWN) {
		status->modifiers = status->modifiers | modifier;
	} else {
		status->modifiers = status->modifiers & (~modifier);
	}
}

/** Find index of given key in key code array.
 * @retval (size_t)-1 Key not found.
 */
static size_t find_key_index(kb_key_code_t *keys, size_t size, kb_key_code_t key)
{
	size_t i;
	for (i = 0; i < size; i++) {
		if (keys[i] == key) {
			return i;
		}
	}
	return (size_t)-1;
}

/** Change pressed key. */
void kb_change_key(kb_status_t *status, kb_key_action_t action,
    kb_key_code_t key_code)
{
	size_t pos = find_key_index(status->pressed_keys, KB_MAX_KEYS_AT_ONCE,
	    key_code);
	if (action == KB_KEY_DOWN) {
		if (pos != (size_t)-1) {
			return;
		}
		/*
		 * Find first free item in the array.
		 */
		size_t i;
		for (i = 0; i < KB_MAX_KEYS_AT_ONCE; i++) {
			if (status->pressed_keys[i] == 0) {
				status->pressed_keys[i] = key_code;
				return;
			}
		}
		// TODO - handle buffer overflow
	} else {
		if (pos == (size_t)-1) {
			return;
		}
		status->pressed_keys[pos] = 0;
	}	
}

/** Process list of events. */
void kb_process_events(kb_status_t *status,
    kb_event_t *events, size_t count,
    kb_on_status_change on_change)
{
	while (count-- > 0) {
		if (events->normal_key) {
			kb_change_key(status, events->action,
			    events->key_change);
		} else {
			kb_change_modifier(status, events->action,
			    events->modifier_change);
		}
		if (on_change) {
			on_change(status);
		}
		events++;
	}
}

/** @}
 */
