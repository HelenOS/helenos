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

#ifndef INET_ADDROBJ_H_
#define INET_ADDROBJ_H_

#include <stddef.h>
#include <stdint.h>
#include "inetsrv.h"

typedef enum {
	/* Find matching network address (using mask) */
	iaf_net,
	/* Find exact local address (not using mask) */
	iaf_addr
} inet_addrobj_find_t;

extern inet_addrobj_t *inet_addrobj_new(void);
extern void inet_addrobj_delete(inet_addrobj_t *);
extern errno_t inet_addrobj_add(inet_addrobj_t *);
extern void inet_addrobj_remove(inet_addrobj_t *);
extern inet_addrobj_t *inet_addrobj_find(inet_addr_t *, inet_addrobj_find_t);
extern inet_addrobj_t *inet_addrobj_find_by_name(const char *, inet_link_t *);
extern inet_addrobj_t *inet_addrobj_get_by_id(sysarg_t);
extern errno_t inet_addrobj_send_dgram(inet_addrobj_t *, inet_addr_t *,
    inet_dgram_t *, uint8_t, uint8_t, int);
extern errno_t inet_addrobj_get_id_list(sysarg_t **, size_t *);

#endif

/** @}
 */
