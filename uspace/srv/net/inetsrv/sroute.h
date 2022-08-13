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

#ifndef INET_SROUTE_H_
#define INET_SROUTE_H_

#include <stddef.h>
#include <stdint.h>
#include "inetsrv.h"

extern inet_sroute_t *inet_sroute_new(void);
extern void inet_sroute_delete(inet_sroute_t *);
extern void inet_sroute_add(inet_sroute_t *);
extern void inet_sroute_remove(inet_sroute_t *);
extern inet_sroute_t *inet_sroute_find(inet_addr_t *);
extern inet_sroute_t *inet_sroute_find_by_name(const char *);
extern inet_sroute_t *inet_sroute_get_by_id(sysarg_t);
extern errno_t inet_sroute_send_dgram(inet_sroute_t *, inet_addr_t *,
    inet_dgram_t *, uint8_t, uint8_t, int);
extern errno_t inet_sroute_get_id_list(sysarg_t **, size_t *);

#endif

/** @}
 */
