/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup trace
 * @{
 */
/** @file
 */

#ifndef IPC_DESC_H_
#define IPC_DESC_H_

typedef struct {
	int number;
	const char *name;
} ipc_m_desc_t;

extern ipc_m_desc_t ipc_methods[];
extern size_t ipc_methods_len;

#endif

/** @}
 */
