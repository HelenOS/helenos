#!/bin/sh

qemu-system-ppc $@ -M g3beige -boot d -cdrom image.iso
