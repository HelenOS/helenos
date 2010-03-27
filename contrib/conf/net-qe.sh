#!/bin/sh

arguments="-vga std -M isapc -net nic,model=ne2k_isa -net user -boot d -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -cdrom image.iso"

#qemu 0.10.2 and later 0.10.*:
#qemu $@ -no-kqemu $arguments

#qemu 0.11:
qemu $@ $arguments
