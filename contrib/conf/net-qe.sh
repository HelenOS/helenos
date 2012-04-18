#!/bin/sh

case "$1" in
	ne2k)
		shift
		qemu $@ -device ne2k_isa,irq=5,vlan=0 -net user -boot d -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -cdrom image.iso
		;;
	e1k)
		shift
		qemu $@ -device e1000,vlan=0 -net user -boot d -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -cdrom image.iso
		;;
	rtl8139)
		shift
		qemu $@ -device rtl8139,vlan=0 -net user -boot d -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -cdrom image.iso
		;;
	*)
		echo "Usage: $0 {ne2k|e1k|rtl8139}"
esac
