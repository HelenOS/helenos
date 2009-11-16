#ifndef PCI_CONF_H
#define PCI_CONF_H

#include "pci.h"

uint8_t pci_conf_read_8(pci_dev_t *dev, int reg);
uint16_t pci_conf_read_16(pci_dev_t *dev, int reg);
uint32_t pci_conf_read_32(pci_dev_t *dev, int reg);

#endif
