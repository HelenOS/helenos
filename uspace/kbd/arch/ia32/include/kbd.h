/*
 * Copyright (C) 2006 Josef Cejka
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

/** @addtogroup kbdamd64 amd64
 * @brief	HelenOS ia32 / amd64 arch dependent parts of uspace keyboard and mouse handler.
 * @ingroup  kbd
 * @{
 */

/** @file
 * @ingroup kbdia32
 */

#ifndef __ia32_KBD_H__
#define __ia32_KBD_H__

#include <key_buffer.h>
#include <ddi.h>
#include <libarch/ddi.h>

#define KBD_IRQ      1
#define MOUSE_IRQ    12

#define i8042_DATA      0x60
#define i8042_STATUS    0X64


typedef unsigned char u8;
typedef short u16;

static inline void i8042_data_write(u8 data)
{
	outb(i8042_DATA, data);
}

static inline u8 i8042_data_read(void)
{
	return inb(i8042_DATA);
}

static inline u8 i8042_status_read(void)
{
	return inb(i8042_STATUS);
}

static inline void i8042_command_write(u8 command)
{
	outb(i8042_STATUS, command);
}

int kbd_arch_init(void);

#endif

/**
 * @}
 */ 

