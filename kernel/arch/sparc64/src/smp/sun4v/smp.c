/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2009 Pavel Rimsky
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#include <smp/smp.h>
#include <smp/ipi.h>
#include <genarch/ofw/ofw_tree.h>
#include <cpu.h>
#include <arch/cpu.h>
#include <arch/boot/boot.h>
#include <arch.h>
#include <config.h>
#include <macros.h>
#include <halt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <synch/waitq.h>
#include <print.h>
#include <arch/sun4v/hypercall.h>
#include <arch/sun4v/md.h>
#include <arch/sun4v/ipi.h>
#include <time/delay.h>
#include <arch/smp/sun4v/smp.h>
#include <str.h>
#include <errno.h>

/** hypervisor code of the "running" state of the CPU */
#define CPU_STATE_RUNNING	2

/** maximum possible number of processor cores */
#define MAX_NUM_CORES		8

/** needed in the CPU_START hypercall */
extern void kernel_image_start(void);

/** needed in the CPU_START hypercall */
extern void *trap_table;

/** number of execution units detected */
uint8_t exec_unit_count = 0;

/** execution units (processor cores) */
exec_unit_t exec_units[MAX_NUM_CORES];

/** CPU structures */
extern cpu_t *cpus;

/** maximum number of strands per a physical core detected */
unsigned int max_core_strands = 0;

#if 0
/**
 * Proposes the optimal number of ready threads for each virtual processor
 * in the given processor core so that the processor core is as busy as the
 * average processor core. The proposed number of ready threads will be
 * stored to the proposed_nrdy variable of the cpu_arch_t struture.
 */
bool calculate_optimal_nrdy(exec_unit_t *exec_unit) {

	/* calculate the number of threads the core will steal */
	int avg = atomic_get(&nrdy) / exec_unit_count;
	int to_steal = avg - atomic_get(&(exec_units->nrdy));
	if (to_steal < 0) {
		return true;
	} else if (to_steal == 0) {
		return false;
	}

	/* initialize the proposals with the real numbers of ready threads */
	unsigned int k;
	for (k = 0; k < exec_unit->strand_count; k++) {
		exec_units->cpus[k]->arch.proposed_nrdy =
			atomic_get(&(exec_unit->cpus[k]->nrdy));
	}

	/* distribute the threads to be stolen to the core's CPUs */
	int j;
	for (j = to_steal; j > 0; j--) {
		unsigned int k;
		unsigned int least_busy = 0;
		unsigned int least_busy_nrdy =
			exec_unit->cpus[0]->arch.proposed_nrdy;

		/* for each stolen thread, give it to the least busy CPU */
		for (k = 0; k < exec_unit->strand_count; k++) {
			if (exec_unit->cpus[k]->arch.proposed_nrdy
					< least_busy_nrdy) {
				least_busy = k;
				least_busy_nrdy =
					exec_unit->cpus[k]->arch.proposed_nrdy;
			}
		}
		exec_unit->cpus[least_busy]->arch.proposed_nrdy++;
	}

	return false;
}
#endif

/**
 * Finds out which execution units belong to particular CPUs. By execution unit
 * we mean the physical core the logical processor is backed by. Since each
 * Niagara physical core has just one integer execution unit and we will
 * ignore other execution units than the integer ones, we will use the terms
 * "integer execution unit", "execution unit" and "physical core"
 * interchangeably.
 *
 * The physical cores are detected by browsing the children of the CPU node
 * in the machine description and looking for a node representing an integer
 * execution unit. Once the integer execution unit of a particular CPU is
 * known, the ID of the CPU is added to the list of cpuids of the corresponding
 * execution unit structure (exec_unit_t). If an execution unit is encountered
 * for the first time, a new execution unit structure (exec_unit_t) must be
 * created first and added to the execution units array (exec_units).
 *
 * If the function fails to find an execution unit for a CPU (this may happen
 * on machines with older firmware or on Simics), it performs a fallback code
 * which pretends there exists just one execution unit and all CPUs belong to
 * it.
 *
 * Finally, the array of all execution units is reordered such that its element
 * which represents the physical core of the the bootstrap CPU is at index 0.
 * Moreover, the array of CPU IDs within the BSP's physical core structure is
 * reordered such that the element which represents the ID of the BSP is at
 * index 0. This is done because we would like the CPUs to be woken up
 * such that the 0-index CPU of the 0-index execution unit is
 * woken up first. And since the BSP is already woken up, we would like it to be
 * at 0-th position of the 0-th execution unit structure.
 *
 * Apart from that, the code also counts the total number of CPUs and stores
 * it to the global config.cpu_count variable.
 */
static void detect_execution_units(void)
{
	/* ID of the bootstrap processor */
	uint64_t myid;

	/* total number of CPUs detected */
	size_t cpu_count = 0;

	/* will be set to 1 if detecting the physical cores fails */
	bool exec_unit_assign_error = 0;

	/* index of the bootstrap physical core in the array of cores */
	unsigned int bsp_exec_unit_index = 0;

	/* index of the BSP ID inside the array of bootstrap core's cpuids */
	unsigned int bsp_core_strand_index = 0;

	__hypercall_fast_ret1(0, 0, 0, 0, 0, CPU_MYID, &myid);
	md_node_t node = md_get_root();

	/* walk through all the CPU nodes in the MD*/
	while (md_next_node(&node, "cpu")) {

		uint64_t cpuid;
		md_get_integer_property(node, "id", &cpuid);
		cpu_count++;

		/*
 		 * if failed in previous CPUs, don't try
 		 * to detect physical cores any more
 		 */
		if (exec_unit_assign_error)
			continue;

		/* detect exec. unit for the CPU represented by current node */
		uint64_t exec_unit_id = 0;
		md_child_iter_t it = md_get_child_iterator(node);

		while (md_next_child(&it)) {
			md_node_t child = md_get_child_node(it);
			const char *exec_unit_type = "";
			md_get_string_property(child, "type", &exec_unit_type);

			/* each physical core has just 1 integer exec. unit */
			if (str_cmp(exec_unit_type, "integer") == 0) {
				exec_unit_id = child;
				break;
			}
		}

		/* execution unit detected successfully */
		if (exec_unit_id != 0) {

			/* find the exec. unit in array of existing units */
			unsigned int i = 0;
			for (i = 0; i < exec_unit_count; i++) {
				if (exec_units[i].exec_unit_id == exec_unit_id)
					break;
			}

			/*
			 * execution unit just met has not been met before, so
			 * create a new entry in array of all execution units
			 */
			if (i == exec_unit_count) {
				exec_units[i].exec_unit_id = exec_unit_id;
				exec_units[i].strand_count = 0;
				atomic_set(&(exec_units[i].nrdy), 0);
				spinlock_initialize(&(exec_units[i].proposed_nrdy_lock), "exec_units[].proposed_nrdy_lock");
				exec_unit_count++;
			}

			/*
			 * remember the exec. unit and strand of the BSP
			 */
			if (cpuid == myid) {
				bsp_exec_unit_index = i;
				bsp_core_strand_index = exec_units[i].strand_count;
			}

			/* add the CPU just met to the exec. unit's list */
			exec_units[i].cpuids[exec_units[i].strand_count] = cpuid;
			exec_units[i].strand_count++;
			max_core_strands =
				exec_units[i].strand_count > max_core_strands ?
				exec_units[i].strand_count : max_core_strands;

		/* detecting execution unit failed */
		} else {
			exec_unit_assign_error = 1;
		}
	}		

	/* save the number of CPUs to a globally accessible variable */
	config.cpu_count = cpu_count;

	/*
 	 * A fallback code which will be executed if finding out which
 	 * execution units belong to particular CPUs fails. Pretend there
 	 * exists just one execution unit and all CPUs belong to it.
 	 */
	if (exec_unit_assign_error) {
		bsp_exec_unit_index = 0;
		exec_unit_count = 1;
		exec_units[0].strand_count = cpu_count;
		exec_units[0].exec_unit_id = 1;
		spinlock_initialize(&(exec_units[0].proposed_nrdy_lock), "exec_units[0].proposed_nrdy_lock");
		atomic_set(&(exec_units[0].nrdy), 0);
		max_core_strands = cpu_count;

		/* browse CPUs again, assign them the fictional exec. unit */
		node = md_get_root();
		unsigned int i = 0;

		while (md_next_node(&node, "cpu")) {
			uint64_t cpuid;
			md_get_integer_property(node, "id", &cpuid);
			if (cpuid == myid) {
				bsp_core_strand_index = i;
			}
			exec_units[0].cpuids[i++] = cpuid;
		}
	}

	/*
 	 * Reorder the execution units array elements and the cpuid array
 	 * elements so that the BSP will always be the very first CPU of
 	 * the very first execution unit.
 	 */
	exec_unit_t temp_exec_unit = exec_units[0];
	exec_units[0] = exec_units[bsp_exec_unit_index];
	exec_units[bsp_exec_unit_index] = temp_exec_unit;

	uint64_t temp_cpuid = exec_units[0].cpuids[0];
	exec_units[0].cpuids[0] = exec_units[0].cpuids[bsp_exec_unit_index];
	exec_units[0].cpuids[bsp_core_strand_index] = temp_cpuid;

}

/**
 * Determine number of processors and detect physical cores. On Simics
 * copy the code which will be executed by the AP when the BSP sends an
 * IPI to it in order to make it execute HelenOS code.
 */
void smp_init(void)
{
	detect_execution_units();
}

/**
 * For each CPU sets the value of cpus[i].arch.id, where i is the
 * index of the CPU in the cpus variable, to the cpuid of the i-th processor
 * to be run. The CPUs are run such that the CPU represented by cpus[0]
 * is run first, cpus[1] is run after it, and cpus[cpu_count - 1] is run as the
 * last one.
 *
 * The CPU IDs are set such that during waking the CPUs up the
 * processor cores will be alternated, i.e. first one CPU from the first core
 * will be run, after that one CPU from the second CPU core will be run,...
 * then one CPU from the last core will be run, after that another CPU
 * from the first core will be run, then another CPU from the second core
 * will be run,... then another CPU from the last core will be run, and so on.
 */
static void init_cpuids(void)
{
	unsigned int cur_core_strand;
	unsigned int cur_core;
	unsigned int cur_cpu = 0;

	for (cur_core_strand = 0; cur_core_strand < max_core_strands; cur_core_strand++) {
		for (cur_core = 0; cur_core < exec_unit_count; cur_core++) {
			if (cur_core_strand > exec_units[cur_core].strand_count)
				continue;

			cpus[cur_cpu].arch.exec_unit = &(exec_units[cur_core]);
			atomic_add(&(exec_units[cur_core].nrdy), atomic_get(&(cpus[cur_cpu].nrdy)));
			cpus[cur_cpu].arch.id = exec_units[cur_core].cpuids[cur_core_strand];
			exec_units[cur_core].cpus[cur_core_strand] = &(cpus[cur_cpu]);
			cur_cpu++;
		}
	}
}

/**
 * Wakes up a single CPU.
 *
 * @param cpuid	ID of the CPU to be woken up
 */
static bool wake_cpu(uint64_t cpuid)
{
#ifdef CONFIG_SIMICS_SMP_HACK
	ipi_unicast_to((void (*)(void)) 1234, cpuid);
#else
	/* stop the CPU before making it execute our code */
	if (__hypercall_fast1(CPU_STOP, cpuid) != EOK)
		return false;
	
	/* wait for the CPU to stop */
	uint64_t state;
	__hypercall_fast_ret1(cpuid, 0, 0, 0, 0, CPU_STATE, &state);
	while (state == CPU_STATE_RUNNING)
		__hypercall_fast_ret1(cpuid, 0, 0, 0, 0, CPU_STATE, &state);
	
	/* make the CPU run again and execute HelenOS code */
	if (__hypercall_fast4(CPU_START, cpuid,
	    (uint64_t) KA2PA(kernel_image_start), KA2PA(trap_table),
	    physmem_base) != EOK)
		return false;
#endif
	
	if (waitq_sleep_timeout(&ap_completion_wq, 10000000,
	    SYNCH_FLAGS_NONE, NULL) == ETIMEOUT)
		printf("%s: waiting for processor (cpuid = %" PRIu64 ") timed out\n",
		    __func__, cpuid);
	
	return true;
}

/** Wake application processors up. */
void kmp(void *arg)
{
	init_cpuids();

	unsigned int i;

	for (i = 1; i < config.cpu_count; i++) {
		wake_cpu(cpus[i].arch.id);
	}
}

/** @}
 */
