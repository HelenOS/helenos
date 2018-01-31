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

#include <assert.h>
#include <atomic.h>
#include <stdio.h>
#include <errno.h>
#include <abi/ddi/arg.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <device/hw_res.h>
#include <device/hw_res_parsed.h>
#include <device/pio_window.h>
#include <libc.h>
#include <task.h>
#include <as.h>
#include <align.h>
#include <libarch/config.h>
#include "private/libc.h"


/** Map a piece of physical memory to task.
 *
 * Caller of this function must have the PERM_MEM_MANAGER permission.
 *
 * @param phys  Physical address of the starting frame.
 * @param pages Number of pages to map.
 * @param flags Flags for the new address space area.
 * @param virt  Virtual address of the starting page.
 *              If set to AS_AREA_ANY ((void *) -1), a suitable value
 *              is found by the kernel, otherwise the kernel tries to
 *              obey the desired value.
 *
 * @return EOK on success.
 * @return EPERM if the caller lacks the PERM_MEM_MANAGER permission.
 * @return ENOMEM if there was some problem in creating
 *         the address space area.
 *
 */
errno_t physmem_map(uintptr_t phys, size_t pages, unsigned int flags, void **virt)
{
	return (errno_t) __SYSCALL5(SYS_PHYSMEM_MAP, (sysarg_t) phys,
	    pages, flags, (sysarg_t) virt, (sysarg_t) __entry);
}

/** Unmap a piece of physical memory to task.
 *
 * Caller of this function must have the PERM_MEM_MANAGER permission.
 *
 * @param virt Virtual address from the phys-mapped region.
 *
 * @return EOK on success.
 * @return EPERM if the caller lacks the PERM_MEM_MANAGER permission.
 *
 */
errno_t physmem_unmap(void *virt)
{
	return (errno_t) __SYSCALL1(SYS_PHYSMEM_UNMAP, (sysarg_t) virt);
}

/** Lock a piece physical memory for DMA transfers.
 *
 * The mapping of the specified virtual memory address
 * to physical memory address is locked in order to
 * make it safe for DMA transferts.
 *
 * Caller of this function must have the PERM_MEM_MANAGER permission.
 *
 * @param virt      Virtual address of the memory to be locked.
 * @param size      Number of bytes to lock.
 * @param map_flags Desired virtual memory area flags.
 * @param flags     Flags for the physical memory address.
 * @param phys      Locked physical memory address.
 *
 * @return EOK on success.
 * @return EPERM if the caller lacks the PERM_MEM_MANAGER permission.
 * @return ENOMEM if there was some problem in creating
 *         the address space area.
 *
 */
errno_t dmamem_map(void *virt, size_t size, unsigned int map_flags,
    unsigned int flags, uintptr_t *phys)
{
	return (errno_t) __SYSCALL6(SYS_DMAMEM_MAP, (sysarg_t) size,
	    (sysarg_t) map_flags, (sysarg_t) flags & ~DMAMEM_FLAGS_ANONYMOUS,
	    (sysarg_t) phys, (sysarg_t) virt, 0);
}

/** Map a piece of physical memory suitable for DMA transfers.
 *
 * Caller of this function must have the PERM_MEM_MANAGER permission.
 *
 * @param size       Number of bytes to map.
 * @param constraint Bit mask defining the contraint on the physical
 *                   address to be mapped.
 * @param map_flags  Desired virtual memory area flags.
 * @param flags      Flags for the physical memory address.
 * @param virt       Virtual address of the starting page.
 *                   If set to AS_AREA_ANY ((void *) -1), a suitable value
 *                   is found by the kernel, otherwise the kernel tries to
 *                   obey the desired value.
 *
 * @return EOK on success.
 * @return EPERM if the caller lacks the PERM_MEM_MANAGER permission.
 * @return ENOMEM if there was some problem in creating
 *         the address space area.
 *
 */
errno_t dmamem_map_anonymous(size_t size, uintptr_t constraint,
    unsigned int map_flags, unsigned int flags, uintptr_t *phys, void **virt)
{
	*phys = constraint;
	
	return (errno_t) __SYSCALL6(SYS_DMAMEM_MAP, (sysarg_t) size,
	    (sysarg_t) map_flags, (sysarg_t) flags | DMAMEM_FLAGS_ANONYMOUS,
	    (sysarg_t) phys, (sysarg_t) virt, (sysarg_t) __entry);
}

errno_t dmamem_unmap(void *virt, size_t size)
{
	return (errno_t) __SYSCALL3(SYS_DMAMEM_UNMAP, (sysarg_t) virt, (sysarg_t) size, 0);
}

errno_t dmamem_unmap_anonymous(void *virt)
{
	return (errno_t) __SYSCALL3(SYS_DMAMEM_UNMAP, (sysarg_t) virt, 0,
	    DMAMEM_FLAGS_ANONYMOUS);
}

/** Enable I/O space range to task.
 *
 * Caller of this function must have the PERM_IO_MANAGER permission.
 *
 * @param id     Task ID.
 * @param ioaddr Starting address of the I/O range.
 * @param size   Size of the range.
 *
 * @return EOK on success
 * @return EPERM if the caller lacks the PERM_IO_MANAGER permission
 * @return ENOENT if there is no task with specified ID
 * @return ENOMEM if there was some problem in allocating memory.
 *
 */
static errno_t iospace_enable(task_id_t id, void *ioaddr, size_t size)
{
	const ddi_ioarg_t arg = {
		.task_id = id,
		.ioaddr = ioaddr,
		.size = size
	};
	
	return (errno_t) __SYSCALL1(SYS_IOSPACE_ENABLE, (sysarg_t) &arg);
}

/** Disable I/O space range to task.
 *
 * Caller of this function must have the PERM_IO_MANAGER permission.
 *
 * @param id     Task ID.
 * @param ioaddr Starting address of the I/O range.
 * @param size   Size of the range.
 *
 * @return EOK on success
 * @return EPERM if the caller lacks the PERM_IO_MANAGER permission
 * @return ENOENT if there is no task with specified ID
 *
 */
static errno_t iospace_disable(task_id_t id, void *ioaddr, size_t size)
{
	const ddi_ioarg_t arg = {
		.task_id = id,
		.ioaddr = ioaddr,
		.size = size
	};
	
	return (errno_t) __SYSCALL1(SYS_IOSPACE_DISABLE, (sysarg_t) &arg);
}

/** Enable PIO for specified address range.
 *
 * @param range I/O range to be enable.
 * @param virt  Virtual address for application's PIO operations. 
 */
errno_t pio_enable_range(addr_range_t *range, void **virt)
{
	return pio_enable(RNGABSPTR(*range), RNGSZ(*range), virt);
}

/** Enable PIO for specified HW resource wrt. to the PIO window.
 *
 * @param win      PIO window. May be NULL if the resources are known to be
 *                 absolute.
 * @param res      Resources specifying the I/O range wrt. to the PIO window.
 * @param virt     Virtual address for application's PIO operations.
 *
 * @return EOK on success.
 * @return An error code on failure.
 *
 */
errno_t pio_enable_resource(pio_window_t *win, hw_resource_t *res, void **virt)
{
	uintptr_t addr;
	size_t size;

	switch (res->type) {
	case IO_RANGE:
		addr = res->res.io_range.address;
		if (res->res.io_range.relative) {
			if (!win)
				return EINVAL;
			addr += win->io.base;
		}
		size = res->res.io_range.size;
		break;
	case MEM_RANGE:
		addr = res->res.mem_range.address;
		if (res->res.mem_range.relative) {
			if (!win)
				return EINVAL;
			addr += win->mem.base;
		}
		size = res->res.mem_range.size;
		break;
	default:
		return EINVAL;
	}

	return pio_enable((void *) addr, size, virt);	
}

/** Enable PIO for specified I/O range.
 *
 * @param pio_addr I/O start address.
 * @param size     Size of the I/O region.
 * @param virt     Virtual address for application's
 *                 PIO operations. Can be NULL for PMIO.
 *
 * @return EOK on success.
 * @return An error code on failure.
 *
 */
errno_t pio_enable(void *pio_addr, size_t size, void **virt)
{
#ifdef IO_SPACE_BOUNDARY
	if (pio_addr < IO_SPACE_BOUNDARY) {
		if (virt)
			*virt = pio_addr;
		return iospace_enable(task_get_id(), pio_addr, size);
	}
#else
	(void) iospace_enable;
#endif
	if (!virt)
		return EINVAL;
	
	uintptr_t phys_frame =
	    ALIGN_DOWN((uintptr_t) pio_addr, PAGE_SIZE);
	size_t offset = (uintptr_t) pio_addr - phys_frame;
	size_t pages = SIZE2PAGES(offset + size);
	
	void *virt_page = AS_AREA_ANY;
	errno_t rc = physmem_map(phys_frame, pages,
	    AS_AREA_READ | AS_AREA_WRITE, &virt_page);
	if (rc != EOK)
		return rc;
	
	*virt = virt_page + offset;
	return EOK;
}

/** Disable PIO for specified I/O range.
 *
 * @param virt     I/O start address.
 * @param size     Size of the I/O region.
 *
 * @return EOK on success.
 * @return An error code on failure.
 *
 */
errno_t pio_disable(void *virt, size_t size)
{
#ifdef IO_SPACE_BOUNDARY
	if (virt < IO_SPACE_BOUNDARY)
		return iospace_disable(task_get_id(), virt, size);
#else
	(void) iospace_disable;
#endif
	return physmem_unmap(virt);
}

void pio_write_8(ioport8_t *reg, uint8_t val)
{
	pio_trace_log(reg, val, true);
	arch_pio_write_8(reg, val);
}

void pio_write_16(ioport16_t *reg, uint16_t val)
{
	pio_trace_log(reg, val, true);
	arch_pio_write_16(reg, val);
}

void pio_write_32(ioport32_t *reg, uint32_t val)
{
	pio_trace_log(reg, val, true);
	arch_pio_write_32(reg, val);
}

void pio_write_64(ioport64_t *reg, uint64_t val)
{
	pio_trace_log(reg, val, true);
	arch_pio_write_64(reg, val);
}

uint8_t pio_read_8(const ioport8_t *reg)
{
	const uint8_t val = arch_pio_read_8(reg);
	pio_trace_log(reg, val, false);
	return val;
}

uint16_t pio_read_16(const ioport16_t *reg)
{
	const uint16_t val = arch_pio_read_16(reg);
	pio_trace_log(reg, val, false);
	return val;
}

uint32_t pio_read_32(const ioport32_t *reg)
{
	const uint32_t val = arch_pio_read_32(reg);
	pio_trace_log(reg, val, false);
	return val;
}

uint64_t pio_read_64(const ioport64_t *reg)
{
	const uint64_t val = arch_pio_read_64(reg);
	pio_trace_log(reg, val, false);
	return val;
}

/** @}
 */
