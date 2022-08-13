/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kbd_ctl
 * @ingroup input
 * @{
 */
/**
 * @file
 * @brief Keyboard device connector controller driver.
 */

#include <async.h>
#include <stdbool.h>
#include <errno.h>
#include <io/console.h>
#include <io/keycode.h>
#include <ipc/kbdev.h>
#include <loc.h>
#include <stdlib.h>
#include <vfs/vfs_sess.h>
#include "../gsp.h"
#include "../input.h"
#include "../kbd.h"
#include "../kbd_ctl.h"
#include "../kbd_port.h"

static errno_t kbdev_ctl_init(kbd_dev_t *);
static void kbdev_ctl_set_ind(kbd_dev_t *, unsigned int);

static void kbdev_callback_conn(ipc_call_t *, void *arg);

kbd_ctl_ops_t kbdev_ctl = {
	.parse = NULL,
	.init = kbdev_ctl_init,
	.set_ind = kbdev_ctl_set_ind
};

/** Kbdev softstate */
typedef struct {
	/** Link to generic keyboard device */
	kbd_dev_t *kbd_dev;

	/** Session with kbdev device */
	async_sess_t *sess;
} kbdev_t;

static kbdev_t *kbdev_new(kbd_dev_t *kdev)
{
	kbdev_t *kbdev = calloc(1, sizeof(kbdev_t));
	if (kbdev == NULL)
		return NULL;

	kbdev->kbd_dev = kdev;

	return kbdev;
}

static void kbdev_destroy(kbdev_t *kbdev)
{
	if (kbdev->sess != NULL)
		async_hangup(kbdev->sess);

	free(kbdev);
}

static errno_t kbdev_ctl_init(kbd_dev_t *kdev)
{
	async_sess_t *sess = loc_service_connect(kdev->svc_id,
	    INTERFACE_DDF, 0);
	if (sess == NULL) {
		printf("%s: Failed starting session with '%s.'\n", NAME,
		    kdev->svc_name);
		return ENOENT;
	}

	kbdev_t *kbdev = kbdev_new(kdev);
	if (kbdev == NULL) {
		printf("%s: Failed allocating device structure for '%s'.\n",
		    NAME, kdev->svc_name);
		async_hangup(sess);
		return ENOMEM;
	}

	kbdev->sess = sess;

	async_exch_t *exch = async_exchange_begin(sess);
	if (exch == NULL) {
		printf("%s: Failed starting exchange with '%s'.\n", NAME,
		    kdev->svc_name);
		kbdev_destroy(kbdev);
		return ENOENT;
	}

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_KBD_CB, 0, 0,
	    kbdev_callback_conn, kbdev, &port);

	if (rc != EOK) {
		printf("%s: Failed creating callback connection from '%s'.\n",
		    NAME, kdev->svc_name);
		async_exchange_end(exch);
		kbdev_destroy(kbdev);
		return rc;
	}

	async_exchange_end(exch);

	kdev->ctl_private = (void *) kbdev;
	return 0;
}

static void kbdev_ctl_set_ind(kbd_dev_t *kdev, unsigned mods)
{
	async_sess_t *sess = ((kbdev_t *) kdev->ctl_private)->sess;
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return;

	async_msg_1(exch, KBDEV_SET_IND, mods);
	async_exchange_end(exch);
}

static void kbdev_callback_conn(ipc_call_t *icall, void *arg)
{
	kbdev_t *kbdev;
	errno_t retval;
	int type, key;

	/* Kbdev device structure */
	kbdev = arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			kbdev_destroy(kbdev);
			return;
		}

		switch (ipc_get_imethod(&call)) {
		case KBDEV_EVENT:
			/* Got event from keyboard device */
			retval = 0;
			type = ipc_get_arg1(&call);
			key = ipc_get_arg2(&call);
			kbd_push_event(kbdev->kbd_dev, type, key);
			break;
		default:
			retval = ENOTSUP;
			break;
		}

		async_answer_0(&call, retval);
	}
}

/**
 * @}
 */
