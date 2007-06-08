/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup arm32	
 * @{
 */
/** @file
 *  @brief Bootinfo declarations.
 *
 *  Reflects boot/arch/arm32/loader/main.h.
 */

#ifndef KERN_arm32_BOOT_H_
#define KERN_arm32_BOOT_H_

#include <arch/types.h>

/** Maximum number of tasks in the #bootinfo_t struct. */
#define TASKMAP_MAX_RECORDS 32


/** Struct holding information about single loaded uspace task. */
typedef struct {

	/** Address where the task was placed. */
	uintptr_t addr;

	/** Size of the task's binary. */
	uint32_t size;
} utask_t;


/** Struct holding information about loaded uspace tasks. */
typedef struct {

	/** Number of loaded tasks. */
	uint32_t cnt;

	/** Array of loaded tasks. */	
	utask_t tasks[TASKMAP_MAX_RECORDS];
} bootinfo_t;


/** Bootinfo that is filled in #kernel_image_start. */ 
extern bootinfo_t bootinfo;


#endif

/** @}
 */
