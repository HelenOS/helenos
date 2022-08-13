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

#include <stdarg.h>
#include <stdio.h>
#include "hbench.h"

/** Initialize bench run structure.
 *
 * @param run Structure to intialize.
 * @param error_buffer Pre-allocated memory to use for error messages.
 * @param error_buffer_size Size of error_buffer.
 */
void bench_run_init(bench_run_t *run, char *error_buffer, size_t error_buffer_size)
{
	stopwatch_init(&run->stopwatch);
	run->error_message = error_buffer;
	run->error_message_buffer_size = error_buffer_size;
}

/** Format error message on benchmark failure.
 *
 * This function always returns false so it can be easily used in benchmark
 * runners as
 *
 * @code
 * if (error) {
 *     return bench_run_fail(run, "we failed");
 * }
 * @endcode
 *
 * @param run Current benchmark run.
 * @param fmt printf-style message followed by extra arguments.
 * @retval false Always.
 */
bool bench_run_fail(bench_run_t *run, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(run->error_message, run->error_message_buffer_size, fmt, args);
	va_end(args);

	return false;
}

/** @}
 */
