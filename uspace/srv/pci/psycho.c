#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <stdio.h>
#include <futex.h>

#include "pci.h"
#include "pci_bus.h"

#define PCI_CONF_OFFSET   0x001000000
#define PCI_CONF_SIZE     0x001000000

/* 
 * virtual address of specified PCI configuration register:
 * conf_base ... base address of configuration address space 
 * bus ... bus number 
 * dev ... device number (0 - 31)
 * fn  ... function number (0 - 7)
 * reg ... register number (register's position within PCI configuration header) 
 **/
#define CONF_ADDR(conf_base, bus, dev, fn, reg)   ((void *)(conf_base + ((bus << 16) | (dev << 11) | (fn << 8) | reg)))

static atomic_t pci_conf_futex = FUTEX_INITIALIZER;

static long u2p_space_cnt = 0;
static long *u2p_bases;
/* virtual addresses of PCI configuration spaces */
static void **conf_bases;  

static int psycho_init();
static void u2p_bases_init();
static void psycho_scan();
static void * psycho_conf_addr(pci_dev_t *dev, int reg);
static void psycho_conf_read(pci_dev_t *d, int reg, uint8_t *buf, int len);
static void psycho_conf_write(pci_dev_t *d, int reg, uint8_t *buf, int len);


static void * psycho_conf_addr(pci_dev_t *dev, int reg)
{	
	return CONF_ADDR(dev->bus->data, dev->bus->num, dev->dev, dev->fn, reg);
}

static void psycho_scan()
{	
	printf("PCI: psycho_scan\n");
	int i, num;
	for (i = 0; i < u2p_space_cnt; i++) {
		for (num = 0; num <= 0x80; num += 0x80) {		
			pci_bus_t *bus = pci_alloc_bus();
			bus->num = num;
			bus->data = conf_bases[i];
			pci_bus_register(bus);
			pci_bus_scan(bus);
		}
	}
}

static int psycho_init()
{
	printf("PCI: starting psycho initialization.\n");
	u2p_bases_init();
	conf_bases = (void **)malloc(u2p_space_cnt * sizeof(void *));	
	int i, error;
	for (i = 0; i < u2p_space_cnt; i++) {
		if (error = pio_enable((void *)(u2p_bases[i] + PCI_CONF_OFFSET), PCI_CONF_SIZE, &(conf_bases[i]))) {
			printf("PCI: failed to enable psycho conf. adr. space num. %d. (error %d)\n", i, error);
			return 0;
		}
	}	
	return 1;
}

static void u2p_bases_init()
{
	// TODO: write this more generally - get this information from firmware (using kernel + sysinfo)	
	u2p_space_cnt = 2;
	//u2p_space_cnt = 1;
	u2p_bases = (void **)malloc(u2p_space_cnt * sizeof(void *));
	u2p_bases[0] = 0x1c800000000;
	u2p_bases[1] = 0x1ca00000000;	
}

uint8_t pci_conf_read_8(pci_dev_t *dev, int reg)
{
	uint8_t res;	
	psycho_conf_read(dev, reg, &res, sizeof(uint8_t));	
	return res;
}

uint16_t pci_conf_read_16(pci_dev_t *dev, int reg)
{
	uint16_t res;
	psycho_conf_read(dev, reg, (uint8_t *)(&res), sizeof(uint16_t));	
	return res;
}

uint32_t pci_conf_read_32(pci_dev_t *dev, int reg)
{
	uint32_t res;
	psycho_conf_read(dev, reg, (uint8_t *)(&res), sizeof(uint32_t));		
	return res;	
}

static inline uint16_t invert_endianness_16(uint16_t x) 
{
	return (x & 0xFF) << 8 | (x >> 8);
}

static inline uint32_t invert_endianness_32(uint32_t x) 
{
	return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | (x >> 24);
}

static void psycho_conf_read(pci_dev_t *d, int reg, uint8_t *buf, int len)
{	
	futex_down(&pci_conf_futex);
	switch (len) {
		case 1:
			buf[0] = pio_read_8(psycho_conf_addr(d, reg));
			break;
		case 2:
			*((uint16_t *)buf) = invert_endianness_16(pio_read_16(psycho_conf_addr(d, reg)));
			break;
		case 4:
			*((uint32_t *)buf) = invert_endianness_32(pio_read_32(psycho_conf_addr(d, reg)));
			break;
	}
	futex_up(&pci_conf_futex);
}

static void psycho_conf_write(pci_dev_t *d, int reg, uint8_t *buf, int len)
{	
	futex_down(&pci_conf_futex);
	switch (len) {
		case 1:
			pio_write_8(psycho_conf_addr(d, reg), buf[0]);
			break;
		case 2:
			pio_write_16(psycho_conf_addr(d, reg), invert_endianness_16(*((uint16_t *)buf)));
			break;
		case 4:
			pio_write_32(psycho_conf_addr(d, reg), invert_endianness_32(*((uint32_t *)buf)));
			break;
	}
	futex_up(&pci_conf_futex);
}

/* pci bus structure initialization */
void pci_init_bus_data(pci_bus_t *bus, pci_bus_t *parent)
{
	if (parent != NULL) {
		bus->data = parent->data;
	}
}

int pci_bus_init() 
{	
	if(!psycho_init()) {
		return 0;
	}	
	psycho_scan();	
	return 1;
}

void pci_bus_clean()
{
	free(u2p_bases);
	free(conf_bases);
}