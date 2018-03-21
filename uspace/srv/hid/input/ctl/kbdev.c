/*
 * Copyright (c) 2011 Jiri Svoboda
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

static void kbdev_callback_conn(cap_call_handle_t, ipc_call_t *, void *arg);

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

static void kbdev_callback_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	kbdev_t *kbdev;
	errno_t retval;
	int type, key;

	/* Kbdev device structure */
	kbdev = arg;

	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle;

		chandle = async_get_call(&call);
		if (!IPC_GET_IMETHOD(call)) {
			kbdev_destroy(kbdev);
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case KBDEV_EVENT:
			/* Got event from keyboard device */
			retval = 0;
			type = IPC_GET_ARG1(call);
			key = IPC_GET_ARG2(call);
			kbd_push_event(kbdev->kbd_dev, type, key);
			break;
		default:
			retval = ENOTSUP;
			break;
		}

		async_answer_0(chandle, retval);
	}
}

/**
 * @}
 */
