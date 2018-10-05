/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_PAL_H_
#define KERN_ia64_PAL_H_

#define PAL_OK		 0	/**< Call completed without error. */
#define PAL_UNIMPL	-1	/**< Unimplemented procedure. */
#define PAL_INVARG	-2	/**< Invalid argument. */
#define PAL_ERR		-3	/**< Can not compete call without error. */

/** These are the indices for PAL_PROC. */
#define PAL_CACHE_FLUSH		1
#define PAL_CACHE_INFO		2
#define PAL_CACHE_INIT		3
#define PAL_CACHE_PROT_INFO	38
#define PAL_CACHE_SHARED_INFO	43
#define PAL_CACHE_SUMMARY	4

#define PAL_MEM_ATTRIB		5
#define PAL_PREFETCH_VISIBILITY	41
#define PAL_PTCE_INFO		6
#define PAL_VM_INFO		7
#define PAL_VM_PAGE_SIZE	34
#define PAL_VM_SUMMARY		8
#define PAL_VM_TR_READ		261

#define PAL_BUS_GET_FEATURES	9
#define PAL_BUS_SET_FEATURES	10
#define PAL_DEBUG_INFO		11
#define PAL_FIXED_ADDR		12
#define PAL_FREQ_BASE		13
#define PAL_FREQ_RATIOS		14
#define PAL_LOGICAL_TO_PHYSICAL	42
#define PAL_PERF_MON_INFO	15
#define PAL_PLATFORM_ADDR	16
#define PAL_PROC_GET_FEATURES	17
#define PAL_PROC_SET_FEATURES	18
#define PAL_REGISTER_INFO	39
#define PAL_RSE_INFO		19
#define PAL_VERSION		20

#define PAL_MC_CLEAR_LOG	21
#define PAL_MC_DRAIN		22
#define PAL_MC_DYNAMIC_STATE	24
#define PAL_MC_ERROR_INFO	25
#define PAL_MC_EXPECTED		23
#define PAL_MC_REGISTER_MEM	27
#define PAL_MC_RESUME		26

#define PAL_HALT		28
#define PAL_HALT_INFO		257
#define PAL_HALT_LIGHT		29

#define PAL_CACHE_LINE_INIT	31
#define PAL_CACHE_READ		259
#define PAL_CACHE_WRITE		260
#define PAL_TEST_INFO		37
#define PAL_TEST_PROC		258

#define PAL_COPY_INFO		30
#define PAL_COPY_PAL		256
#define PAL_ENTER_IA_32_ENV	33
#define PAL_PMI_ENTRYPOINT	32

/*
 *	Ski PTCE data
 */
#define PAL_PTCE_INFO_BASE() (0x100000000LL)
#define PAL_PTCE_INFO_COUNT1() (2)
#define PAL_PTCE_INFO_COUNT2() (3)
#define PAL_PTCE_INFO_STRIDE1() (0x10000000)
#define PAL_PTCE_INFO_STRIDE2() (0x2000)

#endif

/** @}
 */
