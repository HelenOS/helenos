/*
 * Copyright (c) 2010 Stanislav Kozina
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

/** @addtogroup genericps
 * @{
 */

/**
 * @file
 * @brief	CPU listing.
 */

#include <ps/ps.h>
#include <ps/cpuinfo.h>
#include <arch/asm.h>
#include <cpu.h>
#include <syscall/copy.h>

#define WRITE_CPU_INFO(dst, i, src) copy_to_uspace(dst+i, src, sizeof(uspace_cpu_info_t))

int sys_ps_get_cpu_info(uspace_cpu_info_t *uspace_cpu)
{
	size_t i;
	uspace_cpu_info_t cpuinfo;
	ipl_t ipl;
	ipl = interrupts_disable();

	for (i = 0; i < config.cpu_count; ++i) {
		spinlock_lock(&cpus[i].lock);
		cpuinfo.id = cpus[i].id;
		cpuinfo.frequency_mhz = cpus[i].frequency_mhz;
		cpuinfo.busy_ticks = cpus[i].busy_ticks;
		cpuinfo.idle_ticks = cpus[i].idle_ticks;
		spinlock_unlock(&cpus[i].lock);
		WRITE_CPU_INFO(uspace_cpu, i, &cpuinfo);
	}

	interrupts_restore(ipl);
	return 0;
}

/** @}
 */
