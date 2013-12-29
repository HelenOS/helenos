/*
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup sparc32boot
 * @{
 */
/** @file
 * @brief Bootstrap.
 */

#include <arch/asm.h>
#include <arch/common.h>
#include <arch/arch.h>
#include <arch/ambapp.h>
#include <arch/mm.h>
#include <arch/main.h>
#include <arch/_components.h>
#include <halt.h>
#include <printf.h>
#include <memstr.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <str.h>
#include <errno.h>

uintptr_t amba_uart_base;

static amba_device_t amba_devices[AMBAPP_MAX_DEVICES];
static unsigned int amba_devices_found;
static bool amba_fake;

static void ambapp_scan_area(uintptr_t, unsigned int);

void ambapp_scan()
{
	amba_fake = false;
	
	/* Scan for AHB masters & slaves */
	ambapp_scan_area(AMBAPP_AHBMASTER_AREA, 64);
	ambapp_scan_area(AMBAPP_AHBSLAVE_AREA, 63);
	
	/* Scan for APB slaves on APBMST */
	amba_device_t *apbmst = ambapp_lookup_first(GAISLER, GAISLER_APBMST);
	if (apbmst != NULL)
		ambapp_scan_area(apbmst->bars[0].start, 16);
	
	/* If we found nothing, fake device entries */
	if (amba_devices_found == 0)
		ambapp_qemu_fake_scan();
}

static void ambapp_scan_area(uintptr_t master_bar, unsigned int max_entries)
{
	ambapp_entry_t *entry = (ambapp_entry_t *) (master_bar | AMBAPP_CONF_AREA);
	
	for (unsigned int i = 0; i < max_entries; i++) {
		if (amba_devices_found == AMBAPP_MAX_DEVICES)
			return;
		
		if (entry->vendor_id == 0xff)
			continue;
		
		amba_device_t *device = &amba_devices[amba_devices_found];
		device->vendor_id = (amba_vendor_id_t) entry->vendor_id;
		device->device_id = (amba_device_id_t) entry->device_id;
		device->version = entry->version;
		device->irq = entry->irq;
		
		for (unsigned int bar = 0; bar < 4; bar++) {
			device->bars[bar].start = entry->bar[bar].addr << 20;
			device->bars[bar].size = entry->bar[bar].mask;
			device->bars[bar].prefetchable = (bool) entry->bar[bar].prefetchable;
			device->bars[bar].cacheable = (bool) entry->bar[bar].cacheable;
		}
		
		amba_devices_found++;
	}
}

void ambapp_qemu_fake_scan()
{
	/* UART */
	amba_devices[0].vendor_id = GAISLER;
	amba_devices[0].device_id = GAISLER_APBUART;
	amba_devices[0].version = 1;
	amba_devices[0].irq = 3;
	amba_devices[0].bars[0].start = 0x80000100;
	amba_devices[0].bars[0].size = 0x100;
	
	/* IRQMP */
	amba_devices[1].vendor_id = GAISLER;
	amba_devices[1].device_id = GAISLER_IRQMP;
	amba_devices[1].version = 1;
	amba_devices[1].irq = -1;
	amba_devices[1].bars[0].start = 0x80000200;
	amba_devices[1].bars[0].size = 0x100;
	
	/* GPTIMER */
	amba_devices[2].vendor_id = GAISLER;
	amba_devices[2].device_id = GAISLER_GPTIMER;
	amba_devices[2].version = 1;
	amba_devices[2].irq = 8;
	amba_devices[2].bars[0].start = 0x80000300;
	amba_devices[2].bars[0].size = 0x100;
	
	amba_fake = true;
	amba_devices_found = 3;
}

bool ambapp_fake()
{
	return amba_fake;
}

void ambapp_print_devices()
{
	printf("ABMA devices:\n");
	
	for (unsigned int i = 0; i < amba_devices_found; i++) {
		amba_device_t *dev = &amba_devices[i];
		
		printf("<%1x:%03x> at 0x%08x ", dev->vendor_id, dev->device_id,
		    dev->bars[0].start);
		
		if (dev->irq == -1)
			printf("\n");
		else
			printf("irq %d\n", dev->irq);
	}
}

amba_device_t *ambapp_lookup_first(amba_vendor_id_t vendor,
    amba_device_id_t device)
{
	for (unsigned int i = 0; i < amba_devices_found; i++) {
		if ((amba_devices[i].vendor_id == vendor) &&
		    (amba_devices[i].device_id == device))
			return &amba_devices[i];
	}
	
	return NULL;
}
