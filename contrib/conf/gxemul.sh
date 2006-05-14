#!/bin/sh
# Uspace addresses outside of normal memory (kernel has std. 8 or 16MB)
# we place the pages at 24M
	gxemul $@ -E testmips -X 0x81800000:ns 0x81900000:init kernel.bin 
