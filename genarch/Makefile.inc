#
# Open Firmware
#
OFW=no

ifeq ($(ARCH),ppc32)
OFW=yes
endif

ifeq ($(ARCH),sparc64)
OFW=yes
endif

#
# Advanced Configuration and Power Interface (ACPI)
#
ACPI=no

ifeq ($(ARCH),ia32)
ACPI=yes
endif

ifeq ($(ARCH),amd64)
ACPI=yes
endif

ifeq ($(ARCH),ia64)
#ACPI=yes
endif


ifeq ($(OFW),yes)
genarch_sources+=src/genarch/firmware/ofw/ofw.c
endif

ifeq ($(ACPI),yes)
genarch_sources+=src/genarch/firmware/acpi/acpi.c \
		 src/genarch/firmware/acpi/madt.c
endif
