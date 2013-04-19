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

/** @addtogroup apic
 * @{
 */

/**
 * @file apic.c
 * @brief APIC driver.
 */

#include <ipc/services.h>
#include <ipc/irc.h>
#include <ns.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <stdbool.h>
#include <errno.h>
#include <async.h>
#include <stdio.h>

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
	} __attribute__ ((packed));
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
		} __attribute__ ((packed));
	};
	union {
		uint32_t hi;
		struct {
			unsigned int : 24;	/**< Reserved. */
			uint8_t dest : 8;  	/**< Destination Field. */
		} __attribute__ ((packed));
	};
} __attribute__ ((packed)) io_redirection_reg_t;

// FIXME: get the address from the kernel
#define IO_APIC_BASE	0xfec00000UL
#define IO_APIC_SIZE	20

ioport32_t *io_apic = NULL;

/** Read from IO APIC register.
 *
 * @param address IO APIC register address.
 *
 * @return Content of the addressed IO APIC register.
 *
 */
static uint32_t io_apic_read(uint8_t address)
{
	io_regsel_t regsel;

	regsel.value = io_apic[IOREGSEL];
	regsel.reg_addr = address;
	io_apic[IOREGSEL] = regsel.value;
	return io_apic[IOWIN];
}

/** Write to IO APIC register.
 *
 * @param address IO APIC register address.
 * @param val     Content to be written to the addressed IO APIC register.
 *
 */
static void io_apic_write(uint8_t address, uint32_t val)
{
	io_regsel_t regsel;

	regsel.value = io_apic[IOREGSEL];
	regsel.reg_addr = address;
	io_apic[IOREGSEL] = regsel.value;
	io_apic[IOWIN] = val;
}

static int irq_to_pin(int irq)
{
	// FIXME: get the map from the kernel, even though this may work
	//	  for simple cases
	if (irq == 0)
		return 2;
	return irq;
}

static int apic_enable_irq(sysarg_t irq)
{
	io_redirection_reg_t reg;

	if (irq > APIC_MAX_IRQ)
		return ELIMIT;

	int pin = irq_to_pin(irq);
 	if (pin == -1)
		return ENOENT;

	reg.lo = io_apic_read((uint8_t) (IOREDTBL + pin * 2));
	reg.masked = false;
	io_apic_write((uint8_t) (IOREDTBL + pin * 2), reg.lo);

	return EOK;
}

/** Handle one connection to APIC.
 *
 * @param iid   Hash of the request that opened the connection.
 * @param icall Call data of the request that opened the connection.
 * @param arg	Local argument.
 */
static void apic_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);
	
	while (true) {
		callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(callid, apic_enable_irq(IPC_GET_ARG1(call)));
			break;
		case IRC_CLEAR_INTERRUPT:
			/* Noop */
			async_answer_0(callid, EOK);
			break;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

/** Initialize the APIC driver.
 *
 */
static bool apic_init(void)
{
	sysarg_t apic;
	
	if ((sysinfo_get_value("apic", &apic) != EOK) || (!apic)) {
		printf("%s: No APIC found\n", NAME);
		return false;
	}

	int rc = pio_enable((void *) IO_APIC_BASE, IO_APIC_SIZE,
		(void **) &io_apic);
	if (rc != EOK) {
		printf("%s: Failed to enable PIO for APIC: %d\n", NAME, rc);
		return false;	
	}
	
	async_set_client_connection(apic_connection);
	service_register(SERVICE_IRC);
	
	return true;
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS APIC driver\n", NAME);
	
	if (!apic_init())
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Never reached */
	return 0;
}

/**
 * @}
 */
