#include <futex.h>
#include <assert.h>

#include "pci.h"
#include "pci_bus.h"
#include "pci_regs.h"
#include "pci_conf.h"

#define NAME "PCI"

LIST_INITIALIZE(devices_list);
LIST_INITIALIZE(buses_list);
LIST_INITIALIZE(drivers_list);

static atomic_t pci_bus_futex = FUTEX_INITIALIZER;

static int pci_match(pci_drv_t *drv, pci_dev_t *dev);
static int pci_pass_dev(pci_drv_t *drv, pci_dev_t *dev);
static void pci_lookup_driver(pci_dev_t *dev);
static void pci_lookup_devices(pci_drv_t *drv);


void pci_bus_scan(pci_bus_t *bus)
{
	pci_dev_t *dev = pci_alloc_dev();
	pci_bus_t *child_bus = NULL;
	int dnum, fnum, bus_num;
	bool multi;
	uint8_t header_type;
	
	for (dnum = 0; dnum < 32; dnum++) {
		multi = true;
		for (fnum = 0; multi && fnum < 8; fnum++) {
			pci_init_dev(dev, bus, dnum, fnum);
			dev->vendor_id = pci_conf_read_16(dev, PCI_VENDOR_ID);
			dev->device_id = pci_conf_read_16(dev, PCI_DEVICE_ID);
			if (dev->vendor_id == 0xFFFF) { // device is not present, go on scanning the bus
				if (fnum == 0) {
					break;
				} else {
					continue;  
				}
			}
			header_type = pci_conf_read_8(dev, PCI_HEADER_TYPE);
			if (fnum == 0) {
				 multi = header_type >> 7;  // is the device multifunction?
			}
			header_type = header_type & 0x7F; // clear the multifunction bit
			
			printf(NAME ": adding new device %3d : %2d : %2d", dev->bus->num, dnum, fnum);
			printf(" - vendor = 0x%04X, device = 0x%04X.\n", dev->vendor_id, dev->device_id);
			pci_device_register(dev);			
			
			if (header_type == PCI_HEADER_TYPE_BRIDGE || header_type == PCI_HEADER_TYPE_CARDBUS ) {
				bus_num = pci_conf_read_8(dev, PCI_BRIDGE_SEC_BUS_NUM);
				printf(NAME ": device is pci-to-pci bridge, secondary bus number = %d.\n", bus_num);
				if(bus_num > bus->num) {					
					child_bus = pci_alloc_bus();
					pci_init_bus(child_bus, bus, bus_num);
					pci_bus_register(child_bus);
					pci_bus_scan(child_bus);	
				}					
			}
			
			dev = pci_alloc_dev();  // alloc new aux. dev. structure
		}
	}
	
	if (dev->vendor_id == 0xFFFF) {
		pci_free_dev(dev);  // free the auxiliary device structure
	}	
}

/*
 * Usage: pci_bus_futex must be down.
 */ 
static int pci_pass_dev(pci_drv_t *drv, pci_dev_t *dev)
{
	assert(dev->driver == NULL);
	assert(drv->name != NULL);
	
	printf(NAME ": passing device to driver '%s'.\n", drv->name);
	if (drv->ops->add_device != NULL && drv->ops->add_device(dev)) {
		dev->driver = drv;
		return 1;
	}
	
	return 0;
}

/*
 * Usage: pci_bus_futex must be down.
 */ 
static void pci_lookup_driver(pci_dev_t *dev)
{
	link_t *item = drivers_list.next;
	pci_drv_t *drv = NULL; 
	
	while (item != &drivers_list) {
		drv = list_get_instance(item, pci_drv_t, link);
		if (pci_match(drv, dev) && pci_pass_dev(drv, dev)) {
			return;
		}
		item = item->next;
	}
}

/*
 * Usage: pci_bus_futex must be down.
 */ 
static void pci_lookup_devices(pci_drv_t *drv)
{
	link_t *item = devices_list.next;
	pci_dev_t *dev = NULL; 
	
	printf(NAME ": looking up devices for a newly added driver '%s'.\n", drv->name);
	
	while (item != &devices_list) {
		dev = list_get_instance(item, pci_dev_t, link);
		if (dev->driver == NULL && pci_match(drv, dev)) {
			pci_pass_dev(drv, dev);
		}
		item = item->next;
	}
}

static int pci_match(pci_drv_t *drv, pci_dev_t *dev)
{
	return drv->vendor_id == dev->vendor_id && drv->device_id == dev->device_id;
}

void pci_bus_register(pci_bus_t *bus)
{
	futex_down(&pci_bus_futex);
	
	// add the device to the list of pci devices
	list_append(&(bus->link), &buses_list);
	
	futex_up(&pci_bus_futex);
}

void pci_device_register(pci_dev_t *dev)
{
	futex_down(&pci_bus_futex);
	
	// add the device to the list of pci devices
	list_append(&(dev->link), &devices_list);
	
	// try to find suitable driver for the device and pass the device to it
	pci_lookup_driver(dev);
	
	futex_up(&pci_bus_futex);	
}

void pci_driver_register(pci_drv_t *drv)
{	
	assert(drv->name != NULL);
	
	futex_down(&pci_bus_futex);
	printf(NAME ": registering new driver '%s'.\n", drv->name);
	list_append(&(drv->link), &drivers_list);
	
	// try to find compatible devices
	pci_lookup_devices(drv);
	
	futex_up(&pci_bus_futex);
}

