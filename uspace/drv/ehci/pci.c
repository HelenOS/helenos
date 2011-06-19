/*
 * Copyright (c) 2011 Jan Vesely
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

/**
 * @addtogroup drvusbehci
 * @{
 */
/**
 * @file
 * PCI related functions needed by the EHCI driver.
 */

#include <errno.h>
#include <str_error.h>
#include <assert.h>
#include <as.h>
#include <devman.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <device/hw_res.h>

#include <usb/debug.h>
#include <pci_dev_iface.h>

#include "pci.h"

#define PAGE_SIZE_MASK 0xfffff000

#define HCC_PARAMS_OFFSET 0x8
#define HCC_PARAMS_EECP_MASK 0xff
#define HCC_PARAMS_EECP_OFFSET 8

#define CMD_OFFSET 0x0
#define STS_OFFSET 0x4
#define INT_OFFSET 0x8
#define CFG_OFFSET 0x40

#define USBCMD_RUN 1
#define USBSTS_HALTED (1 << 12)

#define USBLEGSUP_OFFSET 0
#define USBLEGSUP_BIOS_CONTROL (1 << 16)
#define USBLEGSUP_OS_CONTROL (1 << 24)
#define USBLEGCTLSTS_OFFSET 4

#define DEFAULT_WAIT 1000
#define WAIT_STEP 10

#define PCI_READ(size) \
do { \
	async_sess_t *parent_sess = \
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, dev->handle, \
	    IPC_FLAG_BLOCKING); \
	if (!parent_sess) \
		return ENOMEM; \
	\
	sysarg_t add = (sysarg_t) address; \
	sysarg_t val; \
	\
	async_exch_t *exch = async_exchange_begin(parent_sess); \
	\
	const int ret = \
	    async_req_2_1(exch, DEV_IFACE_ID(PCI_DEV_IFACE), \
	        IPC_M_CONFIG_SPACE_READ_##size, add, &val); \
	\
	async_exchange_end(exch); \
	async_hangup(parent_sess); \
	\
	assert(value); \
	\
	*value = val; \
	return ret; \
} while (0)

static int pci_read32(const ddf_dev_t *dev, int address, uint32_t *value)
{
	PCI_READ(32);
}

static int pci_read16(const ddf_dev_t *dev, int address, uint16_t *value)
{
	PCI_READ(16);
}

static int pci_read8(const ddf_dev_t *dev, int address, uint8_t *value)
{
	PCI_READ(8);
}

#define PCI_WRITE(size) \
do { \
	async_sess_t *parent_sess = \
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, dev->handle, \
	    IPC_FLAG_BLOCKING); \
	if (!parent_sess) \
		return ENOMEM; \
	\
	sysarg_t add = (sysarg_t) address; \
	sysarg_t val = value; \
	\
	async_exch_t *exch = async_exchange_begin(parent_sess); \
	\
	const int ret = \
	    async_req_3_0(exch, DEV_IFACE_ID(PCI_DEV_IFACE), \
	        IPC_M_CONFIG_SPACE_WRITE_##size, add, val); \
	\
	async_exchange_end(exch); \
	async_hangup(parent_sess); \
	\
	return ret; \
} while(0)

static int pci_write32(const ddf_dev_t *dev, int address, uint32_t value)
{
	PCI_WRITE(32);
}

static int pci_write16(const ddf_dev_t *dev, int address, uint16_t value)
{
	PCI_WRITE(16);
}

static int pci_write8(const ddf_dev_t *dev, int address, uint8_t value)
{
	PCI_WRITE(8);
}

/** Get address of registers and IRQ for given device.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] mem_reg_address Base address of the memory range.
 * @param[out] mem_reg_size Size of the memory range.
 * @param[out] irq_no IRQ assigned to the device.
 * @return Error code.
 */
int pci_get_my_registers(const ddf_dev_t *dev,
    uintptr_t *mem_reg_address, size_t *mem_reg_size, int *irq_no)
{
	assert(dev != NULL);
	
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, dev->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;
	
	hw_resource_list_t hw_resources;
	int rc = hw_res_get_resource_list(parent_sess, &hw_resources);
	if (rc != EOK) {
		async_hangup(parent_sess);
		return rc;
	}
	
	uintptr_t mem_address = 0;
	size_t mem_size = 0;
	bool mem_found = false;
	
	int irq = 0;
	bool irq_found = false;
	
	size_t i;
	for (i = 0; i < hw_resources.count; i++) {
		hw_resource_t *res = &hw_resources.resources[i];
		switch (res->type) {
		case INTERRUPT:
			irq = res->res.interrupt.irq;
			irq_found = true;
			usb_log_debug2("Found interrupt: %d.\n", irq);
			break;
		case MEM_RANGE:
			if (res->res.mem_range.address != 0
			    && res->res.mem_range.size != 0 ) {
				mem_address = res->res.mem_range.address;
				mem_size = res->res.mem_range.size;
				usb_log_debug2("Found mem: %" PRIxn" %zu.\n",
				    mem_address, mem_size);
				mem_found = true;
			}
		default:
			break;
		}
	}
	
	if (mem_found && irq_found) {
		*mem_reg_address = mem_address;
		*mem_reg_size = mem_size;
		*irq_no = irq;
		rc = EOK;
	} else {
		rc = ENOENT;
	}
	
	async_hangup(parent_sess);
	return rc;
}
/*----------------------------------------------------------------------------*/
/** Calls the PCI driver with a request to enable interrupts
 *
 * @param[in] device Device asking for interrupts
 * @return Error code.
 */
int pci_enable_interrupts(const ddf_dev_t *device)
{
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, device->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;
	
	const bool enabled = hw_res_enable_interrupt(parent_sess);
	async_hangup(parent_sess);
	
	return enabled ? EOK : EIO;
}
/*----------------------------------------------------------------------------*/
/** Implements BIOS handoff routine as decribed in EHCI spec
 *
 * @param[in] device Device asking for interrupts
 * @return Error code.
 */
int pci_disable_legacy(
    const ddf_dev_t *device, uintptr_t reg_base, size_t reg_size, int irq)
{
	assert(device);
	(void) pci_read16;
	(void) pci_read8;
	(void) pci_write16;

#define CHECK_RET_RETURN(ret, message...) \
	if (ret != EOK) { \
		usb_log_error(message); \
		return ret; \
	} else (void)0

	/* Map EHCI registers */
	void *regs = NULL;
	int ret = pio_enable((void*)reg_base, reg_size, &regs);
	CHECK_RET_RETURN(ret, "Failed to map registers %p: %s.\n",
	    (void *) reg_base, str_error(ret));

	const uint32_t hcc_params =
	    *(uint32_t*)(regs + HCC_PARAMS_OFFSET);
	usb_log_debug("Value of hcc params register: %x.\n", hcc_params);

	/* Read value of EHCI Extended Capabilities Pointer
	 * position of EEC registers (points to PCI config space) */
	const uint32_t eecp =
	    (hcc_params >> HCC_PARAMS_EECP_OFFSET) & HCC_PARAMS_EECP_MASK;
	usb_log_debug("Value of EECP: %x.\n", eecp);

	/* Read the first EEC. i.e. Legacy Support register */
	uint32_t usblegsup;
	ret = pci_read32(device, eecp + USBLEGSUP_OFFSET, &usblegsup);
	CHECK_RET_RETURN(ret, "Failed to read USBLEGSUP: %s.\n", str_error(ret));
	usb_log_debug("USBLEGSUP: %" PRIx32 ".\n", usblegsup);

	/* Request control from firmware/BIOS, by writing 1 to highest byte.
	 * (OS Control semaphore)*/
	usb_log_debug("Requesting OS control.\n");
	ret = pci_write8(device, eecp + USBLEGSUP_OFFSET + 3, 1);
	CHECK_RET_RETURN(ret, "Failed to request OS EHCI control: %s.\n",
	    str_error(ret));

	size_t wait = 0;
	/* Wait for BIOS to release control. */
	ret = pci_read32(device, eecp + USBLEGSUP_OFFSET, &usblegsup);
	while ((wait < DEFAULT_WAIT) && (usblegsup & USBLEGSUP_BIOS_CONTROL)) {
		async_usleep(WAIT_STEP);
		ret = pci_read32(device, eecp + USBLEGSUP_OFFSET, &usblegsup);
		wait += WAIT_STEP;
	}


	if ((usblegsup & USBLEGSUP_BIOS_CONTROL) == 0) {
		usb_log_info("BIOS released control after %zu usec.\n", wait);
	} else {
		/* BIOS failed to hand over control, this should not happen. */
		usb_log_warning( "BIOS failed to release control after "
		    "%zu usecs, force it.\n", wait);
		ret = pci_write32(device, eecp + USBLEGSUP_OFFSET,
		    USBLEGSUP_OS_CONTROL);
		CHECK_RET_RETURN(ret, "Failed to force OS control: %s.\n",
		    str_error(ret));
		/* Check capability type here, A value of 01h
		 * identifies the capability as Legacy Support.
		 * This extended capability requires one
		 * additional 32-bit register for control/status information,
		 * and this register is located at offset EECP+04h
		 * */
		if ((usblegsup & 0xff) == 1) {
			/* Read the second EEC
			 * Legacy Support and Control register */
			uint32_t usblegctlsts;
			ret = pci_read32(
			    device, eecp + USBLEGCTLSTS_OFFSET, &usblegctlsts);
			CHECK_RET_RETURN(ret,
			    "Failed to get USBLEGCTLSTS: %s.\n", str_error(ret));
			usb_log_debug("USBLEGCTLSTS: %" PRIx32 ".\n",
			    usblegctlsts);
			/* Zero SMI enables in legacy control register.
			 * It should prevent pre-OS code from interfering. */
			ret = pci_write32(device, eecp + USBLEGCTLSTS_OFFSET,
			    0xe0000000); /* three upper bits are WC */
			CHECK_RET_RETURN(ret,
			    "Failed(%d) zero USBLEGCTLSTS.\n", ret);
			udelay(10);
			ret = pci_read32(
			    device, eecp + USBLEGCTLSTS_OFFSET, &usblegctlsts);
			CHECK_RET_RETURN(ret,
			    "Failed to get USBLEGCTLSTS 2: %s.\n",
			    str_error(ret));
			usb_log_debug("Zeroed USBLEGCTLSTS: %" PRIx32 ".\n",
			    usblegctlsts);
		}
	}


	/* Read again Legacy Support register */
	ret = pci_read32(device, eecp + USBLEGSUP_OFFSET, &usblegsup);
	CHECK_RET_RETURN(ret, "Failed to read USBLEGSUP: %s.\n", str_error(ret));
	usb_log_debug("USBLEGSUP: %" PRIx32 ".\n", usblegsup);

	/*
	 * TURN OFF EHCI FOR NOW, DRIVER WILL REINITIALIZE IT
	 */

	/* Get size of capability registers in memory space. */
	const unsigned operation_offset = *(uint8_t*)regs;
	usb_log_debug("USBCMD offset: %d.\n", operation_offset);

	/* Zero USBCMD register. */
	volatile uint32_t *usbcmd =
	    (uint32_t*)((uint8_t*)regs + operation_offset + CMD_OFFSET);
	volatile uint32_t *usbsts =
	    (uint32_t*)((uint8_t*)regs + operation_offset + STS_OFFSET);
	volatile uint32_t *usbconf =
	    (uint32_t*)((uint8_t*)regs + operation_offset + CFG_OFFSET);
	volatile uint32_t *usbint =
	    (uint32_t*)((uint8_t*)regs + operation_offset + INT_OFFSET);
	usb_log_debug("USBCMD value: %x.\n", *usbcmd);
	if (*usbcmd & USBCMD_RUN) {
		*usbsts = 0x3f; /* ack all interrupts */
		*usbint = 0; /* disable all interrutps */
		*usbconf = 0; /* relase control of RH ports */

		*usbcmd = 0;
		/* Wait until hc is halted */
		while ((*usbsts & USBSTS_HALTED) == 0);
		usb_log_info("EHCI turned off.\n");
	} else {
		usb_log_info("EHCI was not running.\n");
	}
	usb_log_debug("Registers: \n"
	    "\t USBCMD: %x(0x00080000 = at least 1ms between interrupts)\n"
	    "\t USBSTS: %x(0x00001000 = HC halted)\n"
	    "\t USBINT: %x(0x0 = no interrupts).\n"
	    "\t CONFIG: %x(0x0 = ports controlled by companion hc).\n",
	    *usbcmd, *usbsts, *usbint, *usbconf);

	return ret;
#undef CHECK_RET_RETURN
}

/**
 * @}
 */
