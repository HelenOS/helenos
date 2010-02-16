
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
			 * @defgroup dp8390 Generic DP8390 network interface family service
			 * @ingroup netif
			 */

				/**
				 * @defgroup ne2k NE2000 network interface family
				 * @ingroup dp8390
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
