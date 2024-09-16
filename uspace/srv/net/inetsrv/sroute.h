/*
 * Copyright (c) 2024 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <sif.h>
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
extern errno_t inet_sroutes_load(sif_node_t *);
extern errno_t inet_sroutes_save(sif_node_t *);

#endif

/** @}
 */
