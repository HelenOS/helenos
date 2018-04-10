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
 * USB tests.
 */

#include <stdio.h>
#include <errno.h>
#include <str.h>
#include <str_error.h>
#include <getopt.h>
#include <usb/usb.h>
#include <usbdiag_iface.h>
#include "commands.h"
#include "tf.h"

#define NAME   "tmon"
#define INDENT "      "

/** Static array of long options, from which test parameters are parsed. */
static struct option long_options[] = {
	{ "duration", required_argument, NULL, 't' },
	{ "size", required_argument, NULL, 's' },
	{ "validate", required_argument, NULL, 'v' },
	{ 0, 0, NULL, 0 }
};

/** String of short options, from which test parameters are parsed. */
static const char *short_options = "t:s:v";

/** Common option parser for all tests.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 * @param[out] params Parsed test parameters (if successful).
 *
 * @return EOK if successful (in such case caller becomes the owner of `params`).
 */
static errno_t read_params(int argc, char *argv[], void **params)
{
	errno_t rc;
	usbdiag_test_params_t *p = (usbdiag_test_params_t *) malloc(sizeof(usbdiag_test_params_t));
	if (!p)
		return ENOMEM;

	// Default values.
	p->transfer_size = 0;
	p->min_duration = 5000;
	p->validate_data = false;

	// Parse other than default values.
	int c;
	uint32_t duration_uint;
	c = 0;
	optreset = 1;
	optind = 0;
	while (c != -1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		switch (c) {
		case -1:
			break;
		case 'v':
			p->validate_data = true;
			break;
		case 't':
			if (!optarg || str_uint32_t(optarg, NULL, 10, false, &duration_uint) != EOK) {
				puts(NAME ": Invalid duration.");
				rc = EINVAL;
				goto err_malloc;
			}
			p->min_duration = (usbdiag_dur_t) duration_uint * 1000;
			break;
		case 's':
			if (!optarg || str_size_t(optarg, NULL, 10, false, &p->transfer_size) != EOK) {
				puts(NAME ": Invalid data size.");
				rc = EINVAL;
				goto err_malloc;
			}
			break;
		}
	}

	*params = (void *) p;
	return EOK;

err_malloc:
	free(p);
	*params = NULL;
	return rc;
}

/** Print test parameters.
 * @param[in] params Test parameters to print.
 */
static void print_params(const usbdiag_test_params_t *params)
{
	printf("Endpoint type: %s\n", usb_str_transfer_type(params->transfer_type));
	char *str_min_duration = tmon_format_duration(params->min_duration, "%.3f %s");
	printf("Min. duration: %s\n", str_min_duration);
	free(str_min_duration);

	if (params->transfer_size) {
		char *str_size = tmon_format_size(params->transfer_size, "%.3f %s");
		printf("Transfer size: %s\n", str_size);
		free(str_size);
	} else {
		printf("Transfer size: (max. transfer size)\n");
	}

	printf("Validate data: %s\n", params->validate_data ? "yes" : "no");
}

/** Print test results.
 * @param[in] params Test parameters.
 * @param[in] results Test results.
 */
static void print_results(const usbdiag_test_params_t *params, const usbdiag_test_results_t *results)
{
	printf("Transfers performed: %u\n", results->transfer_count);

	char *str_total_duration = tmon_format_duration(results->act_duration, "%.3f %s");
	printf("Total duration: %s\n", str_total_duration);
	free(str_total_duration);

	char *str_size_per_transfer = tmon_format_size(results->transfer_size, "%.3f %s");
	printf("Transfer size: %s\n", str_size_per_transfer);
	free(str_size_per_transfer);

	const size_t total_size = results->transfer_size * results->transfer_count;
	char *str_total_size = tmon_format_size(total_size, "%.3f %s");
	printf("Total size: %s\n", str_total_size);
	free(str_total_size);

	const double dur_per_transfer = (double) results->act_duration / (double) results->transfer_count;
	char *str_dur_per_transfer = tmon_format_duration(dur_per_transfer, "%.3f %s");
	printf("Avg. transfer duration: %s\n", str_dur_per_transfer);
	free(str_dur_per_transfer);

	const double speed = 1000.0 * (double) total_size / (double) results->act_duration;
	char *str_speed = tmon_format_size(speed, "%.3f %s/s");
	printf("Avg. speed: %s\n", str_speed);
	free(str_speed);
}

/** Run test on "in" endpoint.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'usbdiag_test_params_t'.
 *
 * @return Exit code
 */
static int test_in(async_exch_t *exch, const void *generic_params)
{
	const usbdiag_test_params_t *params = (usbdiag_test_params_t *) generic_params;
	print_params(params);
	fputs("\nTesting... ", stdout);

	usbdiag_test_results_t results;
	errno_t rc = usbdiag_test_in(exch, params, &results);
	if (rc != EOK) {
		puts("failed");
		printf(NAME ": %s\n", str_error(rc));
		return 1;
	}

	puts("succeeded\n");
	print_results(params, &results);
	return 0;
}

/** Run test on "out" endpoint.
 * @param[in] exch Open async exchange with the diagnostic device.
 * @param[in] generic_params Test parameters. Must point to 'usbdiag_test_params_t'.
 *
 * @return Exit code
 */
static int test_out(async_exch_t *exch, const void *generic_params)
{
	const usbdiag_test_params_t *params = (usbdiag_test_params_t *) generic_params;
	print_params(params);
	fputs("\nTesting... ", stdout);

	usbdiag_test_results_t results;
	errno_t rc = usbdiag_test_out(exch, params, &results);
	if (rc) {
		puts("failed");
		printf(NAME ": %s\n", str_error(rc));
		return 1;
	}

	puts("succeeded\n");
	print_results(params, &results);
	return 0;
}

#define GEN_PRE_RUN(FN, TYPE) \
	static int pre_run_##FN(void *generic_params) \
	{ \
		usbdiag_test_params_t *params = (usbdiag_test_params_t *) generic_params; \
		params->transfer_type = USB_TRANSFER_##TYPE; \
		return EOK; \
	}

GEN_PRE_RUN(intr, INTERRUPT);
GEN_PRE_RUN(bulk, BULK);
GEN_PRE_RUN(isoch, ISOCHRONOUS);

#undef GEN_PRE_RUN

/** Interrupt in test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_intr_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.pre_run = pre_run_intr,
		.run = test_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Interrupt out test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_intr_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.pre_run = pre_run_intr,
		.run = test_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Interrupt bulk test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_bulk_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.pre_run = pre_run_bulk,
		.run = test_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Bulk out test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_bulk_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.pre_run = pre_run_bulk,
		.run = test_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Isochronous in test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_isoch_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.pre_run = pre_run_isoch,
		.run = test_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** Isochronous out test command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_isoch_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.pre_run = pre_run_isoch,
		.run = test_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** @}
 */
