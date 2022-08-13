/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup netecho
 * @{
 */
/**
 * @file
 */

#ifndef COMM_H
#define COMM_H

#include <stddef.h>

extern errno_t comm_open_listen(const char *);
extern errno_t comm_open_talkto(const char *);
extern void comm_close(void);
extern errno_t comm_send(void *, size_t);

#endif

/** @}
 */
