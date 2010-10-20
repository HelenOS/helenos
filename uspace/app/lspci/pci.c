/*
 * HelenOS PCI driver.
 *
 * (Based on public domain libpci example.c written by Martin Mares.)
 * Copyright (c) 2006 Jakub Jermar
 *
 * Can be freely distributed and used under the terms of the GNU GPL.
 */

/**
 * @addtogroup pci
 * @{
 */

#include <stdio.h>
#include <ddi.h>
#include <task.h>
#include <stdlib.h>
#include <errno.h>

#include "libpci/pci.h"

#define NAME		"PCI"

static struct pci_access *pacc;

int main(int argc, char *argv[])
{
	struct pci_dev *dev;
	unsigned int c;
	char buf[80];
	
	pacc = pci_alloc();           /* Get the pci_access structure */
	pci_init(pacc);               /* Initialize the PCI library */
	pci_scan_bus(pacc);           /* We want to get the list of devices */
	for(dev=pacc->devices; dev; dev=dev->next) {   /* Iterate over all devices */
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_IRQ);
		c = pci_read_word(dev, PCI_CLASS_DEVICE); /* Read config register directly */
		printf("%02x:%02x.%d vendor=%04x device=%04x class=%04x irq=%d base0=%lx\n",
			dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id,
			c, dev->irq, dev->base_addr[0]);
		printf("\t%s\n", pci_lookup_name(pacc, buf, sizeof(buf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
			dev->vendor_id, dev->device_id));
	}

	pci_cleanup(pacc);
	return 0;
}

/**
 * @}
 */
