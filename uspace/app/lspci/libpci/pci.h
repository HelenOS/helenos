/*
 *	The PCI Library
 *
 *	Copyright (c) 1997--2005 Martin Mares <mj@ucw.cz>
 *
 *	May 8, 2006 - Modified and ported to HelenOS by Jakub Jermar.
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifndef _PCI_LIB_H
#define _PCI_LIB_H

#include "header.h"
#include "types.h"

#define PCI_LIB_VERSION 0x020200

/*
 *	PCI Access Structure
 */

struct pci_methods;

enum pci_access_type {
	/* Known access methods, remember to update access.c as well */
	PCI_ACCESS_I386_TYPE1,	/* i386 ports, type 1 (params: none) */
	PCI_ACCESS_I386_TYPE2,	/* i386 ports, type 2 (params: none) */
	PCI_ACCESS_US2, 
	PCI_ACCESS_MAX
};

struct pci_access {
	/* Options you can change: */
	unsigned int method;	/* Access method */
	char *method_params[PCI_ACCESS_MAX];	/* Parameters for the methods */
	int writeable;		/* Open in read/write mode */
	int buscentric;		/* Bus-centric view of the world */
	int numeric_ids;	/* Don't resolve device IDs to names */
	int debugging;		/* Turn on debugging messages */

	/* Functions you can override: */
	void (*error) (char *msg, ...);	/* Write error message and quit */
	void (*warning) (char *msg, ...);	/* Write a warning message */
	void (*debug) (char *msg, ...);	/* Write a debugging message */

	struct pci_dev *devices;	/* Devices found on this bus */

	/* Fields used internally: */
	struct pci_methods *methods;
	struct id_entry **id_hash;	/* names.c */
	struct id_bucket *current_id_bucket;
};

/* Initialize PCI access */
struct pci_access *pci_alloc(void);
void pci_init(struct pci_access *);
void pci_cleanup(struct pci_access *);

/* Scanning of devices */
void pci_scan_bus(struct pci_access *acc);
struct pci_dev *pci_get_dev(struct pci_access *acc, int domain, int bus, int dev, int func);	/* Raw access to specified device */
void pci_free_dev(struct pci_dev *);

/*
 *	Devices
 */

struct pci_dev {
	struct pci_dev *next;	/* Next device in the chain */
	u16 domain;		/* PCI domain (host bridge) */
	u8 bus, dev, func;	/* Bus inside domain, device and function */

	/* These fields are set by pci_fill_info() */
	int known_fields;	/* Set of info fields already known */
	u16 vendor_id, device_id;	/* Identity of the device */
	int irq;		/* IRQ number */
	pciaddr_t base_addr[6];	/* Base addresses */
	pciaddr_t size[6];	/* Region sizes */
	pciaddr_t rom_base_addr;	/* Expansion ROM base address */
	pciaddr_t rom_size;	/* Expansion ROM size */

	/* Fields used internally: */
	struct pci_access *access;
	struct pci_methods *methods;
	u8 *cache;		/* Cached config registers */
	int cache_len;
	int hdrtype;		/* Cached low 7 bits of header type, -1 if unknown */
	void *aux;		/* Auxillary data */
};

#define PCI_ADDR_IO_MASK (~(pciaddr_t) 0x3)
#define PCI_ADDR_MEM_MASK (~(pciaddr_t) 0xf)

u8 pci_read_byte(struct pci_dev *, int pos);	/* Access to configuration space */
u16 pci_read_word(struct pci_dev *, int pos);
u32 pci_read_long(struct pci_dev *, int pos);
int pci_read_block(struct pci_dev *, int pos, u8 * buf, int len);
int pci_write_byte(struct pci_dev *, int pos, u8 data);
int pci_write_word(struct pci_dev *, int pos, u16 data);
int pci_write_long(struct pci_dev *, int pos, u32 data);
int pci_write_block(struct pci_dev *, int pos, u8 * buf, int len);

int pci_fill_info(struct pci_dev *, int flags);	/* Fill in device information */

#define PCI_FILL_IDENT		1
#define PCI_FILL_IRQ		2
#define PCI_FILL_BASES		4
#define PCI_FILL_ROM_BASE	8
#define PCI_FILL_SIZES		16
#define PCI_FILL_RESCAN		0x10000

void pci_setup_cache(struct pci_dev *, u8 * cache, int len);

/*
 *	Conversion of PCI ID's to names (according to the pci.ids file)
 *
 *	Call pci_lookup_name() to identify different types of ID's:
 *
 *	VENDOR				(vendorID) -> vendor
 *	DEVICE				(vendorID, deviceID) -> device
 *	VENDOR | DEVICE			(vendorID, deviceID) -> combined vendor and device
 *	SUBSYSTEM | VENDOR		(subvendorID) -> subsystem vendor
 *	SUBSYSTEM | DEVICE		(vendorID, deviceID, subvendorID, subdevID) -> subsystem device
 *	SUBSYSTEM | VENDOR | DEVICE	(vendorID, deviceID, subvendorID, subdevID) -> combined subsystem v+d
 *	SUBSYSTEM | ...			(-1, -1, subvendorID, subdevID) -> generic subsystem
 *	CLASS				(classID) -> class
 *	PROGIF				(classID, progif) -> programming interface
 */

char *pci_lookup_name(struct pci_access *a, char *buf, int size, int flags,
		      ...);

int pci_load_name_list(struct pci_access *a);	/* Called automatically by pci_lookup_*() when needed; returns success */
void pci_free_name_list(struct pci_access *a);	/* Called automatically by pci_cleanup() */

enum pci_lookup_mode {
	PCI_LOOKUP_VENDOR = 1,	/* Vendor name (args: vendorID) */
	PCI_LOOKUP_DEVICE = 2,	/* Device name (args: vendorID, deviceID) */
	PCI_LOOKUP_CLASS = 4,	/* Device class (args: classID) */
	PCI_LOOKUP_SUBSYSTEM = 8,
	PCI_LOOKUP_PROGIF = 16,	/* Programming interface (args: classID, prog_if) */
	PCI_LOOKUP_NUMERIC = 0x10000,	/* Want only formatted numbers; default if access->numeric_ids is set */
	PCI_LOOKUP_NO_NUMBERS = 0x20000	/* Return NULL if not found in the database; default is to print numerically */
};

#endif
