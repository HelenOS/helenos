/*
 * Copyright (c) 2006 Josef Cejka
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

/**
 * @addtogroup kbdgen generic
 * @brief HelenOS generic uspace keyboard handler.
 * @ingroup kbd
 * @{
 */
/** @file
 */

#include <adt/list.h>
#include <ipc/services.h>
#include <ipc/kbd.h>
#include <sysinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ns.h>
#include <ns_obsolete.h>
#include <async.h>
#include <async_obsolete.h>
#include <errno.h>
#include <adt/fifo.h>
#include <io/console.h>
#include <io/keycode.h>
#include <devmap.h>
#include <kbd.h>
#include <kbd_port.h>
#include <kbd_ctl.h>
#include <layout.h>

// FIXME: remove this header
#include <kernel/ipc/ipc_methods.h>

#define NAME       "kbd"
#define NAMESPACE  "hid_in"

static void kbd_devs_yield(void);
static void kbd_devs_reclaim(void);

int client_phone = -1;

/** Currently active modifiers. */
static unsigned mods = KM_NUM_LOCK;

/** Currently pressed lock keys. We track these to tackle autorepeat. */
static unsigned lock_keys;

/** List of keyboard devices */
static link_t kbd_devs;

bool irc_service = false;
int irc_phone = -1;

#define NUM_LAYOUTS 3

static layout_op_t *layout[NUM_LAYOUTS] = {
	&us_qwerty_op,
	&us_dvorak_op,
	&cz_op
};

static int active_layout = 0;

void kbd_push_scancode(kbd_dev_t *kdev, int scancode)
{
/*	printf("scancode: 0x%x\n", scancode);*/
	(*kdev->ctl_ops->parse_scancode)(scancode);
}

void kbd_push_ev(kbd_dev_t *kdev, int type, unsigned int key)
{
	kbd_event_t ev;
	unsigned mod_mask;

	switch (key) {
	case KC_LCTRL: mod_mask = KM_LCTRL; break;
	case KC_RCTRL: mod_mask = KM_RCTRL; break;
	case KC_LSHIFT: mod_mask = KM_LSHIFT; break;
	case KC_RSHIFT: mod_mask = KM_RSHIFT; break;
	case KC_LALT: mod_mask = KM_LALT; break;
	case KC_RALT: mod_mask = KM_RALT; break;
	default: mod_mask = 0; break;
	}

	if (mod_mask != 0) {
		if (type == KEY_PRESS)
			mods = mods | mod_mask;
		else
			mods = mods & ~mod_mask;
	}

	switch (key) {
	case KC_CAPS_LOCK: mod_mask = KM_CAPS_LOCK; break;
	case KC_NUM_LOCK: mod_mask = KM_NUM_LOCK; break;
	case KC_SCROLL_LOCK: mod_mask = KM_SCROLL_LOCK; break;
	default: mod_mask = 0; break;
	}

	if (mod_mask != 0) {
		if (type == KEY_PRESS) {
			/*
			 * Only change lock state on transition from released
			 * to pressed. This prevents autorepeat from messing
			 * up the lock state.
			 */
			mods = mods ^ (mod_mask & ~lock_keys);
			lock_keys = lock_keys | mod_mask;

			/* Update keyboard lock indicator lights. */
			(*kdev->ctl_ops->set_ind)(mods);
		} else {
			lock_keys = lock_keys & ~mod_mask;
		}
	}
/*
	printf("type: %d\n", type);
	printf("mods: 0x%x\n", mods);
	printf("keycode: %u\n", key);
*/
	if (type == KEY_PRESS && (mods & KM_LCTRL) &&
		key == KC_F1) {
		active_layout = 0;
		layout[active_layout]->reset();
		return;
	}

	if (type == KEY_PRESS && (mods & KM_LCTRL) &&
		key == KC_F2) {
		active_layout = 1;
		layout[active_layout]->reset();
		return;
	}

	if (type == KEY_PRESS && (mods & KM_LCTRL) &&
		key == KC_F3) {
		active_layout = 2;
		layout[active_layout]->reset();
		return;
	}

	ev.type = type;
	ev.key = key;
	ev.mods = mods;

	ev.c = layout[active_layout]->parse_ev(&ev);

	async_obsolete_msg_4(client_phone, KBD_EVENT, ev.type, ev.key, ev.mods, ev.c);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;

	async_answer_0(iid, EOK);

	while (true) {
		callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			if (client_phone != -1) {
				async_obsolete_hangup(client_phone);
				client_phone = -1;
			}
			
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case IPC_M_CONNECT_TO_ME:
			if (client_phone != -1) {
				retval = ELIMIT;
				break;
			}
			client_phone = IPC_GET_ARG5(call);
			retval = 0;
			break;
		case KBD_YIELD:
			kbd_devs_yield();
			retval = 0;
			break;
		case KBD_RECLAIM:
			kbd_devs_reclaim();
			retval = 0;
			break;
		default:
			retval = EINVAL;
		}
		async_answer_0(callid, retval);
	}	
}

/** Add new keyboard device. */
static void kbd_add_dev(kbd_port_ops_t *port, kbd_ctl_ops_t *ctl)
{
	kbd_dev_t *kdev;

	kdev = malloc(sizeof(kbd_dev_t));
	if (kdev == NULL) {
		printf(NAME ": Failed adding keyboard device. Out of memory.\n");
		return;
	}

	link_initialize(&kdev->kbd_devs);
	kdev->port_ops = port;
	kdev->ctl_ops = ctl;

	/* Initialize port driver. */
	if (kdev->port_ops != NULL) {
		if ((*kdev->port_ops->init)(kdev) != 0)
			goto fail;
	}

	/* Initialize controller driver. */
	if ((*kdev->ctl_ops->init)(kdev) != 0) {
		/* XXX Uninit port */
		goto fail;
	}

	list_append(&kdev->kbd_devs, &kbd_devs);
	return;
fail:
	free(kdev);
}

/** Add legacy drivers/devices. */
static void kbd_add_legacy_devs(void)
{
	/*
	 * Need to add these drivers based on config unless we can probe
	 * them automatically.
	 */
#if defined(UARCH_amd64)
	kbd_add_dev(&chardev_port, &pc_ctl);
#endif
#if defined(UARCH_arm32) && defined(MACHINE_gta02)
	kbd_add_dev(&chardev_port, &stty_ctl);
#endif
#if defined(UARCH_arm32) && defined(MACHINE_testarm) && defined(CONFIG_FB)
	kbd_add_dev(&gxemul_port, &gxe_fb_ctl);
#endif
#if defined(UARCH_arm32) && defined(MACHINE_testarm) && !defined(CONFIG_FB)
	kbd_add_dev(&gxemul_port, &stty_ctl);
#endif
#if defined(UARCH_arm32) && defined(MACHINE_integratorcp)
	kbd_add_dev(&pl050_port, &pc_ctl);
#endif
#if defined(UARCH_ia32)
	kbd_add_dev(&chardev_port, &pc_ctl);
#endif
#if defined(MACHINE_i460GX)
	kbd_add_dev(&chardev_port, &pc_ctl);
#endif
#if defined(MACHINE_ski)
	kbd_add_dev(&ski_port, &stty_ctl);
#endif
#if defined(MACHINE_msim)
	kbd_add_dev(&msim_port, &pc_ctl);
#endif
#if (defined(MACHINE_lgxemul) || defined(MACHINE_bgxemul)) && defined(CONFIG_FB)
	kbd_add_dev(&gxemul_port, &gxe_fb_ctl);
#endif
#if defined(MACHINE_lgxemul) || defined(MACHINE_bgxemul) && !defined(CONFIG_FB)
	kbd_add_dev(&gxemul_port, &stty_ctl);
#endif
#if defined(UARCH_ppc32)
	kbd_add_dev(&adb_port, &apple_ctl);
#endif
#if defined(UARCH_sparc64) && defined(PROCESSOR_sun4v)
	kbd_add_dev(&niagara_port, &stty_ctl);
#endif
#if defined(UARCH_sparc64) && defined(MACHINE_serengeti)
	kbd_add_dev(&sgcn_port, &stty_ctl);
#endif
#if defined(UARCH_sparc64) && defined(MACHINE_generic)
	kbd_add_dev(&z8530_port, &sun_ctl);
	kbd_add_dev(&ns16550_port, &sun_ctl);
#endif
}

static void kbd_devs_yield(void)
{
	/* For each keyboard device */
	list_foreach(kbd_devs, kdev_link) {
		kbd_dev_t *kdev = list_get_instance(kdev_link, kbd_dev_t,
		    kbd_devs);

		/* Yield port */
		if (kdev->port_ops != NULL)
			(*kdev->port_ops->yield)();
	}
}

static void kbd_devs_reclaim(void)
{
	/* For each keyboard device */
	list_foreach(kbd_devs, kdev_link) {
		kbd_dev_t *kdev = list_get_instance(kdev_link, kbd_dev_t,
		    kbd_devs);

		/* Reclaim port */
		if (kdev->port_ops != NULL)
			(*kdev->port_ops->reclaim)();
	}
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS Keyboard service\n", NAME);
	
	sysarg_t fhc;
	sysarg_t obio;
	
	list_initialize(&kbd_devs);
	
	if (((sysinfo_get_value("kbd.cir.fhc", &fhc) == EOK) && (fhc))
	    || ((sysinfo_get_value("kbd.cir.obio", &obio) == EOK) && (obio)))
		irc_service = true;
	
	if (irc_service) {
		while (irc_phone < 0)
			irc_phone = service_obsolete_connect_blocking(SERVICE_IRC, 0, 0);
	}
	
	/* Add legacy devices. */
	kbd_add_legacy_devs();

	/* Add kbdev device */
	kbd_add_dev(NULL, &kbdev_ctl);

	/* Initialize (reset) layout. */
	layout[active_layout]->reset();
	
	/* Register driver */
	int rc = devmap_driver_register(NAME, client_connection);
	if (rc < 0) {
		printf("%s: Unable to register driver (%d)\n", NAME, rc);
		return -1;
	}
	
	char kbd[DEVMAP_NAME_MAXLEN + 1];
	snprintf(kbd, DEVMAP_NAME_MAXLEN, "%s/%s", NAMESPACE, NAME);
	
	devmap_handle_t devmap_handle;
	if (devmap_device_register(kbd, &devmap_handle) != EOK) {
		printf("%s: Unable to register device %s\n", NAME, kbd);
		return -1;
	}

	printf(NAME ": Accepting connections\n");
	async_manager();

	/* Not reached. */
	return 0;
}

/**
 * @}
 */ 
