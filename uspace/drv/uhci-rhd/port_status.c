#include <assert.h>
#include <stdio.h>

#include <usb/debug.h>

#include "port_status.h"

struct flag_name
{
	unsigned flag;
	const char *name;
};

static const struct flag_name flags[] =
{
	{ STATUS_SUSPEND, "suspended" },
	{ STATUS_IN_RESET, "in reset" },
	{ STATUS_LOW_SPEED, "low speed device" },
	{ STATUS_ALWAYS_ONE, "always 1 bit" },
	{ STATUS_RESUME, "resume" },
	{ STATUS_LINE_D_MINUS, "line D- value" },
	{ STATUS_LINE_D_PLUS, "line D+ value" },
	{ STATUS_ENABLED_CHANGED, "enabled changed" },
	{ STATUS_ENABLED, "enabled" },
	{ STATUS_CONNECTED_CHANGED, "connected changed" },
	{ STATUS_CONNECTED, "connected" }
};

void print_port_status(port_status_t value)
{
	unsigned i = 0;
	for (;i < sizeof(flags)/sizeof(struct flag_name); ++i) {
		usb_log_debug("\t%s status: %s.\n", flags[i].name,
		  value & flags[i].flag ? "YES" : "NO");
	}
}
