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

/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */
/** @file
 * @brief PCI device interface definition.
 */

#ifndef LIBDRV_PCI_DEV_IFACE_H_
#define LIBDRV_PCI_DEV_IFACE_H_

#include "ddf/driver.h"

typedef enum {
	IPC_M_CONFIG_SPACE_READ_8,
	IPC_M_CONFIG_SPACE_READ_16,
	IPC_M_CONFIG_SPACE_READ_32,

	IPC_M_CONFIG_SPACE_WRITE_8,
	IPC_M_CONFIG_SPACE_WRITE_16,
	IPC_M_CONFIG_SPACE_WRITE_32
} pci_dev_iface_funcs_t;

/** PCI device communication interface. */
typedef struct {
	int (*config_space_read_8)(ddf_fun_t *, uint32_t address, uint8_t *data);
	int (*config_space_read_16)(ddf_fun_t *, uint32_t address, uint16_t *data);
	int (*config_space_read_32)(ddf_fun_t *, uint32_t address, uint32_t *data);

	int (*config_space_write_8)(ddf_fun_t *, uint32_t address, uint8_t data);
	int (*config_space_write_16)(ddf_fun_t *, uint32_t address, uint16_t data);
	int (*config_space_write_32)(ddf_fun_t *, uint32_t address, uint32_t data);
} pci_dev_iface_t;


#endif
/**
 * @}
 */

