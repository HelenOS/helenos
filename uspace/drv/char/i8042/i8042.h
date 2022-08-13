/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup i8042
 * @{
 */

/** @file
 * @brief i8042 port driver.
 */

#ifndef i8042_H_
#define i8042_H_

#include <adt/circ_buf.h>
#include <io/chardev_srv.h>
#include <ddi.h>
#include <fibril_synch.h>
#include <ddf/driver.h>

#define NAME  "i8042"

/** Buffer needs to be large enough for rate at which keyboard or mouse
 * produces data (mouse produces data at faster rate).
 */
#define BUFFER_SIZE  64

/** i8042 HW I/O interface */
typedef struct {
	ioport8_t data;
	uint8_t pad[3];
	ioport8_t status;
} __attribute__((packed)) i8042_regs_t;

/** i8042 Port. */
typedef struct {
	/** Controller */
	struct i8042 *ctl;
	/** Device function */
	ddf_fun_t *fun;
	/** Character device server data */
	chardev_srvs_t cds;
	/** Circular buffer */
	circ_buf_t cbuf;
	/** Buffer data space */
	uint8_t buf_data[BUFFER_SIZE];
	/** Protect buffer */
	fibril_mutex_t buf_lock;
	/** Signal new data in buffer */
	fibril_condvar_t buf_cv;
	/** Interrupt number */
	int irq;
} i8042_port_t;

/** i8042 Controller. */
typedef struct i8042 {
	/** I/O registers. */
	i8042_regs_t *regs;
	/** Keyboard port */
	i8042_port_t *kbd;
	/** AUX port */
	i8042_port_t *aux;
	/** Prevents simultanous port writes. */
	fibril_mutex_t write_guard;
} i8042_t;

extern errno_t i8042_init(i8042_t *, addr_range_t *, int, int, ddf_dev_t *);

#endif

/**
 * @}
 */
