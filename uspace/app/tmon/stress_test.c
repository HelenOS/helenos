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
 * USB stress tests.
 */

#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <usbdiag_iface.h>
#include "commands.h"
#include "test.h"

#define NAME "tmon"

typedef struct tmon_stress_test_params {
	tmon_test_params_t base; /* inheritance */
	int cycles;
	size_t size;
} tmon_stress_test_params_t;

static int read_params(int argc, char *argv[], tmon_test_params_t **params)
{
	tmon_stress_test_params_t *p = (tmon_stress_test_params_t *) malloc(sizeof(tmon_stress_test_params_t));
	if (!p)
		return ENOMEM;

	// Default values.
	p->cycles = 1024;
	p->size = 65024;

	// TODO: Parse argc, argv here.

	*params = (tmon_test_params_t *) p;
	return EOK;
}

static int run_intr_in(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_stress_test_params_t *params = (tmon_stress_test_params_t *) generic_params;
	printf("Executing interrupt in stress test.\n"
	    "      Packet count: %d\n"
	    "      Packet size: %ld\n", params->cycles, params->size);

	int rc = usbdiag_stress_intr_in(exch, params->cycles, params->size);
	if (rc) {
		printf(NAME ": Test failed. %s\n", str_error(rc));
		return 1;
	}

	return 0;
}

static int run_intr_out(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_stress_test_params_t *params = (tmon_stress_test_params_t *) generic_params;
	printf("Executing interrupt out stress test.\n"
	    "      Packet count: %d\n"
	    "      Packet size: %ld\n", params->cycles, params->size);

	int rc = usbdiag_stress_intr_out(exch, params->cycles, params->size);
	if (rc) {
		printf(NAME ": Test failed. %s\n", str_error(rc));
		return 1;
	}

	return 0;
}

static int run_bulk_in(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_stress_test_params_t *params = (tmon_stress_test_params_t *) generic_params;
	printf("Executing bulk in stress test.\n"
	    "      Packet count: %d\n"
	    "      Packet size: %ld\n", params->cycles, params->size);

	int rc = usbdiag_stress_bulk_in(exch, params->cycles, params->size);
	if (rc) {
		printf(NAME ": Test failed. %s\n", str_error(rc));
		return 1;
	}

	return 0;
}

static int run_bulk_out(async_exch_t *exch, const tmon_test_params_t *generic_params)
{
	const tmon_stress_test_params_t *params = (tmon_stress_test_params_t *) generic_params;
	printf("Executing bulk out stress test.\n"
	    "      Packet count: %d\n"
	    "      Packet size: %ld\n", params->cycles, params->size);

	int rc = usbdiag_stress_bulk_out(exch, params->cycles, params->size);
	if (rc) {
		printf(NAME ": Test failed. %s\n", str_error(rc));
		return 1;
	}

	return 0;
}

int tmon_stress_intr_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_intr_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

int tmon_stress_intr_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_intr_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

int tmon_stress_bulk_in(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_bulk_in,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

int tmon_stress_bulk_out(int argc, char *argv[])
{
	static const tmon_test_ops_t ops = {
		.run = run_bulk_out,
		.read_params = read_params
	};

	return tmon_test_main(argc, argv, &ops);
}

/** @}
 */
