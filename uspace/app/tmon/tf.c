/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tmon
 * @{
 */
/**
 * @file
 * Testing framework.
 */

#include <stdio.h>
#include <macros.h>
#include <devman.h>
#include <str_error.h>
#include <usbdiag_iface.h>
#include "resolve.h"
#include "tf.h"

#define NAME "tmon"
#define MAX_PATH_LENGTH 1024

/** Common command handler for all test commands.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_main(int argc, char *argv[], const tmon_test_ops_t *ops)
{
	// Resolve device function.
	devman_handle_t fun = -1;

	if (argc >= 2 && *argv[1] != '-') {
		// Assume that the first argument is device path.
		if (tmon_resolve_named(argv[1], &fun))
			return 1;
	} else {
		// The first argument is either an option or not present.
		if (tmon_resolve_default(&fun))
			return 1;
	}

	errno_t rc;
	int ec;
	char path[MAX_PATH_LENGTH];
	if ((rc = devman_fun_get_path(fun, path, sizeof(path)))) {
		printf(NAME ": Error resolving path of device with handle "
		    "%" PRIun ". %s\n", fun, str_error(rc));
		return 1;
	}

	printf("Device: %s\n", path);

	// Read test parameters from options.
	void *params = NULL;
	if ((rc = ops->read_params(argc, argv, &params))) {
		printf(NAME ": Reading test parameters failed. %s\n", str_error(rc));
		return 1;
	}

	if ((rc = ops->pre_run(params))) {
		printf(NAME ": Pre-run hook failed. %s\n", str_error(rc));
		return 1;
	}

	// Run the test body.
	async_sess_t *sess = usbdiag_connect(fun);
	if (!sess) {
		printf(NAME ": Could not connect to the device.\n");
		return 1;
	}

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch) {
		printf(NAME ": Could not start exchange with the device.\n");
		ec = 1;
		goto err_sess;
	}

	ec = ops->run(exch, params);
	async_exchange_end(exch);

err_sess:
	usbdiag_disconnect(sess);
	free(params);
	return ec;
}

/** Unit of quantity used for pretty formatting. */
typedef struct tmon_unit {
	/** Prefix letter, which is printed before the actual unit. */
	const char *unit;
	/** Factor of the unit. */
	double factor;
} tmon_unit_t;

/** Format a value for human reading.
 * @param[in] val The value to format.
 * @param[in] fmt Format string. Must include one double and char.
 *
 * @return Heap-allocated string if successful (caller becomes its owner), NULL otherwise.
 */
static char *format_unit(double val, const char *fmt, const tmon_unit_t *units, size_t len)
{
	// Figure out the "tightest" unit.
	unsigned i;
	for (i = 0; i < len; ++i) {
		if (units[i].factor <= val)
			break;
	}

	if (i == len)
		--i;
	const char *unit = units[i].unit;
	double factor = units[i].factor;

	// Format the size.
	const double div_size = val / factor;

	char *out = NULL;
	asprintf(&out, fmt, div_size, unit);

	return out;
}

/** Static array of size units with decreasing factors. */
static const tmon_unit_t size_units[] = {
	{ .unit = "EB", .factor = 1ULL << 60 },
	{ .unit = "PB", .factor = 1ULL << 50 },
	{ .unit = "TB", .factor = 1ULL << 40 },
	{ .unit = "GB", .factor = 1ULL << 30 },
	{ .unit = "MB", .factor = 1ULL << 20 },
	{ .unit = "kB", .factor = 1ULL << 10 },
	{ .unit = "B",  .factor = 1ULL },
};

char *tmon_format_size(double val, const char *fmt)
{
	return format_unit(val, fmt, size_units, ARRAY_SIZE(size_units));
}

/** Static array of duration units with decreasing factors. */
static const tmon_unit_t dur_units[] = {
	{ .unit = "d",   .factor = 60 * 60 * 24 },
	{ .unit = "h",   .factor = 60 * 60 },
	{ .unit = "min", .factor = 60 },
	{ .unit = "s",   .factor = 1 },
	{ .unit = "ms",  .factor = 1e-3 },
	{ .unit = "us",  .factor = 1e-6 },
	{ .unit = "ns",  .factor = 1e-9 },
	{ .unit = "ps",  .factor = 1e-12 },
};

char *tmon_format_duration(usbdiag_dur_t val, const char *fmt)
{
	return format_unit(val / 1000.0, fmt, dur_units, ARRAY_SIZE(dur_units));
}

/** @}
 */
