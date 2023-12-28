/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libdispcfg
 * @{
 */
/** @file Display configuration protocol types
 */

#ifndef _LIBDISPCFG_TYPES_DISPCFG_H_
#define _LIBDISPCFG_TYPES_DISPCFG_H_

#include <ipc/services.h>
#include <types/common.h>

/** Use the default display configuration service (argument to dispcfg_open() */
#define DISPCFG_DEFAULT SERVICE_NAME_DISPCFG

struct dispcfg;

/** Display configuration session */
typedef struct dispcfg dispcfg_t;

/** Display configuration callbacks */
typedef struct {
	/** Seat added */
	void (*seat_added)(void *, sysarg_t);
	/** Seat removed */
	void (*seat_removed)(void *, sysarg_t);
} dispcfg_cb_t;

/** Display configuration event type */
typedef enum {
	/** Seat added */
	dcev_seat_added,
	/** Seat removed */
	dcev_seat_removed,
} dispcfg_ev_type_t;

/** Display configuration event */
typedef struct {
	/** Event type */
	dispcfg_ev_type_t etype;
	/** Seat ID */
	sysarg_t seat_id;
} dispcfg_ev_t;

/** Seat list */
typedef struct {
	/** Number of seats */
	size_t nseats;
	/** ID for each seat */
	sysarg_t *seats;
} dispcfg_seat_list_t;

/** Seat information */
typedef struct {
	/** Seat name */
	char *name;
} dispcfg_seat_info_t;

/** Assigned device list */
typedef struct {
	/** Number of devices */
	size_t ndevs;
	/** ID for each device */
	sysarg_t *devs;
} dispcfg_dev_list_t;

#endif

/** @}
 */
