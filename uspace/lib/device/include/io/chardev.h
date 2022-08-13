/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libdevice
 * @{
 */

#ifndef LIBDEVICE_IO_CHARDEV_H
#define LIBDEVICE_IO_CHARDEV_H

#include <async.h>
#include <stddef.h>
#include <types/io/chardev.h>

typedef struct {
	async_sess_t *sess;
} chardev_t;

extern errno_t chardev_open(async_sess_t *, chardev_t **);
extern void chardev_close(chardev_t *);
extern errno_t chardev_read(chardev_t *, void *, size_t, size_t *,
    chardev_flags_t);
extern errno_t chardev_write(chardev_t *, const void *, size_t, size_t *);

#endif
/**
 * @}
 */
