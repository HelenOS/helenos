/*
 * Copyright (c) 2011 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup uspace_drv_apic
 * @{
 */

/**
 * @file apic.c
 * @brief APIC driver.
 */

#include <ipc/irc.h>
#include <loc.h>
#include <sysinfo.h>
#include <as.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <async.h>
#include <stdio.h>

#include "apic.h"

#define NAME  "apic"

#define APIC_MAX_IRQ	15

#define IOREGSEL  (0x00U / sizeof(uint32_t))
#define IOWIN     (0x10U / sizeof(uint32_t))

#define IOREDTBL   0x10U

/** I/O Register Select Register. */
typedef union {
	uint32_t value;
	struct {
		uint8_t reg_addr;	/**< APIC Register Address. */
		unsigned int : 24;	/**< Reserved. */
	} __attribute__((packed));
} io_regsel_t;

/** I/O Redirection Register. */
typedef struct io_redirection_reg {
	union {
		uint32_t lo;
		struct {
			uint8_t intvec;			/**< Interrupt Vector. */
			unsigned int delmod : 3;	/**< Delivery Mode. */
			unsigned int destmod : 1;	/**< Destination mode. */
			unsigned int delivs : 1;	/**< Delivery status (RO). */
			unsigned int intpol : 1;	/**< Interrupt Input Pin Polarity. */
			unsigned int irr : 1;		/**< Remote IRR (RO). */
			unsigned int trigger_mode : 1;	/**< Trigger Mode. */
			unsigned int masked : 1;	/**< Interrupt Mask. */
			unsigned int : 15;		/**< Reserved. */
		} __attribute__((packed));
	};
	union {
		uint32_t hi;
		struct {
			unsigned int : 24;	/**< Reserved. */
			uint8_t dest : 8;  	/**< Destination Field. */
		} __attribute__((packed));
	};
} __attribute__((packed)) io_redirection_reg_t;

#define IO_APIC_SIZE	20

/** Read from IO APIC register.
 *
 * @param apic APIC
 * @param address IO APIC register address.
 *
 * @return Content of the addressed IO APIC register.
 *
 */
static uint32_t io_apic_read(apic_t *apic, uint8_t address)
{
	io_regsel_t regsel;

	regsel.value = pio_read_32(&apic->regs[IOREGSEL]);
	regsel.reg_addr = address;
	pio_write_32(&apic->regs[IOREGSEL], regsel.value);
	return pio_read_32(&apic->regs[IOWIN]);
}

/** Write to IO APIC register.
 *
 * @param apic    APIC
 * @param address IO APIC register address.
 * @param val     Content to be written to the addressed IO APIC register.
 *
 */
static void io_apic_write(apic_t *apic, uint8_t address, uint32_t val)
{
	io_regsel_t regsel;

	regsel.value = pio_read_32(&apic->regs[IOREGSEL]);
	regsel.reg_addr = address;
	pio_write_32(&apic->regs[IOREGSEL], regsel.value);
	pio_write_32(&apic->regs[IOWIN], val);
}

static int irq_to_pin(int irq)
{
	// FIXME: get the map from the kernel, even though this may work
	//	  for simple cases
	if (irq == 0)
		return 2;
	return irq;
}

static errno_t apic_enable_irq(apic_t *apic, sysarg_t irq)
{
	io_redirection_reg_t reg;

	if (irq > APIC_MAX_IRQ)
		return ELIMIT;

	int pin = irq_to_pin(irq);
	if (pin == -1)
		return ENOENT;

	reg.lo = io_apic_read(apic, (uint8_t) (IOREDTBL + pin * 2));
	reg.masked = false;
	io_apic_write(apic, (uint8_t) (IOREDTBL + pin * 2), reg.lo);

	return EOK;
}

/** Handle one connection to APIC.
 *
 * @param icall Call data of the request that opened the connection.
 * @param arg   Local argument.
 *
 */
static void apic_connection(ipc_call_t *icall, void *arg)
{
	ipc_call_t call;
	apic_t *apic;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_accept_0(icall);

	apic = (apic_t *) ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *) arg));

	while (true) {
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* The other side has hung up. */
			async_answer_0(&call, EOK);
			return;
		}

		switch (ipc_get_imethod(&call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(&call, apic_enable_irq(apic,
			    ipc_get_arg1(&call)));
			break;
		case IRC_DISABLE_INTERRUPT:
			/* XXX TODO */
			async_answer_0(&call, EOK);
			break;
		case IRC_CLEAR_INTERRUPT:
			/* Noop */
			async_answer_0(&call, EOK);
			break;
		default:
			async_answer_0(&call, EINVAL);
			break;
		}
	}
}

/** Add APIC device. */
errno_t apic_add(apic_t *apic, apic_res_t *res)
{
	sysarg_t have_apic;
	ddf_fun_t *fun_a = NULL;
	void *regs;
	errno_t rc;
	bool bound = false;

	if ((sysinfo_get_value("apic", &have_apic) != EOK) || (!have_apic)) {
		printf("%s: No APIC found\n", NAME);
		return ENOTSUP;
	}

	rc = pio_enable((void *) res->base, IO_APIC_SIZE, &regs);
	if (rc != EOK) {
		printf("%s: Failed to enable PIO for APIC: %s\n", NAME, str_error(rc));
		return EIO;
	}

	apic->regs = (ioport32_t *)regs;

	fun_a = ddf_fun_create(apic->dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun_a, apic_connection);

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a': %s", str_error(rc));
		goto error;
	}

	bound = true;

	rc = ddf_fun_add_to_category(fun_a, "irc");
	if (rc != EOK)
		goto error;

	return EOK;
error:
	if (bound)
		ddf_fun_unbind(fun_a);
	if (fun_a != NULL)
		ddf_fun_destroy(fun_a);
	return rc;
}

/** Remove APIC device */
errno_t apic_remove(apic_t *apic)
{
	return ENOTSUP;
}

/** APIC device gone */
errno_t apic_gone(apic_t *apic)
{
	return ENOTSUP;
}

/**
 * @}
 */
