#!/bin/sh

qemu-system-arm $@ -M integratorcp --kernel image.boot
