/*
 * SPDX-FileCopyrightText: 2018 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libuntar
 * @{
 */
/** @file
 */

#ifndef UNTAR_H_
#define UNTAR_H_

#include <stdlib.h>
#include <stdarg.h>

typedef struct tar_file {
	void *data;

	int (*open)(struct tar_file *);
	void (*close)(struct tar_file *);

	size_t (*read)(struct tar_file *, void *, size_t);
	void (*vreport)(struct tar_file *, const char *, va_list);
} tar_file_t;

extern int untar(tar_file_t *);

#endif

/** @}
 */
