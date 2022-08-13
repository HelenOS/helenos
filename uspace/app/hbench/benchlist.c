/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 * SPDX-FileCopyrightText: 2018 Vojtech Horky
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
#include "hbench.h"

benchmark_t *benchmarks[] = {
	&benchmark_dir_read,
	&benchmark_fibril_mutex,
	&benchmark_file_read,
	&benchmark_malloc1,
	&benchmark_malloc2,
	&benchmark_ns_ping,
	&benchmark_ping_pong
};

size_t benchmark_count = sizeof(benchmarks) / sizeof(benchmarks[0]);

/** @}
 */
