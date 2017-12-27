/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup tmon
 * @{
 */
/**
 * @file
 * USB burst tests.
 */

#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <getopt.h>
#include <usbdiag_iface.h>
#include <macros.h>
#include "commands.h"
#include "tf.h"

#define NAME   "tmon"
#define INDENT "      "

/** Generic burst test parameters. */
typedef struct tmon_burst_test_params {
	/** Inherited base. */
	tmon_test_params_t base;
	/** The count of reads/writes to perform. */
	uint32_t cycles;
	/** Size of single read/write. */
	size_t size;
} tmon_burst_test_params_t;

/** Static array of long options, from which test parameters are parsed. */
static struct option long_options[] = {
	{"cycles", required_argument, NULL, 'n'},
	{"size", required_argument, NULL, 's'},
	{0, 0, NULL, 0}
};

/** String of short options, from which test parameters are parsed. */
static const char *short_options = "n:s:";

/** Common option parser for all burst tests.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 * @param[out] params Parsed test parameters (if successful).
 *
 * @return EOK if successful (in such case caller becomes the owner of `params`).
 */
static int read_params(int argc, char *argv[], tmon_test_params_t **params)
{
	int rc;
	tmon_burst_test_params_t *p = (tmon_burst_test_params_t *) malloc(sizeof(tmon_burst_test_params_t));
	if (!p)
		return ENOMEM;

	// Default values.
	p->cycles = 256;
	p->size = 1024;

	// Parse other than default values.
	int c;
	for (c = 0, optreset = 1, optind = 0; c != -1;) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		switch (c) {
		case -1:
			break;
		case 'n':
			if (!optarg || str_uint32_t(optarg, NULL, 10, false, &p->cycles) != EOK) {
				puts(NAME ": Invalid number of cycles.\n");
				rc = EINVAL;
				goto err_malloc;
			}
			break;
		case 's':
			if (!optarg || str_size_t(optarg, NULL, 10, false, &p->size) != EOK) {
				puts(NAME ": Invalid data size.\n");
				rc = EINVAL;
				goto err_malloc;
			}
			break;
		}
	}

	*params = (tmon_test_params_t *) p;
	return EOK;

err_malloc:
	free(p);
	*params = NULL;
	return rc;
}

/** Unit of quantity used for pretty formatting. */
typedef struct tmon_unit {
	/** Prefix letter, which is printed before the actual unit. */
	char prefix;
	/** Factor of the unit. */
	uint64_t factor;
} tmon_unit_t;

/** Static array of units with decreasing factors. */
static const tmon_unit_t units[] = {
	{ .prefix = 'E', .factor = 1ul << 60 },
	{ .prefix = 'P', .factor = 1ul << 50 },
	{ .prefix = 'T', .factor = 1ul << 40 },
	{ .prefix = 'G', .factor = 1ul << 30 },
	{ .prefix = 'M', .factor = 1ul << 20 },
	{ .prefix = 'k', .factor = 1ul << 10 }
};

/** Format size in bytes for human reading.
 * @param[in] size The size to format.
 * @param[in] fmt Format string. Must include one double and char.
 *
 * @return Heap-allocated string if successful (caller becomes its owner), NULL otherwise.
 */
static char * format_size(double size, const char *fmt)
{
	// Figure out the "tightest" unit.
	unsigned i;
	for (i = 0; i < ARRAY_SIZE(units); ++i) {
		if (units[i].factor <= size)
			break;
	}

	char prefix[] = { '\0', '\0' };
	double factor = 1;

	if (i < ARRAY_SIZE(units)) {
		prefix[0] = units[i].prefix;
		factor = units[i].factor;
	}

	// Format the size.
	const double div_size = size / factor;
	char *out = NULL;
	asprintf(&out, fmt, div_size, prefix);

	return out;
}

/** Print burst test parameters.
 * @param[in] params Test parameters to print.
 */
static void print_params(const tmon_burst_test_params_t *params)
{
	printf(INDENT "Number of cycles: %d\n", params->cycles);

	char *str_size = format_size(params->size, "%0.3f %sB");
	printf(INDENT "Data size: %s\n", str_size);
	free(str_size);
}

/** Print burst test results.
 * @param[in] params Test parameters.
 * @param[in] duration Duration of the burst test.
 */
static void print_results(const tmon_burst_test_params_t *params, usbdiag_dur_t duration)
{
	printf(INDENT "Total duration: %ld ms\n", duration);

	const double dur_per_cycle = (double) duration / (double) params->cycles;
	printf(INDENT "Duration per cycle: %0.3f ms\n", dur_per_cycle);

	const size_t total_size = params->size * params->cycles;
	char *str_total_size = format_size(total_size, "%0.3f %sB");
	printf(INDENT "Total size: %s\n", str_total_size);
	free(str_total_size);

	const double speed = 1000.0 * (double) total_size / (double) duration;
	char *str_speed = format_size(speed, "%0.3f %sB/s");
	printf(INDENT "Average speed: %s\n", str_speed);
	free(str_speed);
}

/** Run "interrupt in" burst test.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'tmon_burst_test_params_t'.
 *
 * @return Exit code
 */
static int run_intr_in(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_burst_test_params_t *params = (tmon_burst_test_params_t *) generic_params;
	puts("Reading data from interrupt endpoint.\n");
	print_params(params);

	usbdiag_dur_t duration;
	int rc = usbdiag_burst_intr_in(exch, params->cycles, params->size, &duration);
	if (rc) {
		printf(NAME ": Test failed with error: %s\n", str_error(rc));
		return 1;
	}

	puts("Test succeeded.\n");
	print_results(params, duration);
	return 0;
}

/** Run "interrupt out" burst test.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'tmon_burst_test_params_t'.
 *
 * @return Exit code
 */
static int run_intr_out(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_burst_test_params_t *params = (tmon_burst_test_params_t *) generic_params;
	puts("Writing data to interrupt endpoint.\n");
	print_params(params);

	usbdiag_dur_t duration;
	int rc = usbdiag_burst_intr_out(exch, params->cycles, params->size, &duration);
	if (rc) {
		printf(NAME ": Test failed with error: %s\n", str_error(rc));
		return 1;
	}

	puts("Test succeeded.\n");
	print_results(params, duration);
	return 0;
}

/** Run "bulk in" burst test.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'tmon_burst_test_params_t'.
 *
 * @return Exit code
 */
static int run_bulk_in(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_burst_test_params_t *params = (tmon_burst_test_params_t *) generic_params;
	puts("Reading data from bulk endpoint.\n");
	print_params(params);

	usbdiag_dur_t duration;
	int rc = usbdiag_burst_bulk_in(exch, params->cycles, params->size, &duration);
	if (rc) {
		printf(NAME ": Test failed with error: %s\n", str_error(rc));
		return 1;
	}

	puts("Test succeeded.\n");
	print_results(params, duration);
	return 0;
}

/** Run "bulk out" burst test.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'tmon_burst_test_params_t'.
 *
 * @return Exit code
 */
static int run_bulk_out(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_burst_test_params_t *params = (tmon_burst_test_params_t *) generic_params;
	puts("Writing data to bulk endpoint.\n");
	print_params(params);

	usbdiag_dur_t duration;
	int rc = usbdiag_burst_bulk_out(exch, params->cycles, params->size, &duration);
	if (rc) {
		printf(NAME ": Test failed with error: %s\n", str_error(rc));
		return 1;
	}

	puts("Test succeeded.\n");
	print_results(params, duration);
	return 0;
}

/** Run "isochronous in" burst test.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'tmon_burst_test_params_t'.
 *
 * @return Exit code
 */
static int run_isoch_in(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_burst_test_params_t *params = (tmon_burst_test_params_t *) generic_params;
	puts("Reading data from isochronous endpoint.\n");
	print_params(params);

	usbdiag_dur_t duration;
	int rc = usbdiag_burst_isoch_in(exch, params->cycles, params->size, &duration);
	if (rc) {
		printf(NAME ": Test failed with error: %s\n", str_error(rc));
		return 1;
	}

	puts("Test succeeded.\n");
	print_results(params, duration);
	return 0;
}

/** Run "isochronous out" burst test.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'tmon_burst_test_params_t'.
 *
 * @return Exit code
 */
static int run_isoch_out(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_burst_test_params_t *params = (tmon_burst_test_params_t *) generic_params;
	puts("Writing data to isochronous endpoint.\n");
	print_params(params);

	usbdiag_dur_t duration;
	int rc = usbdiag_burst_isoch_out(exch, params->cycles, params->size, &duration);
	if (rc) {
		printf(NAME ": Test failed with error: %s\n", str_error(rc));
		return 1;
	}

	puts("Test succeeded.\n");
	print_results(params, duration);
	return 0;
}

/** Interrupt in burst test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_burst_intr_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_intr_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Interrupt out burst test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_burst_intr_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_intr_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Interrupt bulk burst test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_burst_bulk_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_bulk_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Bulk out burst test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_burst_bulk_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_bulk_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Isochronous in burst test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_burst_isoch_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_isoch_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Isochronous out burst test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_burst_isoch_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_isoch_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** @}
 */
