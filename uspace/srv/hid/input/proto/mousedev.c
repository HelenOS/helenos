/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
