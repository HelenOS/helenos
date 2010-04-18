/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef KERN_ABI_H_
#define KERN_ABI_H_

#define LOAD_STEPS        3
#define TASK_NAME_BUFLEN  20

typedef struct {
	unsigned int id;
	uint16_t frequency_mhz;
	uint64_t idle_ticks;
	uint64_t busy_ticks;
} stats_cpu_t;

typedef struct {
	uint64_t total;
	uint64_t unavail;
	uint64_t used;
	uint64_t free;
} stats_physmem_t;

typedef struct {
	uint64_t call_sent;
	uint64_t call_recieved;
	uint64_t answer_sent;
	uint64_t answer_recieved;
	uint64_t irq_notif_recieved;
	uint64_t forwarded;
} stats_ipc_t;

typedef struct {
	char name[TASK_NAME_BUFLEN];
	size_t virtmem;
	size_t threads;
	uint64_t ucycles;
	uint64_t kcycles;
	stats_ipc_t ipc_info;
} stats_task_t;

typedef uint32_t load_t;

#endif

/** @}
 */
