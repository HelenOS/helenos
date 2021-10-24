/*
 * Copyright (c) 2010 Lenka Trochtova
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

#ifndef LIBDEVICE_IPC_SERIAL_CTL_H
#define LIBDEVICE_IPC_SERIAL_CTL_H

#include <ipc/chardev.h>

/** IPC methods for getting/setting serial communication properties
 *
 * 1st IPC arg: baud rate
 * 2nd IPC arg: parity
 * 3rd IPC arg: number of bits in one word
 * 4th IPC arg: number of stop bits
 *
 */
typedef enum {
	SERIAL_GET_COM_PROPS = CHARDEV_LIMIT,
	SERIAL_SET_COM_PROPS
} serial_ctl_t;

typedef enum {
	SERIAL_NO_PARITY = 0,
	SERIAL_ODD_PARITY = 1,
	SERIAL_EVEN_PARITY = 3,
	SERIAL_MARK_PARITY = 5,
	SERIAL_SPACE_PARITY = 7
} serial_parity_t;

#endif
