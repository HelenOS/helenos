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
 * @addtogroup inputgen generic
 * @brief HelenOS input server.
 * @ingroup input
 * @{
 */
/** @file
 */

#include <adt/list.h>
#include <bool.h>
#include <fibril_synch.h>
#include <ipc/services.h>
#include <ipc/input.h>
#include <sysinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <adt/fifo.h>
#include <io/console.h>
#include <io/keycode.h>
#include <loc.h>
#include <input.h>
#include <kbd.h>
#include <kbd_port.h>
#include <kbd_ctl.h>
#include <mouse_proto.h>
#include <layout.h>
#include <mouse.h>

#define NUM_LAYOUTS  3

static layout_ops_t *layout[NUM_LAYOUTS] = {
	&us_qwerty_ops,
	&us_dvorak_ops,
	&cz_ops
};

static void kbd_devs_yield(void);
static void kbd_devs_reclaim(void);

async_sess_t *client_sess = NULL;

/** List of keyboard devices */
static list_t kbd_devs;

/** List of mouse devices */
static list_t mouse_devs;

bool irc_service = false;
async_sess_t *irc_sess = NULL;

static FIBRIL_MUTEX_INITIALIZE(discovery_lock);

void kbd_push_data(kbd_dev_t *kdev, sysarg_t data)
{
	(*kdev->ctl_ops->parse)(data);
}

void mouse_push_data(mouse_dev_t *mdev, sysarg_t data)
{
	(*mdev->proto_ops->parse)(data);
}

void kbd_push_event(kbd_dev_t *kdev, int type, unsigned int key)
{
	kbd_event_t ev;
	unsigned int mod_mask;
	
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
			kdev->mods = kdev->mods | mod_mask;
		else
			kdev->mods = kdev->mods & ~mod_mask;
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
			kdev->mods = kdev->mods ^ (mod_mask & ~kdev->lock_keys);
			kdev->lock_keys = kdev->lock_keys | mod_mask;
			
			/* Update keyboard lock indicator lights. */
			(*kdev->ctl_ops->set_ind)(kdev, kdev->mods);
		} else {
			kdev->lock_keys = kdev->lock_keys & ~mod_mask;
		}
	}
	
	if (type == KEY_PRESS && (kdev->mods & KM_LCTRL) &&
	    key == KC_F1) {
		layout_destroy(kdev->active_layout);
		kdev->active_layout = layout_create(layout[0]);
		return;
	}
	
	if (type == KEY_PRESS && (kdev->mods & KM_LCTRL) &&
	    key == KC_F2) {
		layout_destroy(kdev->active_layout);
		kdev->active_layout = layout_create(layout[1]);
		return;
	}
	
	if (type == KEY_PRESS && (kdev->mods & KM_LCTRL) &&
	    key == KC_F3) {
		layout_destroy(kdev->active_layout);
		kdev->active_layout = layout_create(layout[2]);
		return;
	}
	
	ev.type = type;
	ev.key = key;
	ev.mods = kdev->mods;
	
	ev.c = layout_parse_ev(kdev->active_layout, &ev);
	
	async_exch_t *exch = async_exchange_begin(client_sess);
	async_msg_4(exch, INPUT_EVENT_KEY, ev.type, ev.key, ev.mods, ev.c);
	async_exchange_end(exch);
}

/** Mouse pointer has moved. */
void mouse_push_event_move(mouse_dev_t *mdev, int dx, int dy, int dz)
{
	async_exch_t *exch = async_exchange_begin(client_sess);
	if (dx || dy)
		async_msg_2(exch, INPUT_EVENT_MOVE, dx, dy);
	if (dz) {
		// TODO: Implement proper wheel support
		keycode_t code = dz > 0 ? KC_UP : KC_DOWN;
		for (int i = 0; i < 3; ++i) {
			async_msg_4(exch, INPUT_EVENT_KEY, KEY_PRESS, code, 0, 0);
		}
		async_msg_4(exch, INPUT_EVENT_KEY, KEY_RELEASE, code, 0, 0);
	}
	async_exchange_end(exch);
}

/** Mouse button has been pressed. */
void mouse_push_event_button(mouse_dev_t *mdev, int bnum, int press)
{
	async_exch_t *exch = async_exchange_begin(client_sess);
	async_msg_2(exch, INPUT_EVENT_BUTTON, bnum, press);
	async_exchange_end(exch);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			if (client_sess != NULL) {
				async_hangup(client_sess);
				client_sess = NULL;
			}
			
			async_answer_0(callid, EOK);
			return;
		}
		
		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, &call);
		if (sess != NULL) {
			if (client_sess == NULL) {
				client_sess = sess;
				async_answer_0(callid, EOK);
			} else
				async_answer_0(callid, ELIMIT);
		} else {
			switch (IPC_GET_IMETHOD(call)) {
			case INPUT_YIELD:
				kbd_devs_yield();
				async_answer_0(callid, EOK);
				break;
			case INPUT_RECLAIM:
				kbd_devs_reclaim();
				async_answer_0(callid, EOK);
				break;
			default:
				async_answer_0(callid, EINVAL);
			}
		}
	}
}

static kbd_dev_t *kbd_dev_new(void)
{
	kbd_dev_t *kdev = calloc(1, sizeof(kbd_dev_t));
	if (kdev == NULL) {
		printf("%s: Error allocating keyboard device. "
		    "Out of memory.\n", NAME);
		return NULL;
	}
	
	link_initialize(&kdev->kbd_devs);
	
	kdev->mods = KM_NUM_LOCK;
	kdev->lock_keys = 0;
	kdev->active_layout = layout_create(layout[0]);
	
	return kdev;
}

static mouse_dev_t *mouse_dev_new(void)
{
	mouse_dev_t *mdev = calloc(1, sizeof(mouse_dev_t));
	if (mdev == NULL) {
		printf("%s: Error allocating keyboard device. "
		    "Out of memory.\n", NAME);
		return NULL;
	}
	
	link_initialize(&mdev->mouse_devs);
	
	return mdev;
}

/** Add new legacy keyboard device. */
static void kbd_add_dev(kbd_port_ops_t *port, kbd_ctl_ops_t *ctl)
{
	kbd_dev_t *kdev = kbd_dev_new();
	if (kdev == NULL)
		return;
	
	kdev->port_ops = port;
	kdev->ctl_ops = ctl;
	kdev->svc_id = 0;
	
	/* Initialize port driver. */
	if ((*kdev->port_ops->init)(kdev) != 0)
		goto fail;
	
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

/** Add new legacy mouse device. */
static void mouse_add_dev(mouse_port_ops_t *port, mouse_proto_ops_t *proto)
{
	mouse_dev_t *mdev = mouse_dev_new();
	if (mdev == NULL)
		return;
	
	mdev->port_ops = port;
	mdev->proto_ops = proto;
	mdev->svc_id = 0;
	
	/* Initialize port driver. */
	if ((*mdev->port_ops->init)(mdev) != 0)
		goto fail;
	
	/* Initialize protocol driver. */
	if ((*mdev->proto_ops->init)(mdev) != 0) {
		/* XXX Uninit port */
		goto fail;
	}
	
	list_append(&mdev->mouse_devs, &mouse_devs);
	return;
	
fail:
	free(mdev);
}

/** Add new kbdev device.
 *
 * @param service_id	Service ID of the keyboard device
 *
 */
static int kbd_add_kbdev(service_id_t service_id, kbd_dev_t **kdevp)
{
	kbd_dev_t *kdev = kbd_dev_new();
	if (kdev == NULL)
		return -1;
	
	kdev->svc_id = service_id;
	kdev->port_ops = NULL;
	kdev->ctl_ops = &kbdev_ctl;
	
	int rc = loc_service_get_name(service_id, &kdev->svc_name);
	if (rc != EOK) {
		kdev->svc_name = NULL;
		goto fail;
	}
	
	/* Initialize controller driver. */
	if ((*kdev->ctl_ops->init)(kdev) != 0) {
		goto fail;
	}
	
	list_append(&kdev->kbd_devs, &kbd_devs);
	*kdevp = kdev;
	return EOK;
	
fail:
	if (kdev->svc_name != NULL)
		free(kdev->svc_name);
	free(kdev);
	return -1;
}

/** Add new mousedev device.
 *
 * @param service_id	Service ID of the mouse device
 *
 */
static int mouse_add_mousedev(service_id_t service_id, mouse_dev_t **mdevp)
{
	mouse_dev_t *mdev = mouse_dev_new();
	if (mdev == NULL)
		return -1;
	
	mdev->svc_id = service_id;
	mdev->port_ops = NULL;
	mdev->proto_ops = &mousedev_proto;
	
	int rc = loc_service_get_name(service_id, &mdev->svc_name);
	if (rc != EOK) {
		mdev->svc_name = NULL;
		goto fail;
	}
	
	/* Initialize controller driver. */
	if ((*mdev->proto_ops->init)(mdev) != 0) {
		goto fail;
	}
	
	list_append(&mdev->mouse_devs, &mouse_devs);
	*mdevp = mdev;
	return EOK;
	
fail:
	free(mdev);
	return -1;
}

/** Add legacy drivers/devices. */
static void kbd_add_legacy_devs(void)
{
	/*
	 * Need to add these drivers based on config unless we can probe
	 * them automatically.
	 */
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
#if defined(MACHINE_ski)
	kbd_add_dev(&ski_port, &stty_ctl);
#endif
#if defined(MACHINE_msim)
	kbd_add_dev(&msim_port, &stty_ctl);
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
#if defined(UARCH_sparc64) && defined(MACHINE_generic)
	kbd_add_dev(&ns16550_port, &sun_ctl);
#endif
	/* Silence warning on abs32le about kbd_add_dev() being unused */
	(void) kbd_add_dev;
}

/** Add legacy drivers/devices. */
static void mouse_add_legacy_devs(void)
{
	/*
	 * Need to add these drivers based on config unless we can probe
	 * them automatically.
	 */
#if defined(UARCH_ppc32)
	mouse_add_dev(&adb_mouse_port, &adb_proto);
#endif
	/* Silence warning on abs32le about mouse_add_dev() being unused */
	(void) mouse_add_dev;
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

static int dev_check_new_kbdevs(void)
{
	category_id_t keyboard_cat;
	service_id_t *svcs;
	size_t count, i;
	bool already_known;
	int rc;
	
	rc = loc_category_get_id("keyboard", &keyboard_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Failed resolving category 'keyboard'.\n", NAME);
		return ENOENT;
	}
	
	/*
	 * Check for new keyboard devices
	 */
	rc = loc_category_get_svcs(keyboard_cat, &svcs, &count);
	if (rc != EOK) {
		printf("%s: Failed getting list of keyboard devices.\n",
		    NAME);
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;
		
		/* Determine whether we already know this device. */
		list_foreach(kbd_devs, kdev_link) {
			kbd_dev_t *kdev = list_get_instance(kdev_link,
			    kbd_dev_t, kbd_devs);
			if (kdev->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}
		
		if (!already_known) {
			kbd_dev_t *kdev;
			if (kbd_add_kbdev(svcs[i], &kdev) == EOK) {
				printf("%s: Connected keyboard device '%s'\n",
				    NAME, kdev->svc_name);
			}
		}
	}
	
	free(svcs);
	
	/* XXX Handle device removal */
	
	return EOK;
}

static int dev_check_new_mousedevs(void)
{
	category_id_t mouse_cat;
	service_id_t *svcs;
	size_t count, i;
	bool already_known;
	int rc;
	
	rc = loc_category_get_id("mouse", &mouse_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Failed resolving category 'mouse'.\n", NAME);
		return ENOENT;
	}
	
	/*
	 * Check for new mouse devices
	 */
	rc = loc_category_get_svcs(mouse_cat, &svcs, &count);
	if (rc != EOK) {
		printf("%s: Failed getting list of mouse devices.\n",
		    NAME);
		return EIO;
	}
	
	for (i = 0; i < count; i++) {
		already_known = false;
		
		/* Determine whether we already know this device. */
		list_foreach(mouse_devs, mdev_link) {
			mouse_dev_t *mdev = list_get_instance(mdev_link,
			    mouse_dev_t, mouse_devs);
			if (mdev->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}
		
		if (!already_known) {
			mouse_dev_t *mdev;
			if (mouse_add_mousedev(svcs[i], &mdev) == EOK) {
				printf("%s: Connected mouse device '%s'\n",
				    NAME, mdev->svc_name);
			}
		}
	}
	
	free(svcs);
	
	/* XXX Handle device removal */
	
	return EOK;
}

static int dev_check_new(void)
{
	int rc;
	
	fibril_mutex_lock(&discovery_lock);
	
	rc = dev_check_new_kbdevs();
	if (rc != EOK) {
		fibril_mutex_unlock(&discovery_lock);
		return rc;
	}
	
	rc = dev_check_new_mousedevs();
	if (rc != EOK) {
		fibril_mutex_unlock(&discovery_lock);
		return rc;
	}
	
	fibril_mutex_unlock(&discovery_lock);
	
	return EOK;
}

static void cat_change_cb(void)
{
	dev_check_new();
}

/** Start listening for new devices. */
static int input_start_dev_discovery(void)
{
	int rc;

	rc = loc_register_cat_change_cb(cat_change_cb);
	if (rc != EOK) {
		printf("%s: Failed registering callback for device discovery. "
		    "(%d)\n", NAME, rc);
		return rc;
	}

	return dev_check_new();
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS input service\n", NAME);
	
	sysarg_t obio;
	
	list_initialize(&kbd_devs);
	list_initialize(&mouse_devs);
	
	if ((sysinfo_get_value("kbd.cir.obio", &obio) == EOK) && (obio))
		irc_service = true;
	
	if (irc_service) {
		while (irc_sess == NULL)
			irc_sess = service_connect_blocking(EXCHANGE_SERIALIZE,
			    SERVICE_IRC, 0, 0);
	}
	
	/* Add legacy keyboard devices. */
	kbd_add_legacy_devs();
	
	/* Add legacy mouse devices. */
	mouse_add_legacy_devs();
	
	/* Register driver */
	async_set_client_connection(client_connection);
	int rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register server\n", NAME);
		return rc;
	}
	
	char kbd[LOC_NAME_MAXLEN + 1];
	snprintf(kbd, LOC_NAME_MAXLEN, "%s/%s", NAMESPACE, NAME);
	
	service_id_t service_id;
	rc = loc_service_register(kbd, &service_id);
	if (rc != EOK) {
		printf("%s: Unable to register service %s\n", NAME, kbd);
		return rc;
	}
	
	/* Start looking for new input devices */
	input_start_dev_discovery();
	
	printf("%s: Accepting connections\n", NAME);
	async_manager();
	
	/* Not reached. */
	return 0;
}

/**
 * @}
 */
