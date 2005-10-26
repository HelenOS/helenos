# Open Firmware
OFW=no

ifeq ($(ARCH),ppc32)
OFW=yes
endif

ifeq ($(ARCH),sparc64)
OFW=yes
endif

ifeq ($(OFW),yes)
genarch_sources+=src/genarch/firmware/ofw/ofw.c
endif
