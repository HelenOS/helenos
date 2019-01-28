/*
 * Copyright (c) 2019 Vojtech Horky
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
