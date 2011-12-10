/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <sys/types.h>
#include <abi/ddi/arg.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <libc.h>
#include <task.h>
#include <as.h>
#include <align.h>
#include <libarch/config.h>

/** Return unique device number.
 *
 * @return New unique device number.
 *
 */
int device_assign_devno(void)
{
	return __SYSCALL0(SYS_DEVICE_ASSIGN_DEVNO);
}

/** Map a piece of physical memory to task.
 *
 * Caller of this function must have the CAP_MEM_MANAGER capability.
 *
 * @param pf    Physical address of the starting frame.
 * @param vp    Virtual address of the starting page.
 * @param pages Number of pages to map.
 * @param flags Flags for the new address space area.
 *
 * @return EOK on success
 * @return EPERM if the caller lacks the CAP_MEM_MANAGER capability
 * @return ENOENT if there is no task with specified ID
 * @return ENOMEM if there was some problem in creating
 *         the address space area.
 *
 */
int physmem_map(void *pf, void *vp, size_t pages, unsigned int flags)
{
	return __SYSCALL4(SYS_PHYSMEM_MAP, (sysarg_t) pf, (sysarg_t) vp,
	    pages, flags);
}

/** Enable I/O space range to task.
 *
 * Caller of this function must have the IO_MEM_MANAGER capability.
 *
 * @param id		Task ID.
 * @param ioaddr	Starting address of the I/O range.
 * @param size		Size of the range.
 *
 * @return		0 on success, EPERM if the caller lacks the
 *			CAP_IO_MANAGER capability, ENOENT if there is no task
 *			with specified ID and ENOMEM if there was some problem
 *			in allocating memory.
 */
int iospace_enable(task_id_t id, void *ioaddr, unsigned long size)
{
	ddi_ioarg_t arg;

	arg.task_id = id;
	arg.ioaddr = ioaddr;
	arg.size = size;

	return __SYSCALL1(SYS_IOSPACE_ENABLE, (sysarg_t) &arg);
}

/** Enable PIO for specified I/O range.
 *
 * @param pio_addr	I/O start address.
 * @param size		Size of the I/O region.
 * @param use_addr	Address where the final address for application's PIO
 * 			will be stored.
 *
 * @return		Zero on success or negative error code.
 */
int pio_enable(void *pio_addr, size_t size, void **use_addr)
{
	void *phys;
	void *virt;
	size_t offset;
	unsigned int pages;

#ifdef IO_SPACE_BOUNDARY
	if (pio_addr < IO_SPACE_BOUNDARY) {
		*use_addr = pio_addr;
		return iospace_enable(task_get_id(), pio_addr, size);
	}
#endif

	phys = (void *) ALIGN_DOWN((uintptr_t) pio_addr, PAGE_SIZE);
	offset = pio_addr - phys;
	pages = ALIGN_UP(offset + size, PAGE_SIZE) >> PAGE_WIDTH;
	virt = as_get_mappable_page(pages << PAGE_WIDTH);
	*use_addr = virt + offset;
	return physmem_map(phys, virt, pages, AS_AREA_READ | AS_AREA_WRITE);
}

/** Register IRQ notification.
 *
 * @param inr    IRQ number.
 * @param devno  Device number of the device generating inr.
 * @param method Use this method for notifying me.
 * @param ucode  Top-half pseudocode handler.
 *
 * @return Value returned by the kernel.
 *
 */
int register_irq(int inr, int devno, int method, irq_code_t *ucode)
{
	return __SYSCALL4(SYS_REGISTER_IRQ, inr, devno, method,
	    (sysarg_t) ucode);
}

/** Unregister IRQ notification.
 *
 * @param inr   IRQ number.
 * @param devno Device number of the device generating inr.
 *
 * @return Value returned by the kernel.
 *
 */
int unregister_irq(int inr, int devno)
{
	return __SYSCALL2(SYS_UNREGISTER_IRQ, inr, devno);
}

/** @}
 */
