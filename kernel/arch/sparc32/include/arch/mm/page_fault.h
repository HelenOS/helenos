/*
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup sparc32mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc32_PAGE_FAULT_H_
#define KERN_sparc32_PAGE_FAULT_H_

typedef enum {
	FAULT_TYPE_LOAD_USER_DATA = 0,
	FAULT_TYPE_LOAD_SUPERVISOR_DATA = 1,
	FAULT_TYPE_EXECUTE_USER = 2,
	FAULT_TYPE_EXECUTE_SUPERVISOR = 3,
	FAULT_TYPE_STORE_USER_DATA = 4,
	FAULT_TYPE_STORE_SUPERVISOR_DATA = 5,
	FAULT_TYPE_STORE_USER_INSTRUCTION = 6,
	FAULT_TYPE_STORE_SUPERVISOR_INSTRUCTION = 7,
} mmu_fault_type_t;

/** MMU Fault Status register. */
typedef struct {
	unsigned int : 14;
	unsigned int ebe : 8;
	unsigned int l : 2;
	unsigned int at : 3;
	unsigned int ft : 3;
	unsigned int fav : 1;
	unsigned int ow : 1;
} __attribute__((packed)) mmu_fault_status_t;

#endif

/** @}
 */
