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

#ifndef BOOT_OFW_H_
#define BOOT_OFW_H_

#include <types.h>
#include <stdarg.h>

#define BUF_SIZE		1024

#define MEMMAP_MAX_RECORDS 	32

#define MAX_OFW_ARGS            12

typedef unative_t ofw_arg_t;
typedef unsigned int ihandle;
typedef unsigned int phandle;

/** OpenFirmware command structure
 *
 */
typedef struct {
	ofw_arg_t service;		/**< Command name. */
	ofw_arg_t nargs;		/**< Number of in arguments. */
	ofw_arg_t nret;			/**< Number of out arguments. */
	ofw_arg_t args[MAX_OFW_ARGS];	/**< List of arguments. */
} ofw_args_t;

typedef struct {
	void *start;
	uint32_t size;
} memzone_t;

typedef struct {
	uint32_t total;
	uint32_t count;
	memzone_t zones[MEMMAP_MAX_RECORDS];
} memmap_t;

typedef struct {
	void *addr;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t scanline;
} screen_t;

typedef struct {
	void *addr;
	uint32_t size;
} keyboard_t;

typedef struct {
	uint32_t info;
	uint32_t addr_hi;
	uint32_t addr_lo;
} pci_addr_t;

typedef struct {
	pci_addr_t addr;
	uint32_t size_hi;
	uint32_t size_lo;
} pci_reg_t;

extern uintptr_t ofw_cif;

extern phandle ofw_chosen;
extern ihandle ofw_stdout;
extern phandle ofw_root;
extern ihandle ofw_mmu;
extern phandle ofw_memory;
extern phandle ofw_aliases;

extern void ofw_init(void);

extern void ofw_write(const char *str, const int len);

extern int ofw_get_property(const phandle device, const char *name, void *buf, const int buflen);
extern int ofw_get_proplen(const phandle device, const char *name);
extern int ofw_next_property(const phandle device, char *previous, char *buf);

extern phandle ofw_get_child_node(const phandle node);
extern phandle ofw_get_peer_node(const phandle node);
extern phandle ofw_find_device(const char *name);

extern int ofw_package_to_path(const phandle device, char *buf, const int buflen);

extern int ofw(ofw_args_t *arg);
extern unsigned long ofw_call(const char *service, const int nargs, const int nret, ofw_arg_t *rets, ...);
extern unsigned int ofw_get_address_cells(const phandle device);
extern unsigned int ofw_get_size_cells(const phandle device);
extern void *ofw_translate(const void *virt);
extern int ofw_translate_failed(ofw_arg_t flag);
extern void *ofw_claim_virt(const void *virt, const int len);
extern void *ofw_claim_phys(const void *virt, const int len);
extern int ofw_map(const void *phys, const void *virt, const int size, const int mode);
extern int ofw_memmap(memmap_t *map);
extern int ofw_screen(screen_t *screen);
extern int ofw_keyboard(keyboard_t *keyboard);
extern int setup_palette(void);
extern void ofw_quiesce(void);

#endif
