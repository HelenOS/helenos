/*
 * SPDX-FileCopyrightText: 2008 Pavel Rimsky
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief Niagara input/output buffer shared between kernel and user space
 */

#ifdef KERNEL
#include <mm/as.h>
#else
#include <as.h>
#endif

#include <stdint.h>

#define OUTPUT_BUFFER_SIZE  ((PAGE_SIZE) - 2 * 8)

typedef struct {
	uint64_t read_ptr;
	uint64_t write_ptr;
	char data[OUTPUT_BUFFER_SIZE];
} __attribute__((packed)) niagara_output_buffer_t;

#define INPUT_BUFFER_SIZE  ((PAGE_SIZE) - 2 * 8)

typedef struct {
	uint64_t write_ptr;
	uint64_t read_ptr;
	char data[INPUT_BUFFER_SIZE];
} __attribute__((packed)) niagara_input_buffer_t;

/** @}
 */
