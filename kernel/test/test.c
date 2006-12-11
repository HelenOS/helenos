/*
 * Copyright (C) 2006 Martin Decky
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

/** @addtogroup test
 * @{
 */
/** @file
 */

#include <test.h>

test_t tests[] = {
	{
		"atomic1",
		"Test atomic operations",
		&test_atomic1,
		true
	},
	{
		"btree1",
		"Test B-tree operations",
		&test_btree1,
		true
	},
	{
		"mips1",
		"MIPS debug test",
		&test_mips1,
		true
	},
	{
		"fault1",
		"Write to NULL (maybe page fault)",
		&test_fault1,
		false
	},
	{
		"fpu1",
		"Intel FPU test",
		&test_fpu1,
		true
	},
	{
		"sse1",
		"Intel SEE test",
		&test_sse1,
		true
	},
	{
		"mips2",
		"MIPS FPU test",
		&test_mips2,
		true
	},
	{
		"falloc1",
		"Frame allocator test 1",
		&test_falloc1,
		true
	},
	{
		"falloc2",
		"Frame allocator test 2",
		&test_falloc2,
		true
	},
	{
		"mapping1",
		"Mapping test",
		&test_mapping1,
		true
	},
	{
		"slab1",
		"SLAB test 1",
		&test_slab1,
		true
	},
	{
		"slab2",
		"SLAB test 2",
		&test_slab2,
		true
	},
	{
		"purge1",
		"Itanium TLB purge test",
		&test_purge1,
		true
	},
	{
		"rwlock1",
		"RW-lock test 1",
		&test_rwlock1,
		true
	},
	{
		"rwlock2",
		"RW-lock test 2",
		&test_rwlock2,
		true
	},
	{
		"rwlock3",
		"RW-lock test 3",
		&test_rwlock3,
		true
	},
	{
		"rwlock4",
		"RW-lock test 4",
		&test_rwlock4,
		true
	},
	{
		"rwlock5",
		"RW-lock test 5",
		&test_rwlock5,
		true
	},
	{
		"semaphore1",
		"Semaphore test 1",
		&test_semaphore1,
		true
	},
	{
		"semaphore2",
		"Semaphore test 2",
		&test_semaphore2,
		true
	},
	{
		"print1",
		"Printf test",
		&test_print1,
		true
	},
	{
		"thread1",
		"Thread test",
		&test_thread1,
		true
	},
	{
		"sysinfo1",
		"Sysinfo test",
		&test_sysinfo1,
		true
	},
	{NULL, NULL, NULL}
};

/** @}
 */
