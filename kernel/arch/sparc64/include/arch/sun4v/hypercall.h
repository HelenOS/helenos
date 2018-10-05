/*
 * Copyright (c) 2008 Pavel Rimsky
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

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief	Macros, constants and functions needed to perform a call to the
 * 		hypervisor API. For details and terminology see this document:
 *		UltraSPARC Virtual Machine Specification (The Hypervisor API
 *		specification for Logical Domains).
 *
 */

#ifndef KERN_sparc64_sun4v_HYPERCALL_H_
#define KERN_sparc64_sun4v_HYPERCALL_H_

/* SW trap numbers for hyperfast traps */
#define FAST_TRAP		0x80
#define MMU_MAP_ADDR		0x83
#define MMU_UNMAP_ADDR		0x84

/* function codes for fast traps */
#define MACH_DESC		0x01
#define CPU_START		0x10
#define CPU_STOP		0x11
#define CPU_YIELD		0x12
#define CPU_QCONF		0x14
#define CPU_MYID		0x16
#define CPU_STATE		0x17
#define CPU_SET_RTBA		0x18
#define CPU_GET_RTBA		0x19
#define MMU_TSB_CTX0		0x20
#define MMU_TSB_CTXNON0		0x21
#define MMU_DEMAP_PAGE		0x22
#define MMU_DEMAP_CTX		0x23
#define MMU_DEMAP_ALL		0x24
#define MMU_MAP_PERM_ADDR	0x25
#define MMU_FAULT_AREA_CONF	0x26
#define MMU_ENABLE		0x27
#define MMU_UNMAP_PERM_ADDR	0x28
#define MMU_TSB_CTX0_INFO	0x29
#define MMU_TSB_CTXNON0_INFO	0x2a
#define MMU_FAULT_AREA_INFO	0x2b
#define CPU_MONDO_SEND		0x42
#define CONS_GETCHAR		0x60
#define CONS_PUTCHAR		0x61

/* return codes */
#define HV_EOK			0	/**< Successful return */
#define HV_ENOCPU		1	/**< Invalid CPU id */
#define HV_ENORADDR		2	/**< Invalid real address */
#define HV_ENOINTR		3	/**< Invalid interrupt id */
#define HV_EBADPGSZ		4	/**< Invalid pagesize encoding */
#define HV_EBADTSB		5	/**< Invalid TSB description */
#define	HV_EINVAL		6	/**< Invalid argument */
#define HV_EBADTRAP		7	/**< Invalid function number */
#define HV_EBADALIGN		8	/**< Invalid address alignment */
#define HV_EWOULDBLOCK		9	/**< Cannot complete operation without blocking */
#define HV_ENOACCESS		10	/**< No access to specified resource */
#define HV_EIO			11	/**< I/O Error */
#define HV_ECPUERROR		12	/**< CPU is in error state */
#define HV_ENOTSUPPORTED	13	/**< Function not supported */
#define HV_ENOMAP		14	/**< No mapping found */
#define HV_ETOOMANY		15	/**< Too many items specified / limit reached */
#define HV_ECHANNEL		16	/**< Invalid LDC channel */
#define HV_EBUSY		17	/**< Operation failed as resource is otherwise busy */

/**
 * Performs a hyperfast hypervisor API call from the assembly language code.
 * Expects the registers %o1-%o4 are properly filled with the arguments of the
 * call.
 *
 * @param function_number	hyperfast call function number
 */
#define __HYPERCALL_FAST(function_number) \
	set function_number, %o5; \
	ta FAST_TRAP;

/**
 * Performs a fast hypervisor API call from the assembly language code.
 * Expects the registers %o1-%o4 are properly filled with the arguments of the
 * call.
 *
 * @param sw_trap_number	software trap number
 */
#define __HYPERCALL_HYPERFAST(sw_trap_number) \
	ta (sw_trap_number);

#ifndef __ASSEMBLER__

#include <stdint.h>

/*
 * Macros to be used from the C-language code; __hypercall_fastN performs
 * a fast hypervisor API call taking exactly N arguments.
 */

#define __hypercall_fast0(function_number) \
	__hypercall_fast(0, 0, 0, 0, 0, function_number)
#define __hypercall_fast1(function_number, p1) \
	__hypercall_fast(p1, 0, 0, 0, 0, function_number)
#define __hypercall_fast2(function_number, p1, p2) \
	__hypercall_fast(p1, p2, 0, 0, 0, function_number)
#define __hypercall_fast3(function_number, p1, p2, p3) \
	__hypercall_fast(p1, p2, p3, 0, 0, function_number)
#define __hypercall_fast4(function_number, p1, p2, p3, p4) \
	__hypercall_fast(p1, p2, p3, p4, 0, function_number)
#define __hypercall_fast5(function_number, p1, p2, p3, p4, p5) \
	__hypercall_fast(p1, p2, p3, p4, p5, function_number)

/**
 * Performs a fast hypervisor API call which returns no value except for the
 * error status.
 *
 * @param p1			the 1st argument of the hypervisor API call
 * @param p2			the 2nd argument of the hypervisor API call
 * @param p3			the 3rd argument of the hypervisor API call
 * @param p4			the 4th argument of the hypervisor API call
 * @param p5			the 5th argument of the hypervisor API call
 * @param function_number	function number of the call
 * @return			error status
 */
static inline uint64_t
__hypercall_fast(const uint64_t p1, const uint64_t p2, const uint64_t p3,
    const uint64_t p4, const uint64_t p5, const uint64_t function_number)
{
	register uint64_t a6 asm("o5") = function_number;
	register uint64_t a1 asm("o0") = p1;
	register uint64_t a2 asm("o1") = p2;
	register uint64_t a3 asm("o2") = p3;
	register uint64_t a4 asm("o3") = p4;
	register uint64_t a5 asm("o4") = p5;

	asm volatile (
	    "ta %7\n"
	    : "=r" (a1)
	    : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6),
	      "i" (FAST_TRAP)
	    : "memory"
	);

	return a1;
}

/**
 * Performs a fast hypervisor API call which can return a value.
 *
 * @param p1			the 1st argument of the hypervisor API call
 * @param p2			the 2nd argument of the hypervisor API call
 * @param p3			the 3rd argument of the hypervisor API call
 * @param p4			the 4th argument of the hypervisor API call
 * @param p5			the 5th argument of the hypervisor API call
 * @param function_number	function number of the call
 * @param ret1			pointer to an address where the return value
 * 				of the hypercall should be saved, or NULL
 * @return			error status
 */
static inline uint64_t
__hypercall_fast_ret1(const uint64_t p1, const uint64_t p2, const uint64_t p3,
    const uint64_t p4, const uint64_t p5, const uint64_t function_number,
    uint64_t *ret1)
{
	register uint64_t a6 asm("o5") = function_number;
	register uint64_t a1 asm("o0") = p1;
	register uint64_t a2 asm("o1") = p2;
	register uint64_t a3 asm("o2") = p3;
	register uint64_t a4 asm("o3") = p4;
	register uint64_t a5 asm("o4") = p5;

	asm volatile (
	    "ta %8\n"
	    : "=r" (a1), "=r" (a2)
	    : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6),
	      "i" (FAST_TRAP)
	    : "memory"
	);

	if (ret1)
		*ret1 = a2;

	return a1;
}

/**
 * Performs a hyperfast hypervisor API call.
 *
 * @param p1			the 1st argument of the hypervisor API call
 * @param p2			the 2nd argument of the hypervisor API call
 * @param p3			the 3rd argument of the hypervisor API call
 * @param p4			the 4th argument of the hypervisor API call
 * @param p5			the 5th argument of the hypervisor API call
 * @param sw_trap_number	software trap number
 */
static inline uint64_t
__hypercall_hyperfast(const uint64_t p1, const uint64_t p2, const uint64_t p3,
    const uint64_t p4, const uint64_t p5, const uint64_t sw_trap_number)
{
	register uint64_t a1 asm("o0") = p1;
	register uint64_t a2 asm("o1") = p2;
	register uint64_t a3 asm("o2") = p3;
	register uint64_t a4 asm("o3") = p4;
	register uint64_t a5 asm("o4") = p5;

	asm volatile (
	    "ta %6\n"
	    : "=r" (a1)
	    : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5),
	      "i" (sw_trap_number)
	    : "memory"
	);

	return a1;
}

#endif /* ASM */

#endif

/** @}
 */
