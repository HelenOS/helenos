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

/**
 * @addtogroup pci
 * @{
 */

#include <stdio.h>
#include <ddi.h>
#include <task.h>
#include <stdlib.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <errno.h>

#include "libpci/pci.h"

#define PCI_CONF1	0xcf8
#define PCI_CONF1_SIZE	8

#define NAME		"PCI"

static struct pci_access *pacc;

int main(int argc, char *argv[])
{
	struct pci_dev *dev;
	unsigned int c;
	char buf[80];
	ipcarg_t ns_in_phone_hash;

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

	printf("%s: registering at naming service.\n", NAME);
	if (ipc_connect_to_me(PHONE_NS, SERVICE_PCI, 0, &ns_in_phone_hash) != 0) {
		printf("Failed to register %s at naming service.\n", NAME);
		return -1;
	}

	printf("%s: accepting connections\n", NAME);
	while (1) {		
		ipc_call_t call;
		ipc_callid_t callid;

		callid = ipc_wait_for_call(&call);
		switch(IPC_GET_METHOD(call)) {
		case IPC_M_CONNECT_ME_TO:
			IPC_SET_RETVAL(call, 0);
			break;
		}
		if (! (callid & IPC_CALLID_NOTIFICATION)) {
			ipc_answer(callid, &call);
		}
		printf("%s: received call from %lX\n", NAME, call.in_phone_hash);
	}

	pci_cleanup(pacc);
	return 0;
}

/**
 * @}
 */
