/*
 * Copyright (c) 2021 Jiri Svoboda
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup mouse_proto
 * @ingroup input
 * @{
 */
/**
 * @file
 * @brief Mouse device connector controller driver.
 */

#include <stdio.h>
#include <vfs/vfs_sess.h>
#include <async.h>
#include <errno.h>
#include <ipc/mouseev.h>
#include <loc.h>
#include <stdlib.h>
#include <time.h>
#include "../mouse.h"
#include "../mouse_port.h"
#include "../mouse_proto.h"
#include "../input.h"

enum {
	/** Default double-click speed in milliseconds */
	dclick_delay_ms = 500
};

/** Mousedev softstate */
typedef struct {
	/** Link to generic mouse device */
	mouse_dev_t *mouse_dev;
	/** Button number of last button pressed (or -1 if none) */
	int press_bnum;
	/** Time at which button was last pressed */
	struct timespec press_time;
} mousedev_t;

static mousedev_t *mousedev_new(mouse_dev_t *mdev)
{
	mousedev_t *mousedev = calloc(1, sizeof(mousedev_t));
	if (mousedev == NULL)
		return NULL;

	mousedev->mouse_dev = mdev;
	mousedev->press_bnum = -1;

	return mousedev;
}

static void mousedev_destroy(mousedev_t *mousedev)
{
	free(mousedev);
}

static void mousedev_press(mousedev_t *mousedev, int bnum)
{
	struct timespec now;
	nsec_t ms_delay;

	getuptime(&now);

	/* Same button was pressed previously */
	if (mousedev->press_bnum == bnum) {
		/* Compute milliseconds since previous press */
		ms_delay = ts_sub_diff(&now, &mousedev->press_time) / 1000000;

		if (ms_delay <= dclick_delay_ms) {
			mouse_push_event_dclick(mousedev->mouse_dev, bnum);
			mousedev->press_bnum = -1;
			return;
		}
	}

	/* Record which button was last pressed and at what time */
	mousedev->press_bnum = bnum;
	mousedev->press_time = now;
}

static void mousedev_callback_conn(ipc_call_t *icall, void *arg)
{
	/* Mousedev device structure */
	mousedev_t *mousedev = (mousedev_t *) arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			mousedev_destroy(mousedev);
			return;
		}

		errno_t retval;

		switch (ipc_get_imethod(&call)) {
		case MOUSEEV_MOVE_EVENT:
			mouse_push_event_move(mousedev->mouse_dev,
			    ipc_get_arg1(&call), ipc_get_arg2(&call),
			    ipc_get_arg3(&call));
			retval = EOK;
			break;
		case MOUSEEV_ABS_MOVE_EVENT:
			mouse_push_event_abs_move(mousedev->mouse_dev,
			    ipc_get_arg1(&call), ipc_get_arg2(&call),
			    ipc_get_arg3(&call), ipc_get_arg4(&call));
			retval = EOK;
			break;
		case MOUSEEV_BUTTON_EVENT:
			mouse_push_event_button(mousedev->mouse_dev,
			    ipc_get_arg1(&call), ipc_get_arg2(&call));
			if (ipc_get_arg2(&call) != 0)
				mousedev_press(mousedev, ipc_get_arg1(&call));
			retval = EOK;
			break;
		default:
			retval = ENOTSUP;
			break;
		}

		async_answer_0(&call, retval);
	}
}

static errno_t mousedev_proto_init(mouse_dev_t *mdev)
{
	async_sess_t *sess = loc_service_connect(mdev->svc_id, INTERFACE_DDF, 0);
	if (sess == NULL) {
		printf("%s: Failed starting session with '%s'\n", NAME,
		    mdev->svc_name);
		return ENOENT;
	}

	mousedev_t *mousedev = mousedev_new(mdev);
	if (mousedev == NULL) {
		printf("%s: Failed allocating device structure for '%s'.\n",
		    NAME, mdev->svc_name);
		async_hangup(sess);
		return ENOMEM;
	}

	async_exch_t *exch = async_exchange_begin(sess);
	if (exch == NULL) {
		printf("%s: Failed starting exchange with '%s'.\n", NAME,
		    mdev->svc_name);
		mousedev_destroy(mousedev);
		async_hangup(sess);
		return ENOENT;
	}

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_MOUSE_CB, 0, 0,
	    mousedev_callback_conn, mousedev, &port);

	async_exchange_end(exch);
	async_hangup(sess);

	if (rc != EOK) {
		printf("%s: Failed creating callback connection from '%s'.\n",
		    NAME, mdev->svc_name);
		mousedev_destroy(mousedev);
		return rc;
	}

	return EOK;
}

mouse_proto_ops_t mousedev_proto = {
	.init = mousedev_proto_init
};

/**
 * @}
 */
