#ifndef PIC_H
#define PIC_H

#include <stdlib.h>

int pic_init();
void pic_enable_interrupt(int irq);
void pic_enable_irqs(uint16_t irqmask);

#endif