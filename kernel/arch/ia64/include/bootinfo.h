/*
 * Copyright (c) 2005 Martin Decky
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

#ifndef KERN_ia64_BOOTINFO_H_
#define KERN_ia64_BOOTINFO_H_

#define BOOTINFO_ADDRESS 0x4401000

#define CONFIG_INIT_TASKS	32

#define MEMMAP_ITEMS 128

#define EFI_MEMMAP_FREE_MEM 0
#define EFI_MEMMAP_IO 1
#define EFI_MEMMAP_IO_PORTS 2


typedef struct {
	void *addr; 
	unsigned long size;
} binit_task_t;
	
typedef struct {
	unsigned long count;
	binit_task_t tasks[CONFIG_INIT_TASKS];
} binit_t;

typedef struct {
	unsigned int type;
	unsigned long base;
	unsigned long size;
}efi_memmap_item_t;


typedef struct {
	binit_t taskmap;

	efi_memmap_item_t memmap[MEMMAP_ITEMS];
	unsigned int memmap_items;

	unsigned long * sapic;
	unsigned long sys_freq;
	unsigned long freq_scale;
	unsigned int wakeup_intno;

} bootinfo_t;

extern bootinfo_t *bootinfo;

extern void start(void);
extern void bootstrap(void);

#endif
