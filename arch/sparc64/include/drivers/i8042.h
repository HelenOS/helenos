/*
 * Copyright (C) 2006 Jakub Jermar
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

 /** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#ifndef __sparc64_I8042_H__
#define __sparc64_I8042_H__

#include <arch/types.h>

#define KBD_PHYS_ADDRESS	0x1fff8904000ULL

#define STATUS_REG	4
#define COMMAND_REG	4
#define DATA_REG	6

#define LAST_REG	DATA_REG

extern volatile __u8 *kbd_virt_address;

static inline void i8042_data_write(__u8 data)
{
	kbd_virt_address[DATA_REG] = data;
}

static inline __u8 i8042_data_read(void)
{
	return kbd_virt_address[DATA_REG];
}

static inline __u8 i8042_status_read(void)
{
	return kbd_virt_address[STATUS_REG];
}

static inline void i8042_command_write(__u8 command)
{
	kbd_virt_address[COMMAND_REG] = command;
}

extern void kbd_init(void);

#endif

 /** @}
 */

