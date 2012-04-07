/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libnet
 * @{
 */

#include <protocol_map.h>
#include <ethernet_lsap.h>
#include <ethernet_protocols.h>
#include <net_hardware.h>

#include <ipc/services.h>

/** Maps the internetwork layer service to the network interface layer type.
 *
 * @param[in] nil	Network interface layer service.
 * @param[in] il	Internetwork layer service.
 * @return		Network interface layer type of the internetworking
 *			layer service.
 * @return		Zero if mapping is not found.
 */
eth_type_t protocol_map(services_t nil, services_t il)
{
	switch (nil) {
	case SERVICE_ETHERNET:
	case SERVICE_NILDUMMY:
		switch (il) {
		case SERVICE_IP:
			return ETH_P_IP;
		case SERVICE_ARP:
			return ETH_P_ARP;
		default:
			return 0;
		}
	default:
		return 0;
	}
}

/** Maps the network interface layer type to the internetwork layer service.
 *
 * @param[in] nil	Network interface layer service.
 * @param[in] protocol	Network interface layer type.
 * @return		Internetwork layer service of the network interface
 *			layer type.
 * @return		Zero if mapping is not found.
 */
services_t protocol_unmap(services_t nil, int protocol)
{
	switch (nil) {
	case SERVICE_ETHERNET:
	case SERVICE_NILDUMMY:
		switch (protocol) {
		case ETH_P_IP:
			return SERVICE_IP;
		case ETH_P_ARP:
			return SERVICE_ARP;
		default:
			return 0;
		}
	default:
		return 0;
	}
}

/** Maps the link service access point identifier to the Ethernet protocol
 * identifier.
 *
 * @param[in] lsap	Link service access point identifier.
 * @return		Ethernet protocol identifier of the link service access
 *			point identifier.
 * @return		ETH_LSAP_NULL if mapping is not found.
 */
eth_type_t lsap_map(eth_lsap_t lsap)
{
	switch (lsap) {
	case ETH_LSAP_IP:
		return ETH_P_IP;
	case ETH_LSAP_ARP:
		return ETH_P_ARP;
	default:
		return ETH_LSAP_NULL;
	}
}

/** Maps the Ethernet protocol identifier to the link service access point
 * identifier.
 *
 * @param[in] ethertype	Ethernet protocol identifier.
 * @return		Link service access point identifier.
 * @return		Zero if mapping is not found.
 */
eth_lsap_t lsap_unmap(eth_type_t ethertype)
{
	switch (ethertype) {
	case ETH_P_IP:
		return ETH_LSAP_IP;
	case ETH_P_ARP:
		return ETH_LSAP_ARP;
	default:
		return 0;
	}
}

/** Maps the network interface layer services to the hardware types.
 *
 * @param[in] nil	The network interface service.
 * @return		The hardware type of the network interface service.
 * @return		Zero if mapping is not found.
 */
hw_type_t hardware_map(services_t nil)
{
	switch (nil) {
	case SERVICE_ETHERNET:
		return HW_ETHER;
	default:
		return 0;
	}
}

/** @}
 */
