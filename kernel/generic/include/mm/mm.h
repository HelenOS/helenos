/*
 * Copyright (c) 2007 Martin Decky
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

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

#ifndef KERN_MM_H_
#define KERN_MM_H_

#define PAGE_CACHEABLE_SHIFT		0
#define PAGE_NOT_PRESENT_SHIFT		1
#define PAGE_USER_SHIFT			2
#define PAGE_WRITE_SHIFT		4
#define PAGE_EXEC_SHIFT			5
#define PAGE_GLOBAL_SHIFT		6

#define PAGE_CACHEABLE			(1 << PAGE_CACHEABLE_SHIFT)
#define PAGE_NOT_PRESENT		(1 << PAGE_NOT_PRESENT_SHIFT)
#define PAGE_USER			(1 << PAGE_USER_SHIFT)
#define PAGE_WRITE			(1 << PAGE_WRITE_SHIFT)
#define PAGE_EXEC			(1 << PAGE_EXEC_SHIFT)
#define PAGE_GLOBAL			(1 << PAGE_GLOBAL_SHIFT)

#endif

/** @}
 */
