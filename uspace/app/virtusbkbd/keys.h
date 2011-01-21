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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Keyboard keys related structures.
 */
#ifndef VUK_KEYS_H_
#define VUK_KEYS_H_

#include <sys/types.h>
#include <bool.h>

/** Maximum number of keys that can be pressed simultaneously. */
#define KB_MAX_KEYS_AT_ONCE 6

/** Key code type. */
typedef uint8_t kb_key_code_t;

#define USB_HIDUT_KBD_KEY(name, usage_id, l, lc, l1, l2) \
	KB_KEY_##name = usage_id,
/** USB key code. */
typedef enum {
	#include <usb/classes/hidutkbd.h>
} key_code_t;

/** Modifier type. */
typedef uint8_t kb_modifier_t;
#define _KB_MOD(shift) (1 << (shift))
#define KB_MOD_LEFT_CTRL _KB_MOD(0)
#define KB_MOD_LEFT_SHIFT _KB_MOD(1)
#define KB_MOD_LEFT_ALT _KB_MOD(2)
#define KB_MOD_LEFT_GUI _KB_MOD(3)
#define KB_MOD_RIGHT_CTRL _KB_MOD(4)
#define KB_MOD_RIGHT_SHIFT _KB_MOD(5)
#define KB_MOD_RIGHT_ALT _KB_MOD(6)
#define KB_MOD_RIGHT_GUI _KB_MOD(7)

/** Base key action. */
typedef enum {
	KB_KEY_DOWN,
	KB_KEY_UP
} kb_key_action_t;

/** Keyboard status. */
typedef struct {
	/** Bitmap of pressed modifiers. */
	kb_modifier_t modifiers;
	/** Array of currently pressed keys. */
	kb_key_code_t pressed_keys[KB_MAX_KEYS_AT_ONCE];
} kb_status_t;

/** Callback type for status change. */
typedef void (*kb_on_status_change)(kb_status_t *);


void kb_init(kb_status_t *);
void kb_change_modifier(kb_status_t *, kb_key_action_t, kb_modifier_t);
void kb_change_key(kb_status_t *, kb_key_action_t, kb_key_code_t);

/** Keyboard event.
 * Use macros M_DOWN, M_UP, K_DOWN, K_UP, K_PRESS to generate list
 * of them.
 */
typedef struct {
	/** Key action. */
	kb_key_action_t action;
	/** Switch whether action is about modifier or normal key. */
	bool normal_key;
	/** Modifier change. */
	kb_modifier_t modifier_change;
	/** Normal key change. */
	kb_key_code_t key_change;
} kb_event_t;

#define M_DOWN(mod) \
	{ KB_KEY_DOWN, false, (mod), 0 }
#define M_UP(mod) \
	{ KB_KEY_UP, false, (mod), 0 }
#define K_DOWN(key) \
	{ KB_KEY_DOWN, true, 0, (key) }
#define K_UP(key) \
	{ KB_KEY_UP, true, 0, (key) }
#define K_PRESS(key) \
	K_DOWN(key), K_UP(key)


void kb_process_events(kb_status_t *, kb_event_t *, size_t, kb_on_status_change);


#endif
/**
 * @}
 */
