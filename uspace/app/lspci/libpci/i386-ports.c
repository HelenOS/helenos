/*
 *	The PCI Library -- Direct Configuration access via i386 Ports
 *
 *	Copyright (c) 1997--2004 Martin Mares <mj@ucw.cz>
 *
 *	May 8, 2006 - Modified and ported to HelenOS by Jakub Jermar.
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <unistd.h>
#include <ddi.h>
#include <libarch/ddi.h>

#include "internal.h"

#define PCI_CONF1	0xcf8
#define PCI_CONF1_SIZE	8


static void conf12_init(struct pci_access *a)
{	
}

static void conf12_cleanup(struct pci_access *a UNUSED)
{
}

/*
 * Before we decide to use direct hardware access mechanisms, we try to do some
 * trivial checks to ensure it at least _seems_ to be working -- we just test
 * whether bus 00 contains a host bridge (this is similar to checking
 * techniques used in XFree86, but ours should be more reliable since we
 * attempt to make use of direct access hints provided by the PCI BIOS).
 *
 * This should be close to trivial, but it isn't, because there are buggy
 * chipsets (yes, you guessed it, by Intel and Compaq) that have no class ID.
 */

static int intel_sanity_check(struct pci_access *a, struct pci_methods *m)
{
	struct pci_dev d;

	a->debug("...sanity check");
	d.bus = 0;
	d.func = 0;
	for (d.dev = 0; d.dev < 32; d.dev++) {
		u16 class, vendor;
		if (m->read(&d, PCI_CLASS_DEVICE, (byte *) & class,
			 sizeof(class))
		    && (class == cpu_to_le16(PCI_CLASS_BRIDGE_HOST)
			|| class == cpu_to_le16(PCI_CLASS_DISPLAY_VGA))
		    || m->read(&d, PCI_VENDOR_ID, (byte *) & vendor,
			       sizeof(vendor))
		    && (vendor == cpu_to_le16(PCI_VENDOR_ID_INTEL)
			|| vendor == cpu_to_le16(PCI_VENDOR_ID_COMPAQ))) {
			a->debug("...outside the Asylum at 0/%02x/0",
				 d.dev);
			return 1;
		}
	}
	a->debug("...insane");
	return 0;
}

/*
 *	Configuration type 1
 */

#define CONFIG_CMD(bus, device_fn, where)   (0x80000000 | (bus << 16) | (device_fn << 8) | (where & ~3))

static int conf1_detect(struct pci_access *a)
{
	unsigned int tmp;
	int res = 0;
	
	/*
	 * Gain control over PCI configuration ports.
	 */
	void * addr; 
	if (pio_enable((void *)PCI_CONF1, PCI_CONF1_SIZE, &addr)) {
		return 0;
	}

	pio_write_8(0xCFB, 0x01);
	tmp = pio_read_32(0xCF8);
	pio_write_32(0xCF8, 0x80000000);
	if (pio_read_32(0xCF8) == 0x80000000) {
		res = 1;
	}
	pio_write_32(0xCF8, tmp);
	if (res) {
		res = intel_sanity_check(a, &pm_intel_conf1);
	}
	return res;
}

static int conf1_read(struct pci_dev *d, int pos, byte * buf, int len)
{
	int addr = 0xcfc + (pos & 3);

	if (pos >= 256)
		return 0;

	pio_write_32(0xcf8, 0x80000000 | ((d->bus & 0xff) << 16) |
	     (PCI_DEVFN(d->dev, d->func) << 8) | (pos & ~3));

	switch (len) {
	case 1:
		buf[0] = pio_read_8(addr);
		break;
	case 2:
		((u16 *) buf)[0] = cpu_to_le16(pio_read_16(addr));
		break;
	case 4:
		((u32 *) buf)[0] = cpu_to_le32(pio_read_32(addr));
		break;
	default:
		return pci_generic_block_read(d, pos, buf, len);
	}
	return 1;
}

static int conf1_write(struct pci_dev *d, int pos, byte * buf, int len)
{
	int addr = 0xcfc + (pos & 3);

	if (pos >= 256)
		return 0;

	pio_write_32(0xcf8, 0x80000000 | ((d->bus & 0xff) << 16) |
	     (PCI_DEVFN(d->dev, d->func) << 8) | (pos & ~3));

	switch (len) {
	case 1:
		pio_write_8(addr, buf[0]);
		break;
	case 2:
		pio_write_16(addr, le16_to_cpu(((u16 *) buf)[0]));
		break;
	case 4:
		pio_write_32(addr, le32_to_cpu(((u32 *) buf)[0]));
		break;
	default:
		return pci_generic_block_write(d, pos, buf, len);
	}
	return 1;
}

/*
 *	Configuration type 2. Obsolete and brain-damaged, but existing.
 */

static int conf2_detect(struct pci_access *a)
{
	/*
	 * Gain control over PCI configuration ports.
	 */
	void * addr; 
	if (pio_enable((void *)PCI_CONF1, PCI_CONF1_SIZE, &addr)) {
		return 0;
	}
	if (pio_enable((void *)0xC000, 0x1000, &addr)) {
		return 0;
	}	
	
	/* This is ugly and tends to produce false positives. Beware. */
	pio_write_8(0xCFB, 0x00);
	pio_write_8(0xCF8, 0x00);
	pio_write_8(0xCFA, 0x00);
	if (pio_read_8(0xCF8) == 0x00 && pio_read_8(0xCFA) == 0x00)
		return intel_sanity_check(a, &pm_intel_conf2);
	else
		return 0;
}

static int conf2_read(struct pci_dev *d, int pos, byte * buf, int len)
{
	int addr = 0xc000 | (d->dev << 8) | pos;

	if (pos >= 256)
		return 0;

	if (d->dev >= 16)
		/* conf2 supports only 16 devices per bus */
		return 0;
	pio_write_8(0xcf8, (d->func << 1) | 0xf0);
	pio_write_8(0xcfa, d->bus);
	switch (len) {
	case 1:
		buf[0] = pio_read_8(addr);
		break;
	case 2:
		((u16 *) buf)[0] = cpu_to_le16(pio_read_16(addr));
		break;
	case 4:
		((u32 *) buf)[0] = cpu_to_le32(pio_read_32(addr));
		break;
	default:
		pio_write_8(0xcf8, 0);
		return pci_generic_block_read(d, pos, buf, len);
	}
	pio_write_8(0xcf8, 0);
	return 1;
}

static int conf2_write(struct pci_dev *d, int pos, byte * buf, int len)
{
	int addr = 0xc000 | (d->dev << 8) | pos;

	if (pos >= 256)
		return 0;

	if (d->dev >= 16)
		d->access->error("conf2_write: only first 16 devices exist.");
	pio_write_8(0xcf8, (d->func << 1) | 0xf0);
	pio_write_8(0xcfa, d->bus);
	switch (len) {
	case 1:
		pio_write_8(addr, buf[0]);
		break;
	case 2:
		pio_write_16(addr, le16_to_cpu(*(u16 *) buf));
		break;
	case 4:
		pio_write_32(addr, le32_to_cpu(*(u32 *) buf));
		break;
	default:
		pio_write_8(0xcf8, 0);
		return pci_generic_block_write(d, pos, buf, len);
	}
	pio_write_8(0xcf8, 0);
	return 1;
}

struct pci_methods pm_intel_conf1 = {
	"Intel-conf1",
	NULL,			/* config */
	conf1_detect,
	conf12_init,
	conf12_cleanup,
	pci_generic_scan,
	pci_generic_fill_info,
	conf1_read,
	conf1_write,
	NULL,			/* init_dev */
	NULL			/* cleanup_dev */
};

struct pci_methods pm_intel_conf2 = {
	"Intel-conf2",
	NULL,			/* config */
	conf2_detect,
	conf12_init,
	conf12_cleanup,
	pci_generic_scan,
	pci_generic_fill_info,
	conf2_read,
	conf2_write,
	NULL,			/* init_dev */
	NULL			/* cleanup_dev */
};
