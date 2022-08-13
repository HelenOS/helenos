/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef INETPING_H_
#define INETPING_H_

#include <types/inetping.h>
#include "inetsrv.h"

extern void inetping_conn(ipc_call_t *, void *);
extern errno_t inetping_recv(uint16_t, inetping_sdu_t *);

#endif

/** @}
 */
