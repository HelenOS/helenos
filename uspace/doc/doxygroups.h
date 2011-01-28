
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
		 * @defgroup netif Network interface drivers
		 * @ingroup net
		 */

			/**
			 * @defgroup lo Loopback Service
			 * @ingroup netif
			 */

			/**
			 * @defgroup ne2000 NE2000 network interface service
			 * @ingroup netif
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
	 * @defgroup libusb USB library
	 * @ingroup usb
	 * @brief Library for creating USB devices drivers.
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
	  * @defgroup drvusbuhci UHCI driver
	  * @ingroup usb
	  * @brief Driver for USB host controller UHCI.
	  */

