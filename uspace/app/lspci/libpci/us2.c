#include <unistd.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <stdio.h>

#include "internal.h"

/* physical addresses and offsets */
//#define U2P_BASE        0x1FE00000000
#define U2P_BASE        0x1c800000000
#define PCI_CONF_OFFSET   0x001000000
#define PCI_CONF_SIZE     0x001000000
#define PCI_CONF_BASE  (U2P_BASE + PCI_CONF_OFFSET)

/* virtual address of PCI configuration space */
static void *conf_addr = 0;  

/* 
 * virtual address of specified PCI configuration register:
 * bus ... bus number (0 for top level PCI bus B, 1 for top level PCI bus A)
 * dev ... device number (0 - 31)
 * fn  ... function number (0 - 7)
 * reg ... register number (register's position within PCI configuration header) 
 **/
#define CONF_ADDR(bus, dev, fn, reg)   ((void *)(conf_addr + ((bus << 16) | (dev << 11) | (fn << 8) | reg)))



static void us2_init(struct pci_access *a)
{	
}

static void us2_cleanup(struct pci_access *a UNUSED)
{
}

static int us2_detect(struct pci_access *a)
{	
	/*
	 * Gain control over PCI configuration ports.
	 */ 
	if (pio_enable((void *)PCI_CONF_BASE, PCI_CONF_SIZE, &conf_addr)) {
		return 0;
	}	

	unsigned int vendor_id = le16_to_cpu(pio_read_16(CONF_ADDR(0, 0, 0, PCI_VENDOR_ID)));	
	unsigned int device_id = le16_to_cpu(pio_read_16(CONF_ADDR(0, 0, 0, PCI_DEVICE_ID)));
	
	// printf("PCI: vendor id = %x\n", vendor_id);
	// printf("PCI: device id = %x\n", device_id);

	return vendor_id == 0x108E && device_id == 0x8000; // should be Psycho from Sun Microsystems 
}


static int us2_read(struct pci_dev *d, int pos, byte * buf, int len)
{	
	if (pos >= 256)
		return 0;
	
	/* The vendor ID and the device ID registers of the device number 0 (the bridge) 
	 * work in simics in different way than the other config. registers. */
	if (d->dev == 0 && d->func == 0 && pos == 0 && len == 4) {    
		((u32 *) buf)[0] = pio_read_32(CONF_ADDR(d->bus, d->dev, d->func, pos));
		return 1;
	}
	
	int i;
	for (i = 0; i < len; i++) {
		buf[i] = pio_read_8(CONF_ADDR(d->bus, d->dev, d->func, pos + i));
	}
	return 1;
}

static int us2_write(struct pci_dev *d, int pos, byte * buf, int len)
{
	if (pos >= 256)
		return 0;

	int i;
	for (i = 0; i < len; i++) {
		 pio_write_8(CONF_ADDR(d->bus, d->dev, d->func, pos + i), buf[i]);
	}
	return 1;
}


struct pci_methods pm_us2 = {
	"Ultra Sparc IIi",
	NULL,			/* config */
	us2_detect,
	us2_init,
	us2_cleanup,
	pci_generic_scan,
	pci_generic_fill_info,
	us2_read,
	us2_write,
	NULL,			/* init_dev */
	NULL			/* cleanup_dev */
};