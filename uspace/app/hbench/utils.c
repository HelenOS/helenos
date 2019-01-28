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
