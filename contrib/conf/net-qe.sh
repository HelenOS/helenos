#!/bin/sh

# Obsolete versions of QEMU
#
#   QEMU 0.10.2 and later 0.10.*
#    qemu $@ -no-kqemu -vga std -M isapc -net nic,model=ne2k_isa -net user -boot d -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -cdrom image.iso
#
#   QEMU 0.11.* and 0.12.*
#    qemu $@ -vga std -M isapc -net nic,model=ne2k_isa -net user -boot d -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -cdrom image.iso

# QEMU 0.13 and later
qemu $@ -device ne2k_isa,irq=5,vlan=0 -net user -boot d -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -cdrom image.iso
