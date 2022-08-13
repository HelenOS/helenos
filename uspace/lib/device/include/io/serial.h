/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_IO_SERIAL_H
#define LIBDEVICE_IO_SERIAL_H

#include <async.h>
#include <ipc/serial_ctl.h>

typedef struct {
	async_sess_t *sess;
} serial_t;

extern errno_t serial_open(async_sess_t *, serial_t **);
extern void serial_close(serial_t *);
extern errno_t serial_set_comm_props(serial_t *, unsigned, serial_parity_t,
    unsigned, unsigned);
extern errno_t serial_get_comm_props(serial_t *, unsigned *, serial_parity_t *,
    unsigned *, unsigned *);

#endif

/** @}
 */
