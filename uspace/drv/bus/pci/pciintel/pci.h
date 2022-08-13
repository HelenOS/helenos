/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup pciintel
 * @{
 */
/** @file
 */

#ifndef PCI_H_
#define PCI_H_

#include <adt/list.h>
#include <ddi.h>
#include <ddf/driver.h>
#include <fibril_synch.h>

#define PCI_MAX_HW_RES 10

typedef struct pciintel_bus {
	/** DDF device node */
	ddf_dev_t *dnode;
	ioport32_t *conf_addr_reg;
	ioport32_t *conf_data_reg;
	ioport32_t *conf_space;
	pio_window_t pio_win;
	fibril_mutex_t conf_mutex;
	/** List of functions (of pci_fun_t) */
	list_t funs;
} pci_bus_t;

typedef struct pci_fun_data {
	pci_bus_t *busptr;
	ddf_fun_t *fnode;
	/** Link to @c busptr->funs */
	link_t lfuns;

	int bus;
	int dev;
	int fn;
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint8_t class_code;
	uint8_t subclass_code;
	uint8_t prog_if;
	uint8_t revision;
	hw_resource_list_t hw_resources;
	hw_resource_t resources[PCI_MAX_HW_RES];
	pio_window_t pio_window;
} pci_fun_t;

extern pci_bus_t *pci_bus(ddf_dev_t *);

extern void pci_fun_create_match_ids(pci_fun_t *);
extern pci_fun_t *pci_fun_first(pci_bus_t *);
extern pci_fun_t *pci_fun_next(pci_fun_t *);

extern uint8_t pci_conf_read_8(pci_fun_t *, int);
extern uint16_t pci_conf_read_16(pci_fun_t *, int);
extern uint32_t pci_conf_read_32(pci_fun_t *, int);
extern void pci_conf_write_8(pci_fun_t *, int, uint8_t);
extern void pci_conf_write_16(pci_fun_t *, int, uint16_t);
extern void pci_conf_write_32(pci_fun_t *, int, uint32_t);

extern void pci_add_range(pci_fun_t *, uint64_t, size_t, bool);
extern int pci_read_bar(pci_fun_t *, int);
extern void pci_read_interrupt(pci_fun_t *);
extern void pci_add_interrupt(pci_fun_t *, int);

extern pci_fun_t *pci_fun_new(pci_bus_t *);
extern void pci_fun_init(pci_fun_t *, int, int, int);
extern void pci_fun_delete(pci_fun_t *);
extern char *pci_fun_create_name(pci_fun_t *);

extern errno_t pci_bus_scan(pci_bus_t *, int);

extern bool pci_alloc_resource_list(pci_fun_t *);
extern void pci_clean_resource_list(pci_fun_t *);

extern void pci_read_bars(pci_fun_t *);
extern size_t pci_bar_mask_to_size(uint32_t);

#endif

/**
 * @}
 */
