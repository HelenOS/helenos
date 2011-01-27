/*
 * Copyright (c) 2010 Matus Dekanek
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

#ifndef USBMEM_H
#define	USBMEM_H


// group should be changed - this is not usb specific
/** @addtogroup usb
 * @{
 */
/** @file definitions of special memory management, used mostly in usb stack
 *
 * USB HCD needs traslation between physical and virtual addresses. These
 * functions implement such functionality. For each allocated virtual address
 * the memory manager gets also it`s physical translation and remembers it.
 * Addresses allocated byt this manager can be therefore translated from and to
 * physical addresses.
 * Typical use:
 * void * address = mman_malloc(some_size);
 * void * physical_address = mman_getPA(address);
 * void * the_same_address = mman_getVA(physical_address);
 * void * null_address = mman_getPA(non_existing_address);
 * mman_free(address);
 * // physical_address, adress and the_same_address are no longer valid here
 *
 *
 * @note Addresses allocated by this memory manager should be as well
 * deallocated byt it.
 *
 */

#include <sys/types.h>

extern void * mman_malloc(
		size_t size,
		size_t alignment,
		unsigned long max_physical_address);

extern void * mman_getVA(void * addr);

extern void * mman_getPA(void * addr);

extern void mman_free(void * addr);






/** @}
 */


#endif	/* USBMEM_H */

