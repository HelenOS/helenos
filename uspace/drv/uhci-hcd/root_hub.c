#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include <usb_iface.h>

#include <usb/debug.h>

#include "root_hub.h"
/*----------------------------------------------------------------------------*/
static int usb_iface_get_hc_handle(device_t *dev, devman_handle_t *handle)
{
  assert(dev);
  assert(dev->parent != NULL);

  device_t *parent = dev->parent;

  if (parent->ops && parent->ops->interfaces[USB_DEV_IFACE]) {
    usb_iface_t *usb_iface
        = (usb_iface_t *) parent->ops->interfaces[USB_DEV_IFACE];
    assert(usb_iface != NULL);
    if (usb_iface->get_hc_handle) {
      int rc = usb_iface->get_hc_handle(parent, handle);
      return rc;
    }
  }

  return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
static usb_iface_t usb_iface = {
  .get_hc_handle = usb_iface_get_hc_handle
};

static device_ops_t rh_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface
};

int setup_root_hub(device_t **device, device_t *hc)
{
	assert(device);
	device_t *hub = create_device();
	if (!hub) {
		usb_log_error("Failed to create root hub device structure.\n");
		return ENOMEM;
	}
	char *name;
	int ret = asprintf(&name, "UHCI Root Hub");
	if (ret < 0) {
		usb_log_error("Failed to create root hub name.\n");
		free(hub);
		return ENOMEM;
	}

	char *match_str;
	ret = asprintf(&match_str, "usb&uhci&root-hub");
	if (ret < 0) {
		usb_log_error("Failed to create root hub match string.\n");
		free(hub);
		free(name);
		return ENOMEM;
	}

	match_id_t *match_id = create_match_id();
	if (!match_id) {
		usb_log_error("Failed to create root hub match id.\n");
		free(hub);
		free(match_str);
		return ENOMEM;
	}
	match_id->id = match_str;
	match_id->score = 90;

	add_match_id(&hub->match_ids, match_id);
	hub->name = name;
	hub->parent = hc;
	hub->ops = &rh_ops;

	*device = hub;
	return EOK;
}
