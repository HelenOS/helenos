/*
 * SPDX-FileCopyrightText: 2012 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments AM335x timer driver.
 */

#include <assert.h>
#include <genarch/drivers/am335x/timer.h>
#include <mm/km.h>
#include <errno.h>

typedef enum {
	REG_TCLR = 0x00,
	REG_TCRR = 0x01,
	REG_TLDR = 0x02,
	REG_TTGR = 0x04
} timer_reg_t;

typedef struct timer_regs_mmap {
	uintptr_t base;
	size_t size;
} timer_regs_mmap_t;

static const timer_regs_mmap_t regs_map[TIMERS_MAX] = {
	{ .base = AM335x_DMTIMER0_BASE_ADDRESS, .size = AM335x_DMTIMER0_SIZE },
	{ 0, 0 }, /* DMTIMER1 is not supported by this driver */
	{ .base = AM335x_DMTIMER2_BASE_ADDRESS, .size = AM335x_DMTIMER2_SIZE },
	{ .base = AM335x_DMTIMER3_BASE_ADDRESS, .size = AM335x_DMTIMER3_SIZE },
	{ .base = AM335x_DMTIMER4_BASE_ADDRESS, .size = AM335x_DMTIMER4_SIZE },
	{ .base = AM335x_DMTIMER5_BASE_ADDRESS, .size = AM335x_DMTIMER5_SIZE },
	{ .base = AM335x_DMTIMER6_BASE_ADDRESS, .size = AM335x_DMTIMER6_SIZE },
	{ .base = AM335x_DMTIMER7_BASE_ADDRESS, .size = AM335x_DMTIMER7_SIZE },
};

static void
write_register_posted(am335x_timer_t *timer, timer_reg_t reg, uint32_t value)
{
	am335x_timer_regs_t *regs = timer->regs;

	while (regs->twps & reg)
		;

	switch (reg) {
	default:
		return;
	case REG_TCLR:
		regs->tclr = value;
		break;
	case REG_TCRR:
		regs->tcrr = value;
		break;
	case REG_TLDR:
		regs->tldr = value;
		break;
	}
}

errno_t
am335x_timer_init(am335x_timer_t *timer, am335x_timer_id_t id, unsigned hz,
    unsigned srcclk_hz)
{
	uintptr_t base_addr;
	size_t size;

	assert(id < TIMERS_MAX);
	assert(timer != NULL);

	if (id == DMTIMER1_1MS)
		return ENOTSUP; /* Not supported yet */

	base_addr = regs_map[id].base;
	size = regs_map[id].size;

	timer->regs = (void *) km_map(base_addr, size, KM_NATURAL_ALIGNMENT,
	    PAGE_NOT_CACHEABLE);
	assert(timer->regs != NULL);

	timer->id = id;

	am335x_timer_regs_t *regs = timer->regs;

	/* Enable the posted mode of operation */
	regs->tsicr |= AM335x_TIMER_TSICR_POSTED_FLAG;

	/* Stop the timer */
	am335x_timer_stop(timer);

	/* Perform a soft reset */
	am335x_timer_reset(timer);

	unsigned tclr = regs->tclr;

	/* Disable compare mode */
	tclr &= ~AM335x_TIMER_TCLR_CE_FLAG;

	/* Enable auto-reload mode */
	tclr |= AM335x_TIMER_TCLR_AR_FLAG;

	write_register_posted(timer, REG_TCLR, tclr);

	/* Disable the emulation mode */
	regs->tiocp_cfg |= AM335x_TIMER_TIOCPCFG_EMUFREE_FLAG;

	unsigned const count = 0xFFFFFFFF - (srcclk_hz / hz + 1);
	write_register_posted(timer, REG_TCRR, count);
	write_register_posted(timer, REG_TLDR, count);

	return EOK;
}

void
am335x_timer_intr_ack(am335x_timer_t *timer)
{
	/* Clear pending OVF event */
	timer->regs->irqstatus |= AM335x_TIMER_IRQSTATUS_OVF_FLAG;
}

void
am335x_timer_reset(am335x_timer_t *timer)
{
	/* Initiate soft reset */
	timer->regs->tiocp_cfg |= AM335x_TIMER_TIOCPCFG_SOFTRESET_FLAG;
	/* Wait until the reset is done */
	while (timer->regs->tiocp_cfg & AM335x_TIMER_TIOCPCFG_SOFTRESET_FLAG)
		;
}

void
am335x_timer_stop(am335x_timer_t *timer)
{
	/* Disable the interrupt */
	timer->regs->irqenable_clr |= AM335x_TIMER_IRQENABLE_CLR_OVF_FLAG;
	timer->regs->irqwakeen &= ~AM335x_TIMER_IRQWAKEEN_OVF_FLAG;
	/* Stop the timer */
	write_register_posted(timer, REG_TCLR,
	    timer->regs->tclr & ~AM335x_TIMER_TCLR_ST_FLAG);
}

void
am335x_timer_start(am335x_timer_t *timer)
{
	/* Enable the interrupt */
	timer->regs->irqenable_set |= AM335x_TIMER_IRQENABLE_SET_OVF_FLAG;
	timer->regs->irqwakeen |= AM335x_TIMER_IRQWAKEEN_OVF_FLAG;
	/* Start the clock */
	write_register_posted(timer, REG_TCLR,
	    timer->regs->tclr | AM335x_TIMER_TCLR_ST_FLAG);
}

/**
 * @}
 */
