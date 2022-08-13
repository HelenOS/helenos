/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#include <errno.h>

#include "ddf/driver.h"

#define PCI_VENDOR_ID	0x00
#define PCI_DEVICE_ID	0x02
#define PCI_STATUS 	0x06
#define PCI_SUB_CLASS	0x0A
#define PCI_BASE_CLASS	0x0B
#define PCI_BAR0	0x10
#define PCI_CAP_PTR	0x34

#define PCI_BAR_COUNT	6

#define PCI_STATUS_CAP_LIST	(1 << 4)

#define PCI_CAP_ID(c)	((c) + 0x0)
#define PCI_CAP_NEXT(c)	((c) + 0x1)

#define PCI_CAP_PMID		0x1
#define PCI_CAP_VENDORSPECID	0x9

extern errno_t pci_config_space_read_8(async_sess_t *, uint32_t, uint8_t *);
extern errno_t pci_config_space_read_16(async_sess_t *, uint32_t, uint16_t *);
extern errno_t pci_config_space_read_32(async_sess_t *, uint32_t, uint32_t *);

extern errno_t pci_config_space_write_8(async_sess_t *, uint32_t, uint8_t);
extern errno_t pci_config_space_write_16(async_sess_t *, uint32_t, uint16_t);
extern errno_t pci_config_space_write_32(async_sess_t *, uint32_t, uint32_t);

static inline errno_t
pci_config_space_cap_first(async_sess_t *sess, uint8_t *c, uint8_t *id)
{
	errno_t rc;
	uint16_t status;

	rc = pci_config_space_read_16(sess, PCI_STATUS, &status);
	if (rc != EOK)
		return rc;

	if (!(status & PCI_STATUS_CAP_LIST)) {
		*c = 0;
		return EOK;
	}

	rc = pci_config_space_read_8(sess, PCI_CAP_PTR, c);
	if (rc != EOK)
		return rc;
	if (!c)
		return EOK;
	return pci_config_space_read_8(sess, PCI_CAP_ID(*c), id);
}

static inline errno_t
pci_config_space_cap_next(async_sess_t *sess, uint8_t *c, uint8_t *id)
{
	errno_t rc = pci_config_space_read_8(sess, PCI_CAP_NEXT(*c), c);
	if (rc != EOK)
		return rc;
	if (!c)
		return EOK;
	return pci_config_space_read_8(sess, PCI_CAP_ID(*c), id);
}

/** PCI device communication interface. */
typedef struct {
	errno_t (*config_space_read_8)(ddf_fun_t *, uint32_t address, uint8_t *data);
	errno_t (*config_space_read_16)(ddf_fun_t *, uint32_t address, uint16_t *data);
	errno_t (*config_space_read_32)(ddf_fun_t *, uint32_t address, uint32_t *data);

	errno_t (*config_space_write_8)(ddf_fun_t *, uint32_t address, uint8_t data);
	errno_t (*config_space_write_16)(ddf_fun_t *, uint32_t address, uint16_t data);
	errno_t (*config_space_write_32)(ddf_fun_t *, uint32_t address, uint32_t data);
} pci_dev_iface_t;

#endif
/**
 * @}
 */
