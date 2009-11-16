#ifndef PCI_H
#define PCI_H

#include <adt/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pci_drv;
struct pci_dev;
struct pci_bus;
struct pci_drv_ops;

typedef struct pci_drv pci_drv_t;
typedef struct pci_dev pci_dev_t;
typedef struct pci_bus pci_bus_t;
typedef struct pci_drv_ops pci_drv_ops_t;

struct pci_drv {
	const char *name;
	link_t link;
	int vendor_id;
	int device_id;	
	pci_drv_ops_t *ops;	
};

struct pci_dev{
	link_t link;
	pci_bus_t *bus;
	int dev;
	int fn;
	
	int vendor_id;
	int device_id;
	pci_drv_t *driver;	
};

struct pci_drv_ops {
	int (*add_device)(pci_dev_t *dev);
	
};

struct pci_bus {
	link_t link;
	int num;
	// architecture-specific usage
	void *data;	
};

void pci_bus_register(pci_bus_t *bus);
void pci_device_register(pci_dev_t *dev);
void pci_driver_register(pci_drv_t *drv);

void pci_bus_scan(pci_bus_t *bus);

static inline pci_bus_t* pci_alloc_bus()
{
	pci_bus_t *bus = (pci_bus_t *)malloc(sizeof(pci_bus_t));
	link_initialize(&bus->link);
	bus->num = 0;
	bus->data = NULL;
	return bus;
}

// arch. spec. initialization of pci bus structure - of its data member
void pci_init_bus_data(pci_bus_t *bus, pci_bus_t *parent);

static inline void pci_init_bus(pci_bus_t *bus, pci_bus_t *parent, int bus_num)
{
	bus->num = bus_num; 
	pci_init_bus_data(bus, parent);
}

static inline void pci_free_bus(pci_bus_t *bus)
{
	free(bus);
}

static inline pci_dev_t* pci_alloc_dev()
{
	pci_dev_t *dev = (pci_dev_t *)malloc(sizeof(pci_dev_t));
	bzero(dev, sizeof(pci_dev_t));
	link_initialize(&dev->link);	
	return dev;
}

static inline void pci_init_dev(pci_dev_t *dev, pci_bus_t *bus, int devnum, int fn)
{
	dev->bus = bus;
	dev->dev = devnum;
	dev->fn = fn;
}

static inline void pci_free_dev(pci_dev_t *dev)
{
	free(dev);
}

#endif
