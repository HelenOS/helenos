#include <stdio.h>
#include <stdlib.h>
#include <async.h>

#include "intel_piix3.h"
#include "pci.h"
#include "pci_bus.h"
#include "isa.h"
#include "serial.h"


int main(int argc, char **argv) 
{
	printf("PCI bus driver\n");
	
	if (!pci_bus_init()) {
		printf("PCI bus initialization failed.\n");
		return 1;
	}
	
	if (!isa_bus_init()) {
		printf("ISA bus initialization failed.\n");
		return 1;
	}
	
	
	// pci-to-isa bridge device
	intel_piix3_init();
	
	// serial port driver
	serial_init();
	
	printf("PCI + ISA + serial: Accepting connections\n");
    async_manager();
	
	return 0;
}
