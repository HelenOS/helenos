/*
 * Copyright (c) 2016 Jakub Jermar
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

/** @file
 */

#include <sys/types.h>
#include <char_dev_iface.h>
#include <stdio.h>
#include <stdlib.h>
#include <async.h>
#include <fibril_synch.h>
#include <loc.h>
#include <errno.h>
#include <str.h>
#include <sysinfo.h>
#include "../ctl/serial.h"
#include "../output.h"
#include "chardev.h"

static char *console;

static async_sess_t *sess;
static service_id_t serial_cat_id;

static FIBRIL_MUTEX_INITIALIZE(discovery_lock);
static bool discovery_finished;
static FIBRIL_CONDVAR_INITIALIZE(discovery_cv);

static void chardev_putchar(wchar_t ch)
{
	uint8_t byte = (uint8_t) ch;
	char_dev_write(sess, &byte, 1); 
}

static void chardev_control_puts(const char *str)
{
	char_dev_write(sess, (void *) str, str_size(str));
}

/*
 * This callback scans all the services in the 'serial' category, hoping to see
 * the single one the user wishes to use as a serial console. If it spots it, it
 * connects to it and registers it as an output device. Then it unblocks the
 * fibril blocked in chardev_init().
 */
static void check_for_dev(void)
{
	int rc;

	fibril_mutex_lock(&discovery_lock);
	if (discovery_finished) {
		// TODO: no need to receive these callbacks anymore
		fibril_mutex_unlock(&discovery_lock);
		return;
	}

	service_id_t *svc;
	size_t svcs;
	rc = loc_category_get_svcs(serial_cat_id, &svc, &svcs);
	if (rc != EOK) {
		fibril_mutex_unlock(&discovery_lock);
		printf("%s: Failed to get services\n", NAME);
		return;
	}

	service_id_t sid;
	bool found = false;

	for (size_t i = 0; i < svcs && !found; i++) {
		char *name;
		
		rc = loc_service_get_name(svc[i], &name);
		if (rc != EOK)
			continue;

		if (!str_cmp(console, name)) {
			/*
			 * This is the serial console service that the user
			 * wanted to use.
			 */
			found = true;
			sid = svc[i];
		}
			
		free(name);
	}

	free(svc);

	if (!found) {
		fibril_mutex_unlock(&discovery_lock);
		return;
	}

	sess = loc_service_connect(sid, INTERFACE_DDF, IPC_FLAG_BLOCKING);
	if (!sess) {
		fibril_mutex_unlock(&discovery_lock);
		printf("%s: Failed connecting to device\n", NAME);
		return;
	}
	serial_init(chardev_putchar, chardev_control_puts);

	discovery_finished = true;
	fibril_condvar_signal(&discovery_cv);
	fibril_mutex_unlock(&discovery_lock);
}

int chardev_init(void)
{
	char *boot_args;
	size_t size;
	int rc;

	boot_args = sysinfo_get_data("boot_args", &size);
	if (!boot_args || !size) {
		/*
		 * Ok, there is nothing in the boot arguments. That means that
		 * the user did not specify a serial console device.
		 */
		return EOK;
	}

	char *args = boot_args;
	char *arg;
#define ARG_CONSOLE	"console="
	while ((arg = str_tok(args, " ", &args)) != NULL) {
		if (!str_lcmp(arg, ARG_CONSOLE, str_length(ARG_CONSOLE))) {
			console = arg + str_length(ARG_CONSOLE);
			break;
		}
	}

	if (!console) {
		/*
		 * The user specified some boot arguments, but the serial
		 * console service was not among them.
		 */
		return EOK;
	}

	rc = loc_category_get_id("serial", &serial_cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Failed to get \"serial\" category ID.\n", NAME);
		return rc;
	} 

	rc = loc_register_cat_change_cb(check_for_dev);
	if (rc != EOK) {
		printf("%s: Failed to register callback for device discovery.\n",
		    NAME);
		return rc;
	}

	fibril_mutex_lock(&discovery_lock);
	while (!discovery_finished)
		fibril_condvar_wait(&discovery_cv, &discovery_lock);
	fibril_mutex_unlock(&discovery_lock);

	return EOK;
}

/** @}
 */
