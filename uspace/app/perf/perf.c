/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup perf
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <str.h>
#include "perf.h"

benchmark_t benchmarks[] = {
#include "ipc/ns_ping.def"
#include "ipc/ping_pong.def"
#include "malloc/malloc1.def"
#include "malloc/malloc2.def"
	{ NULL, NULL, NULL }
};

static bool run_benchmark(benchmark_t *bench)
{
	/* Execute the benchmarl */
	const char *ret = bench->entry();

	if (ret == NULL) {
		printf("\nBenchmark completed\n");
		return true;
	}

	printf("\n%s\n", ret);
	return false;
}

static int run_benchmarks(void)
{
	benchmark_t *bench;
	unsigned int i = 0;
	unsigned int n = 0;

	char *failed_names = NULL;

	printf("\n*** Running all benchmarks ***\n\n");

	for (bench = benchmarks; bench->name != NULL; bench++) {
		printf("%s (%s)\n", bench->name, bench->desc);
		if (run_benchmark(bench)) {
			i++;
			continue;
		}

		if (!failed_names) {
			failed_names = str_dup(bench->name);
		} else {
			char *f = NULL;
			asprintf(&f, "%s, %s", failed_names, bench->name);
			if (!f) {
				printf("Out of memory.\n");
				abort();
			}
			free(failed_names);
			failed_names = f;
		}
		n++;
	}

	printf("\nCompleted, %u benchmarks run, %u succeeded.\n", i + n, i);
	if (failed_names)
		printf("Failed benchmarks: %s\n", failed_names);

	return n;
}

static void list_benchmarks(void)
{
	size_t len = 0;
	benchmark_t *bench;
	for (bench = benchmarks; bench->name != NULL; bench++) {
		if (str_length(bench->name) > len)
			len = str_length(bench->name);
	}

	unsigned int _len = (unsigned int) len;
	if ((_len != len) || (((int) _len) < 0)) {
		printf("Command length overflow\n");
		return;
	}

	for (bench = benchmarks; bench->name != NULL; bench++)
		printf("%-*s %s\n", _len, bench->name, bench->desc);

	printf("%-*s Run all benchmarks\n", _len, "*");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage:\n\n");
		printf("%s <benchmark>\n\n", argv[0]);
		list_benchmarks();
		return 0;
	}

	if (str_cmp(argv[1], "*") == 0) {
		return run_benchmarks();
	}

	benchmark_t *bench;
	for (bench = benchmarks; bench->name != NULL; bench++) {
		if (str_cmp(argv[1], bench->name) == 0) {
			return (run_benchmark(bench) ? 0 : -1);
		}
	}

	printf("Unknown benchmark \"%s\"\n", argv[1]);
	return -2;
}

/** @}
 */
