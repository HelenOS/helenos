/**
 @mainpage USB support for HelenOS

This is reference manual for USB subsystem for HelenOS.

HelenOS is a microkernel operating system where most of the functionality
is provided by userspace tasks rather than system calls.
This includes file system service, networking and also device drivers.

USB is a standard for communication between host computers and computer
peripherals.
The advantage of USB over other kinds of connection between a computer
and a peripheral is its flexibility.
USB supports plugging and unplugging of devices without need to turn
off any of the components.
The data flow model is designed to accommodate together peripherals
with different transfer characteristics.

Adding support for USB devices to HelenOS shall improve the usability
of the system in general even if the support concerns only USB 1.1 and
the only peripheral drivers are for keyboards and mice.

This reference documentation can be found also on-line at homepage
of HelenOS USB project at http://helenos-usb.sourceforge.net/

*/
