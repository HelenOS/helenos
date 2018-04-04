/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Radim Vansa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libc
 * @{
 */

/** @file
 * NIC interface definitions.
 */

#ifndef LIBC_NIC_H_
#define LIBC_NIC_H_

#include <nic/eth_phys.h>
#include <stdbool.h>

/** Ethernet address length. */
#define ETH_ADDR  6

/** MAC printing format */
#define PRIMAC  "%02x:%02x:%02x:%02x:%02x:%02x"

/** MAC arguments */
#define ARGSMAC(__a) \
	(__a)[0], (__a)[1], (__a)[2], (__a)[3], (__a)[4], (__a)[5]

/* Compare MAC address with specific value */
#define MAC_EQUALS_VALUE(__a, __a0, __a1, __a2, __a3, __a4, __a5) \
	((__a)[0] == (__a0) && (__a)[1] == (__a1) && (__a)[2] == (__a2) \
	&& (__a)[3] == (__a3) && (__a)[4] == (__a4) && (__a)[5] == (__a5))

#define MAC_IS_ZERO(__x) \
	MAC_EQUALS_VALUE(__x, 0, 0, 0, 0, 0, 0)

/** Max length of any hw nic address (currently only eth) */
#define NIC_MAX_ADDRESS_LENGTH  16

#define NIC_VENDOR_MAX_LENGTH         64
#define NIC_MODEL_MAX_LENGTH          64
#define NIC_PART_NUMBER_MAX_LENGTH    64
#define NIC_SERIAL_NUMBER_MAX_LENGTH  64

#define NIC_DEFECTIVE_LONG               0x0001
#define NIC_DEFECTIVE_SHORT              0x0002
#define NIC_DEFECTIVE_BAD_CRC            0x0010
#define NIC_DEFECTIVE_BAD_IPV4_CHECKSUM  0x0020
#define NIC_DEFECTIVE_BAD_IPV6_CHECKSUM  0x0040
#define NIC_DEFECTIVE_BAD_TCP_CHECKSUM   0x0080
#define NIC_DEFECTIVE_BAD_UDP_CHECKSUM   0x0100

/**
 * The bitmap uses single bit for each of the 2^12 = 4096 possible VLAN tags.
 * This means its size is 4096/8 = 512 bytes.
 */
#define NIC_VLAN_BITMAP_SIZE  512

#define NIC_DEVICE_PRINT_FMT  "%x"

/**
 * Structure covering the MAC address.
 */
typedef struct nic_address {
	uint8_t address[ETH_ADDR];
} nic_address_t;

/** Device state. */
typedef enum nic_device_state {
	/**
	 * Device present and stopped. Moving device to this state means to discard
	 * all settings and WOL virtues, rebooting the NIC to state as if the
	 * computer just booted (or the NIC was just inserted in case of removable
	 * NIC).
	 */
	NIC_STATE_STOPPED,
	/**
	 * If the NIC is in this state no packets (frames) are transmitted nor
	 * received. However, the settings are not restarted. You can use this state
	 * to temporarily disable transmition/reception or atomically (with respect
	 * to incoming/outcoming packets) change frames acceptance etc.
	 */
	NIC_STATE_DOWN,
	/** Device is normally operating. */
	NIC_STATE_ACTIVE,
	/** Just a constant to limit the state numbers */
	NIC_STATE_MAX,
} nic_device_state_t;

/**
 * Channel operating mode used on the medium.
 */
typedef enum {
	NIC_CM_UNKNOWN,
	NIC_CM_FULL_DUPLEX,
	NIC_CM_HALF_DUPLEX,
	NIC_CM_SIMPLEX
} nic_channel_mode_t;

/**
 * Role for the device (used e.g. for 1000Gb ethernet)
 */
typedef enum {
	NIC_ROLE_UNKNOWN,
	NIC_ROLE_AUTO,
	NIC_ROLE_MASTER,
	NIC_ROLE_SLAVE
} nic_role_t;

/**
 * Current state of the cable in the device
 */
typedef enum {
	NIC_CS_UNKNOWN,
	NIC_CS_PLUGGED,
	NIC_CS_UNPLUGGED
} nic_cable_state_t;

/**
 * Result of the requested operation
 */
typedef enum {
	/** Successfully disabled */
	NIC_RESULT_DISABLED,
	/** Successfully enabled */
	NIC_RESULT_ENABLED,
	/** Not supported at all */
	NIC_RESULT_NOT_SUPPORTED,
	/** Temporarily not available */
	NIC_RESULT_NOT_AVAILABLE,
	/** Result extensions */
	NIC_RESULT_FIRST_EXTENSION
} nic_result_t;

/** Device usage statistics. */
typedef struct nic_device_stats {
	/** Total packets received (accepted). */
	unsigned long receive_packets;
	/** Total packets transmitted. */
	unsigned long send_packets;
	/** Total bytes received (accepted). */
	unsigned long receive_bytes;
	/** Total bytes transmitted. */
	unsigned long send_bytes;
	/** Bad packets received counter. */
	unsigned long receive_errors;
	/** Packet transmition problems counter. */
	unsigned long send_errors;
	/** Number of frames dropped due to insufficient space in RX buffers */
	unsigned long receive_dropped;
	/** Number of frames dropped due to insufficient space in TX buffers */
	unsigned long send_dropped;
	/** Total multicast packets received (accepted). */
	unsigned long receive_multicast;
	/** Total broadcast packets received (accepted). */
	unsigned long receive_broadcast;
	/** The number of collisions due to congestion on the medium. */
	unsigned long collisions;
	/** Unicast packets received but not accepted (filtered) */
	unsigned long receive_filtered_unicast;
	/** Multicast packets received but not accepted (filtered) */
	unsigned long receive_filtered_multicast;
	/** Broadcast packets received but not accepted (filtered) */
	unsigned long receive_filtered_broadcast;

	/* detailed receive_errors */

	/** Received packet length error counter. */
	unsigned long receive_length_errors;
	/** Receiver buffer overflow counter. */
	unsigned long receive_over_errors;
	/** Received packet with crc error counter. */
	unsigned long receive_crc_errors;
	/** Received frame alignment error counter. */
	unsigned long receive_frame_errors;
	/** Receiver fifo overrun counter. */
	unsigned long receive_fifo_errors;
	/** Receiver missed packet counter. */
	unsigned long receive_missed_errors;

	/* detailed send_errors */

	/** Transmitter aborted counter. */
	unsigned long send_aborted_errors;
	/** Transmitter carrier errors counter. */
	unsigned long send_carrier_errors;
	/** Transmitter fifo overrun counter. */
	unsigned long send_fifo_errors;
	/** Transmitter carrier errors counter. */
	unsigned long send_heartbeat_errors;
	/** Transmitter window errors counter. */
	unsigned long send_window_errors;

	/* for cslip etc */

	/** Total compressed packets received. */
	unsigned long receive_compressed;
	/** Total compressed packet transmitted. */
	unsigned long send_compressed;
} nic_device_stats_t;

/** Errors corresponding to those in the nic_device_stats_t */
typedef enum {
	NIC_SEC_BUFFER_FULL,
	NIC_SEC_ABORTED,
	NIC_SEC_CARRIER_LOST,
	NIC_SEC_FIFO_OVERRUN,
	NIC_SEC_HEARTBEAT,
	NIC_SEC_WINDOW_ERROR,
	/* Error encountered during TX but with other type of error */
	NIC_SEC_OTHER
} nic_send_error_cause_t;

/** Errors corresponding to those in the nic_device_stats_t */
typedef enum {
	NIC_REC_BUFFER_FULL,
	NIC_REC_LENGTH,
	NIC_REC_BUFFER_OVERFLOW,
	NIC_REC_CRC,
	NIC_REC_FRAME_ALIGNMENT,
	NIC_REC_FIFO_OVERRUN,
	NIC_REC_MISSED,
	/* Error encountered during RX but with other type of error */
	NIC_REC_OTHER
} nic_receive_error_cause_t;

/**
 * Information about the NIC that never changes - name, vendor, model,
 * capabilites and so on.
 */
typedef struct nic_device_info {
	/* Device identification */
	char vendor_name[NIC_VENDOR_MAX_LENGTH];
	char model_name[NIC_MODEL_MAX_LENGTH];
	char part_number[NIC_PART_NUMBER_MAX_LENGTH];
	char serial_number[NIC_SERIAL_NUMBER_MAX_LENGTH];
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	/* Device capabilities */
	uint16_t ethernet_support[ETH_PHYS_LAYERS];

	/** The mask of all modes which the device can advertise
	 *
	 *  see ETH_AUTONEG_ macros in nic/eth_phys.h of libc
	 */
	uint32_t autoneg_support;
} nic_device_info_t;

/**
 * Type of the ethernet frame
 */
typedef enum nic_frame_type {
	NIC_FRAME_UNICAST,
	NIC_FRAME_MULTICAST,
	NIC_FRAME_BROADCAST
} nic_frame_type_t;

/**
 * Specifies which unicast frames is the NIC receiving.
 */
typedef enum nic_unicast_mode {
	NIC_UNICAST_UNKNOWN,
	/** No unicast frames are received */
	NIC_UNICAST_BLOCKED,
	/** Only the frames with this NIC's MAC as destination are received */
	NIC_UNICAST_DEFAULT,
	/**
	 * Both frames with this NIC's MAC and those specified in the list are
	 * received
	 */
	NIC_UNICAST_LIST,
	/** All unicast frames are received */
	NIC_UNICAST_PROMISC
} nic_unicast_mode_t;

typedef enum nic_multicast_mode {
	NIC_MULTICAST_UNKNOWN,
	/** No multicast frames are received */
	NIC_MULTICAST_BLOCKED,
	/** Frames with multicast addresses specified in this list are received */
	NIC_MULTICAST_LIST,
	/** All multicast frames are received */
	NIC_MULTICAST_PROMISC
} nic_multicast_mode_t;

typedef enum nic_broadcast_mode {
	NIC_BROADCAST_UNKNOWN,
	/** Broadcast frames are dropped */
	NIC_BROADCAST_BLOCKED,
	/** Broadcast frames are received */
	NIC_BROADCAST_ACCEPTED
} nic_broadcast_mode_t;

/**
 * Structure covering the bitmap with VLAN tags.
 */
typedef struct nic_vlan_mask {
	uint8_t bitmap[NIC_VLAN_BITMAP_SIZE];
} nic_vlan_mask_t;

/* WOL virtue identifier */
typedef unsigned int nic_wv_id_t;

/**
 * Structure passed as argument for virtue NIC_WV_MAGIC_PACKET.
 */
typedef struct nic_wv_magic_packet_data {
	uint8_t password[6];
} nic_wv_magic_packet_data_t;

/**
 * Structure passed as argument for virtue NIC_WV_DIRECTED_IPV4
 */
typedef struct nic_wv_ipv4_data {
	uint8_t address[4];
} nic_wv_ipv4_data_t;

/**
 * Structure passed as argument for virtue NIC_WV_DIRECTED_IPV6
 */
typedef struct nic_wv_ipv6_data {
	uint8_t address[16];
} nic_wv_ipv6_data_t;

/**
 * WOL virtue types defining the interpretation of data passed to the virtue.
 * Those tagged with S can have only single virtue active at one moment, those
 * tagged with M can have multiple ones.
 */
typedef enum nic_wv_type {
	/**
	 * Used for deletion of the virtue - in this case the mask, data and length
	 * arguments are ignored.
	 */
	NIC_WV_NONE,
	/** S
	 * Enabled <=> wakeup upon link change
	 */
	NIC_WV_LINK_CHANGE,
	/** S
	 * If this virtue is set up, wakeup can be issued by a magic packet frame.
	 * If the data argument is not NULL, it must contain
	 * nic_wv_magic_packet_data structure with the SecureOn password.
	 */
	NIC_WV_MAGIC_PACKET,
	/** M
	 * If the virtue is set up, wakeup can be issued by a frame targeted to
	 * device with MAC address specified in data. The data must contain
	 * nic_address_t structure.
	 */
	NIC_WV_DESTINATION,
	/** S
	 * Enabled <=> wakeup upon receiving broadcast frame
	 */
	NIC_WV_BROADCAST,
	/** S
	 * Enabled <=> wakeup upon receiving ARP Request
	 */
	NIC_WV_ARP_REQUEST,
	/** M
	 * If enabled, the wakeup is issued upon receiving frame with an IPv4 packet
	 * with IPv4 address specified in data. The data must contain
	 * nic_wv_ipv4_data structure.
	 */
	NIC_WV_DIRECTED_IPV4,
	/** M
	 * If enabled, the wakeup is issued upon receiving frame with an IPv4 packet
	 * with IPv6 address specified in data. The data must contain
	 * nic_wv_ipv6_data structure.
	 */
	NIC_WV_DIRECTED_IPV6,
	/** M
	 * First length/2 bytes in the argument are interpreted as mask, second
	 * length/2 bytes are interpreted as content.
	 * If enabled, the wakeup is issued upon receiving frame where the bytes
	 * with non-zero value in the mask equal to those in the content.
	 */
	NIC_WV_FULL_MATCH,
	/**
	 * Dummy value, do not use.
	 */
	NIC_WV_MAX
} nic_wv_type_t;

/**
 * Specifies the interrupt/polling mode used by the driver and NIC
 */
typedef enum nic_poll_mode {
	/**
	 * NIC issues interrupts upon events.
	 */
	NIC_POLL_IMMEDIATE,
	/**
	 * Some uspace app calls nic_poll_now(...) in order to check the NIC state
	 * - no interrupts are received from the NIC.
	 */
	NIC_POLL_ON_DEMAND,
	/**
	 * The driver itself issues a poll request in a periodic manner. It is
	 * allowed to use hardware timer if the NIC supports it.
	 */
	NIC_POLL_PERIODIC,
	/**
	 * The driver itself issued a poll request in a periodic manner. The driver
	 * must create software timer, internal hardware timer of NIC must not be
	 * used even if the NIC supports it.
	 */
	NIC_POLL_SOFTWARE_PERIODIC
} nic_poll_mode_t;

/**
 * Says if this virtue type is a multi-virtue (there can be multiple virtues of
 * this type at once).
 *
 * @param type
 *
 * @return true or false
 */
static inline int nic_wv_is_multi(nic_wv_type_t type)
{
	switch (type) {
	case NIC_WV_FULL_MATCH:
	case NIC_WV_DESTINATION:
	case NIC_WV_DIRECTED_IPV4:
	case NIC_WV_DIRECTED_IPV6:
		return true;
	default:
		return false;
	}
}

static inline const char *nic_device_state_to_string(nic_device_state_t state)
{
	switch (state) {
	case NIC_STATE_STOPPED:
		return "stopped";
	case NIC_STATE_DOWN:
		return "down";
	case NIC_STATE_ACTIVE:
		return "active";
	default:
		return "undefined";
	}
}

#endif

/** @}
 */
