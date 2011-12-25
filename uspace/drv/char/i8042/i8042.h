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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */

/** @file
 * @brief i8042 port driver.
 */

#ifndef i8042_H_
#define i8042_H_

#include <sys/types.h>
#include <libarch/ddi.h>
#include <async.h>
#include <fibril_synch.h>
#include <ddf/driver.h>

/** i8042 HW I/O interface */
typedef struct {
	ioport8_t data;
	uint8_t pad[3];
	ioport8_t status;
} __attribute__ ((packed)) i8042_regs_t;

/** Softstate structure, one for each serial port (primary and aux). */
/*
typedef struct {
	service_id_t service_id;
	async_sess_t *client_sess;
} i8042_port_t;
*/

typedef struct i8042 i8042_t;

enum {
	DEVID_PRI = 0, /**< primary device */
        DEVID_AUX = 1, /**< AUX device */
	MAX_DEVS  = 2
};

struct i8042 {
	i8042_regs_t *regs;
//	i8042_port_t port[MAX_DEVS];
	ddf_fun_t *kbd_fun;
	ddf_fun_t *mouse_fun;
	fibril_mutex_t guard;
	fibril_condvar_t data_avail;
};

int i8042_init(i8042_t *, void *, size_t, int, int, ddf_dev_t *);
int i8042_write_kbd(i8042_t *, uint8_t);
int i8042_read_kbd(i8042_t *, uint8_t *);
int i8042_write_aux(i8042_t *, uint8_t);
int i8042_read_aux(i8042_t *, uint8_t *);

#endif
/**
 * @}
 */
