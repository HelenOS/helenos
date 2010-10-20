#include <unistd.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <stdio.h>

#include "pic.h"

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define REG_COUNT	2			/* command and data */

#define NAME "PIC"

static ioport8_t *pic1_cmd, *pic1_data, *pic2_cmd, *pic2_data;

static int pic_enable_ports(void *base_phys_addr, ioport8_t **pic_cmd, ioport8_t **pic_data);


int pic_init()
{	
	printf(NAME ": enabling ports 0x%x - 0x%x.\n", PIC1, PIC1 + 1);
	if (!pic_enable_ports((void *)PIC1, &pic1_cmd, &pic1_data)) {
		printf(NAME ": Error - master PIC initialization failed.\n");
		return 0;
	}
	
	printf(NAME ": enabling ports 0x%x - 0x%x.\n", PIC2, PIC2 + 1);
	if (!pic_enable_ports((void *)PIC2, &pic2_cmd, &pic2_data)) {
		printf(NAME ": Error - slave PIC initialization failed.");
		return 0;
	}
	
	printf(NAME ": initialization was successful.\n");
	return 1;
}

static int pic_enable_ports(void *base_phys_addr, ioport8_t **pic_cmd, ioport8_t **pic_data)
{
	if (pio_enable(base_phys_addr, REG_COUNT, (void **)(pic_cmd))) {  // Gain control over port's registers.
		printf(NAME ": Error - cannot gain the port %lx.\n", base_phys_addr);
		return 0;
	}
	
	*pic_data = *pic_cmd + 1;
	return 1;
}

void pic_enable_interrupt(int irq)
{
	printf(NAME ": pic_enable_interrupt %d.", irq);
	pic_enable_irqs(1 << irq);
	printf(NAME ": interrupt %d enabled.", irq);
}

void pic_enable_irqs(uint16_t irqmask)
{
	uint8_t x;

	if (irqmask & 0xff) {
		x = pio_read_8(pic1_data);
		pio_write_8(pic1_data, (uint8_t) (x & (~(irqmask & 0xff))));
	}
	if (irqmask >> 8) {
		x = pio_read_8(pic2_data);
		pio_write_8(pic2_data, (uint8_t) (x & (~(irqmask >> 8))));
	}
}
