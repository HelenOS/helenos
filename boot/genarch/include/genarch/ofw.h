/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_OFW_H_
#define BOOT_OFW_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <_bits/native.h>

#define MEMMAP_MAX_RECORDS  32
#define MAX_OFW_ARGS        12

#define OFW_TREE_PATH_MAX_LEN           256
#define OFW_TREE_PROPERTY_MAX_NAMELEN   32
#define OFW_TREE_PROPERTY_MAX_VALUELEN  64

typedef sysarg_t ofw_arg_t;
typedef native_t ofw_ret_t;
typedef uint32_t ofw_prop_t;
typedef uint32_t ihandle;
typedef uint32_t phandle;

/** OpenFirmware command structure
 *
 */
typedef struct {
	ofw_arg_t service;             /**< Command name. */
	ofw_arg_t nargs;               /**< Number of in arguments. */
	ofw_arg_t nret;                /**< Number of out arguments. */
	ofw_arg_t args[MAX_OFW_ARGS];  /**< List of arguments. */
} ofw_args_t;

typedef struct {
	void *start;
	size_t size;
} memzone_t;

typedef struct {
	uint64_t total;
	size_t cnt;
	memzone_t zones[MEMMAP_MAX_RECORDS];
} memmap_t;

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

extern void ofw_init(void);

extern void ofw_putchar(char);

extern ofw_arg_t ofw_get_property(const phandle, const char *, void *,
    const size_t);
extern ofw_arg_t ofw_get_proplen(const phandle, const char *);
extern ofw_arg_t ofw_next_property(const phandle, char *, char *);

extern phandle ofw_get_child_node(const phandle);
extern phandle ofw_get_peer_node(const phandle);
extern phandle ofw_find_device(const char *);

extern ofw_arg_t ofw_package_to_path(const phandle, char *, const size_t);

extern ofw_arg_t ofw(ofw_args_t *);
extern ofw_arg_t ofw_call(const char *, const size_t, const size_t, ofw_arg_t *,
    ...);

extern size_t ofw_get_address_cells(const phandle);
extern size_t ofw_get_size_cells(const phandle);

extern void *ofw_translate(const void *);

extern void ofw_claim_virt(const void *, const size_t);
extern void *ofw_claim_virt_any(const size_t, const size_t);

extern void ofw_claim_phys(const void *, const size_t);
extern void *ofw_claim_phys_any(const size_t, const size_t);

extern void ofw_map(const void *, const void *, const size_t,
    const ofw_arg_t);

extern void ofw_alloc(const char *, void **, void **, const size_t, void *);

extern void ofw_memmap(memmap_t *);
extern void ofw_setup_screens(void);
extern void ofw_quiesce(void);

#endif
