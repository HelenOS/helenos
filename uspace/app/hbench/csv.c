/*
 * SPDX-FileCopyrightText: 2019 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hbench
 * @{
 */
/**
 * @file
 */

#include <stdlib.h>
#include <stdio.h>
#include "hbench.h"

static FILE *csv_output = NULL;

/** Open CSV benchmark report.
 *
 * @param filename Filename where to store the CSV.
 * @return Whether it was possible to open the file.
 */
errno_t csv_report_open(const char *filename)
{
	csv_output = fopen(filename, "w");
	if (csv_output == NULL) {
		return errno;
	}

	fprintf(csv_output, "benchmark,run,size,duration_nanos\n");

	return EOK;
}

/** Add one entry to the report.
 *
 * When csv_report_open() was not called or failed, the function does
 * nothing.
 *
 * @param run Performance data of the entry.
 * @param run_index Run index, use negative values for warm-up.
 * @param bench Benchmark information.
 * @param workload_size Workload size.
 */
void csv_report_add_entry(bench_run_t *run, int run_index,
    benchmark_t *bench, uint64_t workload_size)
{
	if (csv_output == NULL) {
		return;
	}

	fprintf(csv_output, "%s,%d,%" PRIu64 ",%lld\n",
	    bench->name, run_index, workload_size,
	    (long long) stopwatch_get_nanos(&run->stopwatch));
}

/** Close CSV report.
 *
 * When csv_report_open() was not called or failed, the function does
 * nothing.
 */
void csv_report_close(void)
{
	if (csv_output != NULL) {
		fclose(csv_output);
	}
}

/** @}
 */
