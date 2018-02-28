
/* Definitions of modules and its relations for generating Doxygen documentation */

/**
 * @defgroup srvcs HelenOS Services
 * @ingroup uspace
 */

	/**
	 * @defgroup ns Naming Service
	 * @ingroup srvcs
	 */

	/**
	 * @defgroup kbd Keyboard Service
	 * @ingroup srvcs
	 */

	/**
	 * @defgroup fbs Framebuffer Service
	 * @ingroup srvcs
	 */

	/**
	 * @defgroup console Console Service
	 * @ingroup srvcs
	 */

	/**
	 * @defgroup net Networking Stack
	 * @ingroup srvcs
	 */

		/**
		 * @defgroup nic Network interface controllers
		 * @ingroup net
		 */

			/**
			 * @defgroup libnic Base NIC framework library
			 * @ingroup nic
			 */

			/**
			 * @defgroup nic_drivers Drivers using the NICF
			 * @ingroup nic
			 */

		/**
		 * @defgroup net_nil Network interface layer
		 * @ingroup net
		 */

			/**
			 * @defgroup eth Ethernet (IEEE 802.3) network interface layer Service
			 * @ingroup net_nil
			 */

			/**
			 * @defgroup nildummy Dummy network interface layer Service
			 * @ingroup net_nil
			 */

		/**
		 * @defgroup net_il Inter-networking layer
		 * @ingroup net
		 */

			/**
			 * @defgroup arp Address Resolution Protocol (ARP) Service
			 * @ingroup net_il
			 */

			/**
			 * @defgroup ip Internet Protocol (IP) Service
			 * @ingroup net_il
			 */

		/**
		 * @defgroup net_tl Transport layer
		 * @ingroup net
		 */

			/**
			 * @defgroup icmp Internet Control Message Protocol (ICMP) Service
			 * @ingroup net_tl
			 */

			/**
			 * @defgroup udp User Datagram Protocol (UDP) Service
			 * @ingroup net_tl
			 */

			/**
			 * @defgroup tcp Transmission Control Protocol (TCP) Service
			 * @ingroup net_tl
			 */

		/**
		 * @defgroup packet Packet management system
		 * @ingroup net
		 */

		/**
		 * @defgroup net_app Applications
		 * @ingroup net
		 */

			/**
			 * @defgroup echo Echo Service
			 * @ingroup net_app
			 */

			/**
			 * @defgroup ping Ping
			 * @ingroup net_app
			 */

			/**
			 * @defgroup nettest Networking tests
			 * @ingroup net_app
			 */

		/**
		 * @defgroup net_lib Application library
		 * @ingroup net
		 */

			/**
			 * @defgroup socket Sockets
			 * @ingroup net_lib
			 */

			/**
			 * @defgroup netdb Netdb
			 * @ingroup net_lib
			 */

	/**
	 * @cond amd64
	 * @defgroup pci PCI Service
	 * @ingroup srvcs
	 * @endcond
	 */

	/**
	 * @cond ia32
	 * @defgroup pci PCI Service
	 * @ingroup srvcs
	 * @endcond
	 */

/**
 * @defgroup emul Emulation Libraries
 * @ingroup uspace
 */

 	/**
	 * @defgroup sfl Softloat
	 * @ingroup emul
	 */

	/**
	 * @defgroup softint Softint
	 * @ingroup emul
	 */

/**
 * @defgroup usb USB
 * @ingroup uspace
 * @brief USB support for HelenOS.
 */
	/**
	 * @defgroup libusb Base USB library
	 * @ingroup usb
	 * @brief Common definitions for any driver or application
	 * dealing with USB.
	 */

	/**
	 * @defgroup libusbdev USB library for device drivers
	 * @ingroup usb
	 * @brief Library for writing drivers of endpoint devices (functions).
	 */

	/**
	 * @defgroup libusbhost USB library for host controller drivers
	 * @ingroup usb
	 * @brief Library for writing host controller drivers.
	 */

	/**
	 * @defgroup libusbhid USB library for HID devices
	 * @ingroup usb
	 * @brief Library for writing USB HID drivers.
	 */

	/**
	 * @defgroup usbvirt USB virtualization
	 * @ingroup usb
	 * @brief Support for virtual USB devices.
	 */

		/**
		 * @defgroup libusbvirt USB virtualization library
		 * @ingroup usbvirt
		 * @brief Library for creating virtual USB devices.
		 */

		/**
		 * @defgroup drvusbvhc Virtual USB host controller
		 * @ingroup usbvirt
		 * @brief Driver simulating work of USB host controller.
		 */

		/**
		 * @defgroup usbvirthub Virtual USB hub
		 * @ingroup usbvirt
		 * @brief Extra virtual USB hub for virtual host controller.
		 * @details
		 * Some of the sources are shared with virtual host controller,
		 * see @ref drvusbvhc for the rest of the files.
		 */

		/**
		 * @defgroup usbvirtkbd Virtual USB keybaord
		 * @ingroup usbvirt
		 * @brief Virtual USB keyboard for virtual host controller.
		 */

	/**
	 * @defgroup usbinfo USB info application
	 * @ingroup usb
	 * @brief Application for querying USB devices.
	 * @details
	 * The intended usage of this application is to query new USB devices
	 * for their descriptors etc. to simplify driver writing.
	 */

	/**
	 * @defgroup lsusb HelenOS version of lsusb command
	 * @ingroup usb
	 * @brief Application for listing USB host controllers.
	 * @details
	 * List all found host controllers.
	 */

	/**
	 * @defgroup drvusbmid USB multi interface device driver
	 * @ingroup usb
	 * @brief USB multi interface device driver
	 * @details
	 * This driver serves as a mini hub (or bus) driver for devices
	 * that have the class defined at interface level (those devices
	 * usually have several interfaces).
	 *
	 * The term multi interface device driver (MID) was borrowed
	 * Solaris operating system.
	 */

	/**
	 * @defgroup drvusbhub USB hub driver
	 * @ingroup usb
	 * @brief USB hub driver.
	 */

	/**
	 * @defgroup drvusbhid USB HID driver
	 * @ingroup usb
	 * @brief USB driver for HID devices.
	 */

	/**
	 * @defgroup drvusbmast USB mass storage driver
	 * @ingroup usb
	 * @brief USB driver for mass storage devices (bulk-only protocol).
	 * This driver is a only a stub and is currently used only for
	 * testing that bulk transfers work.
	 */

	/**
	 * @defgroup drvusbuhci UHCI driver
	 * @ingroup usb
	 * @brief Drivers for USB UHCI host controller and root hub.
	 */

	/**
	 * @defgroup drvusbohci OHCI driver
	 * @ingroup usb
	 * @brief Driver for OHCI host controller.
	 */

	/**
	 * @defgroup drvusbehci EHCI driver
	 * @ingroup usb
	 * @brief Driver for EHCI host controller.
	 */

	/**
	 * @defgroup drvusbfallback USB fallback driver
	 * @ingroup usb
	 * @brief Fallback driver for any USB device.
	 * @details
	 * The purpose of this driver is to simplify querying of unknown
	 * devices from within HelenOS (without a driver, no node at all
	 * may appear under /loc/devices).
	 */


