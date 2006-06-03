#!/bin/sh
# Uspace addresses outside of normal memory (kernel has std. 8 or 16MB)
# we place the pages at 24M
	gxemul $@ -E testmips -X 0x81800000:../uspace/ns/ns 0x81900000:../uspace/kbd/kbd 0x81a00000:../uspace/fb/fb 0x81b00000:../uspace/init/init 0x81c00000:../uspace/console/console 0x81d00000:../uspace/tetris/tetris kernel.bin 
