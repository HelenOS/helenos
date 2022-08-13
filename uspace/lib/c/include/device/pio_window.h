/*
 * SPDX-FileCopyrightText: 2013 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_DEVICE_PIO_WINDOW_H_
#define _LIBC_DEVICE_PIO_WINDOW_H_

#include <ipc/dev_iface.h>
#include <async.h>

/** PIO_WINDOW provider interface */
typedef enum {
	PIO_WINDOW_GET = 0,
} pio_window_method_t;

typedef struct {
	struct {
		uintptr_t base;
		size_t size;
	} mem, io;
} pio_window_t;

extern errno_t pio_window_get(async_sess_t *, pio_window_t *);

#endif

/** @}
 */
