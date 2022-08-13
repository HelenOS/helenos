/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nterm
 * @{
 */
/**
 * @file
 */

#ifndef CONN_H
#define CONN_H

#include <stddef.h>

extern errno_t conn_open(const char *);
extern errno_t conn_send(void *, size_t);

#endif

/** @}
 */
