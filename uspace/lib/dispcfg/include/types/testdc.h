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
/** @file Display configuration test service types
 */

#ifndef _LIBDISPCFG_TYPES_TESTDC_H_
#define _LIBDISPCFG_TYPES_TESTDC_H_

#include <errno.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <types/common.h>
#include <types/dispcfg.h>

/** Describes to the server how to respond to our request and pass tracking
 * data back to the client.
 */
typedef struct {
	errno_t rc;
	sysarg_t seat_id;
	dispcfg_ev_t event;
	dispcfg_ev_t revent;
	int event_cnt;

	bool get_seat_list_called;
	dispcfg_seat_list_t *get_seat_list_rlist;

	bool get_seat_info_called;
	sysarg_t get_seat_info_seat_id;
	dispcfg_seat_info_t *get_seat_info_rinfo;

	bool seat_create_called;
	char *seat_create_name;
	sysarg_t seat_create_seat_id;

	bool seat_delete_called;
	sysarg_t seat_delete_seat_id;

	bool dev_assign_called;
	sysarg_t dev_assign_svc_id;
	sysarg_t dev_assign_seat_id;

	bool dev_unassign_called;
	sysarg_t dev_unassign_svc_id;

	bool get_asgn_dev_list_called;
	sysarg_t get_asgn_dev_list_seat_id;
	dispcfg_dev_list_t *get_asgn_dev_list_rlist;

	bool get_event_called;

	bool seat_added_called;
	sysarg_t seat_added_seat_id;

	bool seat_removed_called;
	sysarg_t seat_removed_seat_id;

	bool seat_changed_called;
	sysarg_t seat_changed_seat_id;

	fibril_condvar_t event_cv;
	fibril_mutex_t event_lock;
	dispcfg_srv_t *srv;
} test_response_t;

#endif

/** @}
 */
