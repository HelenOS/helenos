#include <ddi.h>
#include <libarch/ddi.h>
#include <futex.h>

#include "pci.h"
#include "pci_bus.h"
#include "pci_conf.h"

#define CONF_ADDR_PORT 0xCF8
#define CONF_DATA_PORT 0xCFC
#define CONF_PORT_SIZE 4

static void *conf_addr_port = NULL;
static void *conf_data_port = NULL;

static atomic_t pci_conf_futex = FUTEX_INITIALIZER;

static void method1_conf_read(pci_dev_t *d, int reg, uint8_t *buf, int len);

#define CONF_ADDR(bus, dev, fn, reg)   ((1 << 31) | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))

uint8_t pci_conf_read_8(pci_dev_t *dev, int reg)
{
	uint8_t res;
	method1_conf_read(dev, reg, &res, 1);
	return res;
}

uint16_t pci_conf_read_16(pci_dev_t *dev, int reg)
{
	uint16_t res;
	method1_conf_read(dev, reg, (uint8_t *)&res, 2);
	return res;
}

uint32_t pci_conf_read_32(pci_dev_t *dev, int reg)
{
	uint32_t res;
	method1_conf_read(dev, reg, (uint8_t *)&res, 4);
	return res;	
}

static void method1_conf_read(pci_dev_t *dev, int reg, uint8_t *buf, int len)
{
	futex_down(&pci_conf_futex);
	uint32_t conf_addr =  CONF_ADDR(dev->bus->num, dev->dev, dev->fn, reg);
	void *addr = conf_data_port + (reg & 3);
	
	pio_write_32(conf_addr_port, conf_addr);
	
	switch (len) {
		case 1:
			buf[0] = pio_read_8(addr);
			break;
		case 2:
			((uint16_t *)buf)[0] = pio_read_16(addr);
			break;
		case 4:
			((uint32_t *)buf)[0] = pio_read_32(addr);
			break;
	}
	
	futex_up(&pci_conf_futex);
}

int pci_bus_init() 
{	
	int error;
	if (error = pio_enable((void *)CONF_ADDR_PORT, CONF_PORT_SIZE, &conf_addr_port)) {
		printf("PCI: failed to enable configuration address port (error %d)\n", error);
		return 0;
	}
	
	if (error = pio_enable((void *)CONF_DATA_PORT, CONF_PORT_SIZE, &conf_data_port)) {
		printf("PCI: failed to enable configuration data port (error %d)\n", error);
		return 0;
	}
	
	pci_bus_t *bus = pci_alloc_bus();
	bus->data = NULL;
	bus->num = 0;
	pci_bus_register(bus);
	pci_bus_scan(bus);
	return 1;
}

void pci_bus_clean()
{
}

/* pci bus structure initialization */
void pci_init_bus_data(pci_bus_t *bus, pci_bus_t *parent)
{
	bus->data = NULL;
}
