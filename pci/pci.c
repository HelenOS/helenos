/*
 * HelenOS PCI driver.
 *
 * Copyright (C) 1997-2003 Martin Mares
 * Copyright (C) 2006 Jakub Jermar
 *
 * (Based on libpci example.c written by Martin Mares.)
 *
 * Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <stdio.h>
#include <ddi.h>
#include <task.h>
#include <stdlib.h>
#include <ipc.h>
#include <errno.h>

#include "libpci/pci.h"

#define PCI_CONF1	0xcf8
#define PCI_CONF1_SIZE	8

#define NAME		"PCI"

int main(int argc, char *argv[])
{
	struct pci_access *pacc;
	struct pci_dev *dev;
	unsigned int c;
	char buf[80];

	int ipc_res;
	ipcarg_t ns_phone_addr;

	printf("%s: HelenOS PCI driver\n", NAME);

	/*
	 * Gain control over PCI configuration ports.
	 */
	iospace_enable(task_get_id(), (void *) PCI_CONF1, PCI_CONF1_SIZE);

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
	pci_cleanup(pacc);            /* Close everything */

	printf("%s: registering at naming service.\n", NAME);
	if (ipc_connect_to_me(PHONE_NS, 40, 70, &ns_phone_addr) != 0) {
		printf("Failed to register %s at naming service.\n", NAME);
		return -1;
	}
	
	printf("%s: accepting connections\n", NAME);
	while (1) {
		ipc_call_t call;
		ipc_callid_t callid;
		
		callid = ipc_wait_for_call(&call, 0);
		ipc_answer(callid, EHANGUP, 0, 0);
	}
	return 0;
}
