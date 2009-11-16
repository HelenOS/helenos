#include "pci.h"
#include "intel_piix3.h"
#include "isa.h"

#define NAME "Intel PIIX3"


static int piix3_add_device(pci_dev_t *dev);
static void * piix3_absolutize(void *phys_addr);

static pci_drv_ops_t piix3_pci_ops = {
	.add_device = piix3_add_device
};

static pci_drv_t piix3_drv = {
	.name = NAME,
	.link = { NULL, NULL },
	.vendor_id = 0x8086,
	.device_id = 0x7010, 
	.ops = &piix3_pci_ops
};

static bridge_to_isa_ops_t piix3_bridge_ops = {
	.absolutize = piix3_absolutize
};

int intel_piix3_init()
{
	pci_driver_register(&piix3_drv);
	return 0;
}


static int piix3_add_device(pci_dev_t *dev)
{
	printf(NAME " driver: new device %3d : %2d : %2d was added.\n", dev->bus->num, dev->dev, dev->fn);
	
	// register this device as a pci-to-isa bridge by the isa bus driver
	bridge_to_isa_t *bridge_dev = isa_alloc_bridge();
	isa_init_bridge(bridge_dev, &piix3_bridge_ops, dev);
	isa_register_bridge(bridge_dev);		
	
	return 1;
}

// this might be more usable at different architectures
static void * piix3_absolutize(void *phys_addr)
{
	return phys_addr;
}




