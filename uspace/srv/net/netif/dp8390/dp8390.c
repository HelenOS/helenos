/*
 * Copyright (c) 1987,1997, 2006, Vrije Universiteit, Amsterdam, The Netherlands All rights reserved. Redistribution and use of the MINIX 3 operating system in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * * Neither the name of the Vrije Universiteit nor the names of the software authors or contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * * Any deviations from these conditions require written permission from the copyright holder in advance
 *
 *
 * Disclaimer
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS, AUTHORS, AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes:
 *  2009 ported to HelenOS, Lukas Mejdrech
 */

/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  DP8390 network interface core implementation.
 */

#include <assert.h>
#include <errno.h>

#include "../../include/byteorder.h"

#include "../../structures/packet/packet.h"
#include "../../structures/packet/packet_client.h"

#include "../netif.h"

#include "dp8390_drv.h"
#include "dp8390_port.h"

/*
 * dp8390.c
 *
 * Created:	before Dec 28, 1992 by Philip Homburg <philip@f-mnx.phicoh.com>
 *
 * Modified Mar 10 1994 by Philip Homburg
 *	Become a generic dp8390 driver.
 *
 * Modified Dec 20 1996 by G. Falzoni <falzoni@marina.scn.de>
 *	Added support for 3c503 boards.
 */

#include "local.h"
#include "dp8390.h"

/** Queues the outgoing packet.
 *  @param[in] dep The network interface structure.
 *  @param[in] packet The outgoing packet.
 *  @returns EOK on success.
 *  @returns EINVAL 
 */
int queue_packet(dpeth_t * dep, packet_t packet);

/** Reads a memory block byte by byte.
 *  @param[in] port The source address.
 *  @param[out] buf The destination buffer.
 *  @param[in] size The memory block size in bytes.
 */
static void outsb(port_t port, void * buf, size_t size);

/** Reads a memory block word by word.
 *  @param[in] port The source address.
 *  @param[out] buf The destination buffer.
 *  @param[in] size The memory block size in bytes.
 */
static void outsw(port_t port, void * buf, size_t size);

//static u16_t eth_ign_proto;
//static char *progname;

/* Configuration */
/*typedef struct dp_conf
{
	port_t dpc_port;
	int dpc_irq;
	phys_bytes dpc_mem;
	char *dpc_envvar;
} dp_conf_t;
*/
//dp_conf_t dp_conf[]=	/* Card addresses */
//{
	/* I/O port, IRQ,  Buffer address,  Env. var. */
/*	{ 0x280,     3,    0xD0000,        "DPETH0"	},
	{ 0x300,     5,    0xC8000,        "DPETH1"	},
	{ 0x380,    10,    0xD8000,        "DPETH2"	},
};
*/
/* Test if dp_conf has exactly DE_PORT_NR entries.  If not then you will see
 * the error: "array size is negative".
 */
//extern int ___dummy[DE_PORT_NR == sizeof(dp_conf)/sizeof(dp_conf[0]) ? 1 : -1];

/* Card inits configured out? */
#if !ENABLE_WDETH
#define wdeth_probe(dep)	(0)
#endif
#if !ENABLE_NE2000
#define ne_probe(dep)		(0)
#endif
#if !ENABLE_3C503
#define el2_probe(dep)		(0)
#endif

/* Some clones of the dp8390 and the PC emulator 'Bochs' require the CR_STA
 * on writes to the CR register. Additional CR_STAs do not appear to hurt
 * genuine dp8390s
 */
#define CR_EXTRA	CR_STA

//#if ENABLE_PCI
//_PROTOTYPE(static void pci_conf, (void)				);
//#endif
//_PROTOTYPE(static void do_vwrite, (message *mp, int from_int,
//							int vectored)	);
//_PROTOTYPE(static void do_vwrite_s, (message *mp, int from_int)	);
//_PROTOTYPE(static void do_vread, (message *mp, int vectored)		);
//_PROTOTYPE(static void do_vread_s, (message *mp)			);
//_PROTOTYPE(static void do_init, (message *mp)				);
//_PROTOTYPE(static void do_int, (dpeth_t *dep)				);
//_PROTOTYPE(static void do_getstat, (message *mp)			);
//_PROTOTYPE(static void do_getstat_s, (message *mp)			);
//_PROTOTYPE(static void do_getname, (message *mp)			);
//_PROTOTYPE(static void do_stop, (message *mp)				);
_PROTOTYPE(static void dp_init, (dpeth_t *dep)				);
//_PROTOTYPE(static void dp_confaddr, (dpeth_t *dep)			);
_PROTOTYPE(static void dp_reinit, (dpeth_t *dep)			);
_PROTOTYPE(static void dp_reset, (dpeth_t *dep)			);
//_PROTOTYPE(static void dp_check_ints, (dpeth_t *dep)			);
_PROTOTYPE(static void dp_recv, (dpeth_t *dep)				);
_PROTOTYPE(static void dp_send, (dpeth_t *dep)				);
//_PROTOTYPE(static void dp8390_stop, (void)				);
_PROTOTYPE(static void dp_getblock, (dpeth_t *dep, int page,
				size_t offset, size_t size, void *dst)	);
_PROTOTYPE(static void dp_pio8_getblock, (dpeth_t *dep, int page,
				size_t offset, size_t size, void *dst)	);
_PROTOTYPE(static void dp_pio16_getblock, (dpeth_t *dep, int page,
				size_t offset, size_t size, void *dst)	);
_PROTOTYPE(static int dp_pkt2user, (dpeth_t *dep, int page,
							int length) );
//_PROTOTYPE(static int dp_pkt2user_s, (dpeth_t *dep, int page,
//							int length)	);
_PROTOTYPE(static void dp_user2nic, (dpeth_t *dep, iovec_dat_t *iovp, 
		vir_bytes offset, int nic_addr, vir_bytes count) );
//_PROTOTYPE(static void dp_user2nic_s, (dpeth_t *dep, iovec_dat_s_t *iovp, 
//		vir_bytes offset, int nic_addr, vir_bytes count)	);
_PROTOTYPE(static void dp_pio8_user2nic, (dpeth_t *dep,
				iovec_dat_t *iovp, vir_bytes offset,
				int nic_addr, vir_bytes count) );
//_PROTOTYPE(static void dp_pio8_user2nic_s, (dpeth_t *dep,
//				iovec_dat_s_t *iovp, vir_bytes offset,
//				int nic_addr, vir_bytes count)		);
_PROTOTYPE(static void dp_pio16_user2nic, (dpeth_t *dep,
				iovec_dat_t *iovp, vir_bytes offset,
				int nic_addr, vir_bytes count) );
//_PROTOTYPE(static void dp_pio16_user2nic_s, (dpeth_t *dep,
//				iovec_dat_s_t *iovp, vir_bytes offset,
//				int nic_addr, vir_bytes count)		);
_PROTOTYPE(static void dp_nic2user, (dpeth_t *dep, int nic_addr, 
		iovec_dat_t *iovp, vir_bytes offset, vir_bytes count)	);
//_PROTOTYPE(static void dp_nic2user_s, (dpeth_t *dep, int nic_addr, 
//		iovec_dat_s_t *iovp, vir_bytes offset, vir_bytes count)	);
_PROTOTYPE(static void dp_pio8_nic2user, (dpeth_t *dep, int nic_addr, 
		iovec_dat_t *iovp, vir_bytes offset, vir_bytes count)	);
//_PROTOTYPE(static void dp_pio8_nic2user_s, (dpeth_t *dep, int nic_addr, 
//		iovec_dat_s_t *iovp, vir_bytes offset, vir_bytes count)	);
_PROTOTYPE(static void dp_pio16_nic2user, (dpeth_t *dep, int nic_addr, 
		iovec_dat_t *iovp, vir_bytes offset, vir_bytes count)	);
//_PROTOTYPE(static void dp_pio16_nic2user_s, (dpeth_t *dep, int nic_addr, 
//		iovec_dat_s_t *iovp, vir_bytes offset, vir_bytes count)	);
_PROTOTYPE(static void dp_next_iovec, (iovec_dat_t *iovp)		);
//_PROTOTYPE(static void dp_next_iovec_s, (iovec_dat_s_t *iovp)		);
_PROTOTYPE(static void conf_hw, (dpeth_t *dep)				);
//_PROTOTYPE(static void update_conf, (dpeth_t *dep, dp_conf_t *dcp)	);
_PROTOTYPE(static void map_hw_buffer, (dpeth_t *dep)			);
//_PROTOTYPE(static int calc_iovec_size, (iovec_dat_t *iovp)		);
//_PROTOTYPE(static int calc_iovec_size_s, (iovec_dat_s_t *iovp)		);
_PROTOTYPE(static void reply, (dpeth_t *dep, int err, int may_block)	);
//_PROTOTYPE(static void mess_reply, (message *req, message *reply)	);
_PROTOTYPE(static void get_userdata, (int user_proc,
		vir_bytes user_addr, vir_bytes count, void *loc_addr)	);
//_PROTOTYPE(static void get_userdata_s, (int user_proc,
//		cp_grant_id_t grant, vir_bytes offset, vir_bytes count,
//		void *loc_addr)	);
//_PROTOTYPE(static void put_userdata, (int user_proc,
//		vir_bytes user_addr, vir_bytes count, void *loc_addr)	);
//_PROTOTYPE(static void put_userdata_s, (int user_proc,
//		cp_grant_id_t grant, size_t count, void *loc_addr)	);
_PROTOTYPE(static void insb, (port_t port, void *buf, size_t size)				);
_PROTOTYPE(static void insw, (port_t port, void *buf, size_t size)				);
//_PROTOTYPE(static void do_vir_insb, (port_t port, int proc,
//					vir_bytes buf, size_t size)	);
//_PROTOTYPE(static void do_vir_insw, (port_t port, int proc,
//					vir_bytes buf, size_t size)	);
//_PROTOTYPE(static void do_vir_outsb, (port_t port, int proc,
//					vir_bytes buf, size_t size)	);
//_PROTOTYPE(static void do_vir_outsw, (port_t port, int proc,
//					vir_bytes buf, size_t size)	);

int do_probe(dpeth_t * dep){
	/* This is the default, try to (re)locate the device. */
	conf_hw(dep);
	if (dep->de_mode == DEM_DISABLED)
	{
		/* Probe failed, or the device is configured off. */
		return EXDEV;//ENXIO;
	}
	if (dep->de_mode == DEM_ENABLED)
		dp_init(dep);
	return EOK;
}

/*===========================================================================*
 *				dp8390_dump				     *
 *===========================================================================*/
void dp8390_dump(dpeth_t * dep)
{
//	dpeth_t *dep;
	int /*i,*/ isr;

//	printf("\n");
//	for (i= 0, dep = &de_table[0]; i<DE_PORT_NR; i++, dep++)
//	{
#if XXX
		if (dep->de_mode == DEM_DISABLED)
			printf("dp8390 port %d is disabled\n", i);
		else if (dep->de_mode == DEM_SINK)
			printf("dp8390 port %d is in sink mode\n", i);
#endif

		if (dep->de_mode != DEM_ENABLED)
//			continue;
			return;

//		printf("dp8390 statistics of port %d:\n", i);

		printf("recvErr    :%8ld\t", dep->de_stat.ets_recvErr);
		printf("sendErr    :%8ld\t", dep->de_stat.ets_sendErr);
		printf("OVW        :%8ld\n", dep->de_stat.ets_OVW);

		printf("CRCerr     :%8ld\t", dep->de_stat.ets_CRCerr);
		printf("frameAll   :%8ld\t", dep->de_stat.ets_frameAll);
		printf("missedP    :%8ld\n", dep->de_stat.ets_missedP);

		printf("packetR    :%8ld\t", dep->de_stat.ets_packetR);
		printf("packetT    :%8ld\t", dep->de_stat.ets_packetT);
		printf("transDef   :%8ld\n", dep->de_stat.ets_transDef);

		printf("collision  :%8ld\t", dep->de_stat.ets_collision);
		printf("transAb    :%8ld\t", dep->de_stat.ets_transAb);
		printf("carrSense  :%8ld\n", dep->de_stat.ets_carrSense);

		printf("fifoUnder  :%8ld\t", dep->de_stat.ets_fifoUnder);
		printf("fifoOver   :%8ld\t", dep->de_stat.ets_fifoOver);
		printf("CDheartbeat:%8ld\n", dep->de_stat.ets_CDheartbeat);

		printf("OWC        :%8ld\t", dep->de_stat.ets_OWC);

		isr= inb_reg0(dep, DP_ISR);
		printf("dp_isr = 0x%x + 0x%x, de_flags = 0x%x\n", isr,
					inb_reg0(dep, DP_ISR), dep->de_flags);
//	}
}

/*===========================================================================*
 *				do_init					     *
 *===========================================================================*/
int do_init(dpeth_t * dep, int mode){
	if (dep->de_mode == DEM_DISABLED)
	{
		// might call do_probe()
		return EXDEV;
	}

	if (dep->de_mode == DEM_SINK)
	{
//		strncpy((char *) dep->de_address.ea_addr, "ZDP", 6);
//		dep->de_address.ea_addr[5] = port;
//		dp_confaddr(dep);
//		reply_mess.m_type = DL_CONF_REPLY;
//		reply_mess.m3_i1 = mp->DL_PORT;
//		reply_mess.m3_i2 = DE_PORT_NR;
//		*(ether_addr_t *) reply_mess.m3_ca1 = dep->de_address;
//		mess_reply(mp, &reply_mess);
//		return;
		return EOK;
	}
	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags &DEF_ENABLED);

	dep->de_flags &= ~(DEF_PROMISC | DEF_MULTI | DEF_BROAD);

	if (mode &DL_PROMISC_REQ)
		dep->de_flags |= DEF_PROMISC | DEF_MULTI | DEF_BROAD;
	if (mode &DL_MULTI_REQ)
		dep->de_flags |= DEF_MULTI;
	if (mode &DL_BROAD_REQ)
		dep->de_flags |= DEF_BROAD;

//	dep->de_client = mp->m_source;
	dp_reinit(dep);

//	reply_mess.m_type = DL_CONF_REPLY;
//	reply_mess.m3_i1 = mp->DL_PORT;
//	reply_mess.m3_i2 = DE_PORT_NR;
//	*(ether_addr_t *) reply_mess.m3_ca1 = dep->de_address;

//	mess_reply(mp, &reply_mess);
	return EOK;
}

/*===========================================================================*
 *				do_stop					     *
 *===========================================================================*/
void do_stop(dpeth_t * dep){
	if((dep->de_mode != DEM_SINK) && (dep->de_mode == DEM_ENABLED) && (dep->de_flags &DEF_ENABLED)){
		outb_reg0(dep, DP_CR, CR_STP | CR_DM_ABORT);
		(dep->de_stopf)(dep);

		dep->de_flags = DEF_EMPTY;
	}
}

int queue_packet(dpeth_t * dep, packet_t packet){
	packet_t tmp;

	if(dep->packet_count >= MAX_PACKETS){
		netif_pq_release(packet_get_id(packet));
		return ELIMIT;
	}

	tmp = dep->packet_queue;
	while(pq_next(tmp)){
		tmp = pq_next(tmp);
	}
	if(pq_add(&tmp, packet, 0, 0) != EOK){
		return EINVAL;
	}
	if(! dep->packet_count){
		dep->packet_queue = packet;
	}
	++ dep->packet_count;
	return EBUSY;
}

/*===========================================================================*
 *			based on	do_vwrite				     *
 *===========================================================================*/
int do_pwrite(dpeth_t * dep, packet_t packet, int from_int)
{
//	int port, count, size;
	int size;
	int sendq_head;
/*	dpeth_t *dep;

	port = mp->DL_PORT;
	count = mp->DL_COUNT;
	if (port < 0 || port >= DE_PORT_NR)
		panic("", "dp8390: illegal port", port);
	dep= &de_table[port];
	dep->de_client= mp->DL_PROC;
*/
	if (dep->de_mode == DEM_SINK)
	{
		assert(!from_int);
//		dep->de_flags |= DEF_PACK_SEND;
		reply(dep, OK, FALSE);
//		return;
		return EOK;
	}
	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags &DEF_ENABLED);
	if(dep->packet_queue && (! from_int)){
//	if (dep->de_flags &DEF_SEND_AVAIL){
//		panic("", "dp8390: send already in progress", NO_NUM);
		return queue_packet(dep, packet);
	}

	sendq_head= dep->de_sendq_head;
//	if (dep->de_sendq[sendq_head].sq_filled)
//	{
//		if (from_int)
//			panic("", "dp8390: should not be sending\n", NO_NUM);
//		dep->de_sendmsg= *mp;
//		dep->de_flags |= DEF_SEND_AVAIL;
//		reply(dep, OK, FALSE);
//		return;
//		return queue_packet(dep, packet);
//	}
//	assert(!(dep->de_flags &DEF_PACK_SEND));

/*	if (vectored)
	{
		get_userdata(mp->DL_PROC, (vir_bytes) mp->DL_ADDR,
			(count > IOVEC_NR ? IOVEC_NR : count) *
			sizeof(iovec_t), dep->de_write_iovec.iod_iovec);
		dep->de_write_iovec.iod_iovec_s = count;
		dep->de_write_iovec.iod_proc_nr = mp->DL_PROC;
		dep->de_write_iovec.iod_iovec_addr = (vir_bytes) mp->DL_ADDR;

		dep->de_tmp_iovec = dep->de_write_iovec;
		size = calc_iovec_size(&dep->de_tmp_iovec);
	}
	else
	{ 
		dep->de_write_iovec.iod_iovec[0].iov_addr =
			(vir_bytes) mp->DL_ADDR;
		dep->de_write_iovec.iod_iovec[0].iov_size =
			mp->DL_COUNT;
		dep->de_write_iovec.iod_iovec_s = 1;
		dep->de_write_iovec.iod_proc_nr = mp->DL_PROC;
		dep->de_write_iovec.iod_iovec_addr = 0;
		size= mp->DL_COUNT;
	}
*/
	size = packet_get_data_length(packet);
	dep->de_write_iovec.iod_iovec[0].iov_addr = (vir_bytes) packet_get_data(packet);
	dep->de_write_iovec.iod_iovec[0].iov_size = size;
	dep->de_write_iovec.iod_iovec_s = 1;
	dep->de_write_iovec.iod_iovec_addr = NULL;

	if (size < ETH_MIN_PACK_SIZE || size > ETH_MAX_PACK_SIZE_TAGGED)
	{
		panic("", "dp8390: invalid packet size", size);
		return EINVAL;
	}
	(dep->de_user2nicf)(dep, &dep->de_write_iovec, 0,
		dep->de_sendq[sendq_head].sq_sendpage * DP_PAGESIZE,
		size);
	dep->de_sendq[sendq_head].sq_filled= TRUE;
	if (dep->de_sendq_tail == sendq_head)
	{
		outb_reg0(dep, DP_TPSR, dep->de_sendq[sendq_head].sq_sendpage);
		outb_reg0(dep, DP_TBCR1, size >> 8);
		outb_reg0(dep, DP_TBCR0, size &0xff);
		outb_reg0(dep, DP_CR, CR_TXP | CR_EXTRA);/* there it goes.. */
	}
	else
		dep->de_sendq[sendq_head].sq_size= size;
	
	if (++sendq_head == dep->de_sendq_nr)
		sendq_head= 0;
	assert(sendq_head < SENDQ_NR);
	dep->de_sendq_head= sendq_head;

//	dep->de_flags |= DEF_PACK_SEND;

	/* If the interrupt handler called, don't send a reply. The reply
	 * will be sent after all interrupts are handled. 
	 */
	if (from_int)
		return EOK;
	reply(dep, OK, FALSE);

	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags &DEF_ENABLED);
	return EOK;
}

/*===========================================================================*
 *				dp_init					     *
 *===========================================================================*/
void dp_init(dep)
dpeth_t *dep;
{
	int dp_rcr_reg;
	int i;//, r;

	/* General initialization */
	dep->de_flags = DEF_EMPTY;
	(*dep->de_initf)(dep);

//	dp_confaddr(dep);

	if (debug)
	{
		printf("%s: Ethernet address ", dep->de_name);
		for (i= 0; i < 6; i++)
			printf("%x%c", dep->de_address.ea_addr[i],
							i < 5 ? ':' : '\n');
	}

	/* Map buffer */
	map_hw_buffer(dep);

	/* Initialization of the dp8390 following the mandatory procedure
	 * in reference manual ("DP8390D/NS32490D NIC Network Interface
	 * Controller", National Semiconductor, July 1995, Page 29).
	 */
	/* Step 1: */
	outb_reg0(dep, DP_CR, CR_PS_P0 | CR_STP | CR_DM_ABORT);
	/* Step 2: */
	if (dep->de_16bit)
		outb_reg0(dep, DP_DCR, DCR_WORDWIDE | DCR_8BYTES | DCR_BMS);
	else
		outb_reg0(dep, DP_DCR, DCR_BYTEWIDE | DCR_8BYTES | DCR_BMS);
	/* Step 3: */
	outb_reg0(dep, DP_RBCR0, 0);
	outb_reg0(dep, DP_RBCR1, 0);
	/* Step 4: */
	dp_rcr_reg = 0;
	if (dep->de_flags &DEF_PROMISC)
		dp_rcr_reg |= RCR_AB | RCR_PRO | RCR_AM;
	if (dep->de_flags &DEF_BROAD)
		dp_rcr_reg |= RCR_AB;
	if (dep->de_flags &DEF_MULTI)
		dp_rcr_reg |= RCR_AM;
	outb_reg0(dep, DP_RCR, dp_rcr_reg);
	/* Step 5: */
	outb_reg0(dep, DP_TCR, TCR_INTERNAL);
	/* Step 6: */
	outb_reg0(dep, DP_BNRY, dep->de_startpage);
	outb_reg0(dep, DP_PSTART, dep->de_startpage);
	outb_reg0(dep, DP_PSTOP, dep->de_stoppage);
	/* Step 7: */
	outb_reg0(dep, DP_ISR, 0xFF);
	/* Step 8: */
	outb_reg0(dep, DP_IMR, IMR_PRXE | IMR_PTXE | IMR_RXEE | IMR_TXEE |
		IMR_OVWE | IMR_CNTE);
	/* Step 9: */
	outb_reg0(dep, DP_CR, CR_PS_P1 | CR_DM_ABORT | CR_STP);

	outb_reg1(dep, DP_PAR0, dep->de_address.ea_addr[0]);
	outb_reg1(dep, DP_PAR1, dep->de_address.ea_addr[1]);
	outb_reg1(dep, DP_PAR2, dep->de_address.ea_addr[2]);
	outb_reg1(dep, DP_PAR3, dep->de_address.ea_addr[3]);
	outb_reg1(dep, DP_PAR4, dep->de_address.ea_addr[4]);
	outb_reg1(dep, DP_PAR5, dep->de_address.ea_addr[5]);

	outb_reg1(dep, DP_MAR0, 0xff);
	outb_reg1(dep, DP_MAR1, 0xff);
	outb_reg1(dep, DP_MAR2, 0xff);
	outb_reg1(dep, DP_MAR3, 0xff);
	outb_reg1(dep, DP_MAR4, 0xff);
	outb_reg1(dep, DP_MAR5, 0xff);
	outb_reg1(dep, DP_MAR6, 0xff);
	outb_reg1(dep, DP_MAR7, 0xff);

	outb_reg1(dep, DP_CURR, dep->de_startpage + 1);
	/* Step 10: */
	outb_reg0(dep, DP_CR, CR_DM_ABORT | CR_STA);
	/* Step 11: */
	outb_reg0(dep, DP_TCR, TCR_NORMAL);

	inb_reg0(dep, DP_CNTR0);		/* reset counters by reading */
	inb_reg0(dep, DP_CNTR1);
	inb_reg0(dep, DP_CNTR2);

	/* Finish the initialization. */
	dep->de_flags |= DEF_ENABLED;
	for (i= 0; i<dep->de_sendq_nr; i++)
		dep->de_sendq[i].sq_filled= 0;
	dep->de_sendq_head= 0;
	dep->de_sendq_tail= 0;
	if (!dep->de_prog_IO)
	{
		dep->de_user2nicf= dp_user2nic;
//		dep->de_user2nicf_s= dp_user2nic_s;
		dep->de_nic2userf= dp_nic2user;
//		dep->de_nic2userf_s= dp_nic2user_s;
		dep->de_getblockf= dp_getblock;
	}
	else if (dep->de_16bit)
	{
		dep->de_user2nicf= dp_pio16_user2nic;
//		dep->de_user2nicf_s= dp_pio16_user2nic_s;
		dep->de_nic2userf= dp_pio16_nic2user;
//		dep->de_nic2userf_s= dp_pio16_nic2user_s;
		dep->de_getblockf= dp_pio16_getblock;
	}
	else
	{
		dep->de_user2nicf= dp_pio8_user2nic;
//		dep->de_user2nicf_s= dp_pio8_user2nic_s;
		dep->de_nic2userf= dp_pio8_nic2user;
//		dep->de_nic2userf_s= dp_pio8_nic2user_s;
		dep->de_getblockf= dp_pio8_getblock;
	}

	/* Set the interrupt handler and policy. Do not automatically 
	 * reenable interrupts. Return the IRQ line number on interrupts.
 	 */
/* 	dep->de_hook = dep->de_irq;
	r= sys_irqsetpolicy(dep->de_irq, 0, &dep->de_hook);
	if (r != OK)
		panic("DP8390", "sys_irqsetpolicy failed", r);

	r= sys_irqenable(&dep->de_hook);
	if (r != OK)
	{
		panic("DP8390", "unable enable interrupts", r);
	}
*/
}

/*===========================================================================*
 *				dp_reinit				     *
 *===========================================================================*/
static void dp_reinit(dep)
dpeth_t *dep;
{
	int dp_rcr_reg;

	outb_reg0(dep, DP_CR, CR_PS_P0 | CR_EXTRA);

	dp_rcr_reg = 0;
	if (dep->de_flags &DEF_PROMISC)
		dp_rcr_reg |= RCR_AB | RCR_PRO | RCR_AM;
	if (dep->de_flags &DEF_BROAD)
		dp_rcr_reg |= RCR_AB;
	if (dep->de_flags &DEF_MULTI)
		dp_rcr_reg |= RCR_AM;
	outb_reg0(dep, DP_RCR, dp_rcr_reg);
}

/*===========================================================================*
 *				dp_reset				     *
 *===========================================================================*/
static void dp_reset(dep)
dpeth_t *dep;
{
	int i;

	/* Stop chip */
	outb_reg0(dep, DP_CR, CR_STP | CR_DM_ABORT);
	outb_reg0(dep, DP_RBCR0, 0);
	outb_reg0(dep, DP_RBCR1, 0);
	for (i= 0; i < 0x1000 && ((inb_reg0(dep, DP_ISR) &ISR_RST) == 0); i++)
		; /* Do nothing */
	outb_reg0(dep, DP_TCR, TCR_1EXTERNAL|TCR_OFST);
	outb_reg0(dep, DP_CR, CR_STA|CR_DM_ABORT);
	outb_reg0(dep, DP_TCR, TCR_NORMAL);

	/* Acknowledge the ISR_RDC (remote dma) interrupt. */
	for (i= 0; i < 0x1000 && ((inb_reg0(dep, DP_ISR) &ISR_RDC) == 0); i++)
		; /* Do nothing */
	outb_reg0(dep, DP_ISR, inb_reg0(dep, DP_ISR) &~ISR_RDC);

	/* Reset the transmit ring. If we were transmitting a packet, we
	 * pretend that the packet is processed. Higher layers will
	 * retransmit if the packet wasn't actually sent.
	 */
	dep->de_sendq_head= dep->de_sendq_tail= 0;
	for (i= 0; i<dep->de_sendq_nr; i++)
		dep->de_sendq[i].sq_filled= 0;
	dp_send(dep);
	dep->de_flags &= ~DEF_STOPPED;
}

/*===========================================================================*
 *				dp_check_ints				     *
 *===========================================================================*/
void dp_check_ints(dep, isr)
dpeth_t *dep;
int isr;
{
	int /*isr,*/ tsr;
	int size, sendq_tail;

	if (!(dep->de_flags &DEF_ENABLED))
		panic("", "dp8390: got premature interrupt", NO_NUM);

	for(;;)
	{
//		isr = inb_reg0(dep, DP_ISR);
		if (!isr)
			break;
		outb_reg0(dep, DP_ISR, isr);
		if (isr &(ISR_PTX|ISR_TXE))
		{
			if (isr &ISR_TXE)
			{
#if DEBUG
 {printf("%s: got send Error\n", dep->de_name);}
#endif
				dep->de_stat.ets_sendErr++;
			}
			else
			{
				tsr = inb_reg0(dep, DP_TSR);

				if (tsr &TSR_PTX) dep->de_stat.ets_packetT++;
#if 0	/* Reserved in later manuals, should be ignored */
				if (!(tsr &TSR_DFR))
				{
					/* In most (all?) implementations of
					 * the dp8390, this bit is set
					 * when the packet is not deferred
					 */
					dep->de_stat.ets_transDef++;
				}
#endif
				if (tsr &TSR_COL) dep->de_stat.ets_collision++;
				if (tsr &TSR_ABT) dep->de_stat.ets_transAb++;
				if (tsr &TSR_CRS) dep->de_stat.ets_carrSense++;
				if (tsr &TSR_FU
					&& ++dep->de_stat.ets_fifoUnder <= 10)
				{
					printf("%s: fifo underrun\n",
						dep->de_name);
				}
				if (tsr &TSR_CDH
					&& ++dep->de_stat.ets_CDheartbeat <= 10)
				{
					printf("%s: CD heart beat failure\n",
						dep->de_name);
				}
				if (tsr &TSR_OWC) dep->de_stat.ets_OWC++;
			}
			sendq_tail= dep->de_sendq_tail;

			if (!(dep->de_sendq[sendq_tail].sq_filled))
			{
				/* Software bug? */
				assert(!debug);

				/* Or hardware bug? */
				printf(
				"%s: transmit interrupt, but not sending\n",
					dep->de_name);
				continue;
			}
			dep->de_sendq[sendq_tail].sq_filled= 0;
			if (++sendq_tail == dep->de_sendq_nr)
				sendq_tail= 0;
			dep->de_sendq_tail= sendq_tail;
			if (dep->de_sendq[sendq_tail].sq_filled)
			{
				size= dep->de_sendq[sendq_tail].sq_size;
				outb_reg0(dep, DP_TPSR,
					dep->de_sendq[sendq_tail].sq_sendpage);
				outb_reg0(dep, DP_TBCR1, size >> 8);
				outb_reg0(dep, DP_TBCR0, size &0xff);
				outb_reg0(dep, DP_CR, CR_TXP | CR_EXTRA);
			}
//			if (dep->de_flags &DEF_SEND_AVAIL)
				dp_send(dep);
		}

		if (isr &ISR_PRX)
		{
			/* Only call dp_recv if there is a read request */
//			if (dep->de_flags) &DEF_READING)
				dp_recv(dep);
		}
		
		if (isr &ISR_RXE) dep->de_stat.ets_recvErr++;
		if (isr &ISR_CNT)
		{
			dep->de_stat.ets_CRCerr += inb_reg0(dep, DP_CNTR0);
			dep->de_stat.ets_frameAll += inb_reg0(dep, DP_CNTR1);
			dep->de_stat.ets_missedP += inb_reg0(dep, DP_CNTR2);
		}
		if (isr &ISR_OVW)
		{
			dep->de_stat.ets_OVW++;
#if 0
			{printW(); printf(
				"%s: got overwrite warning\n", dep->de_name);}
#endif
/*			if (dep->de_flags &DEF_READING)
			{
				printf(
"dp_check_ints: strange: overwrite warning and pending read request\n");
				dp_recv(dep);
			}
*/		}
		if (isr &ISR_RDC)
		{
			/* Nothing to do */
		}
		if (isr &ISR_RST)
		{
			/* this means we got an interrupt but the ethernet 
			 * chip is shutdown. We set the flag DEF_STOPPED,
			 * and continue processing arrived packets. When the
			 * receive buffer is empty, we reset the dp8390.
			 */
#if 0
			 {printW(); printf(
				"%s: NIC stopped\n", dep->de_name);}
#endif
			dep->de_flags |= DEF_STOPPED;
			break;
		}
		isr = inb_reg0(dep, DP_ISR);
	}
//	if ((dep->de_flags &(DEF_READING|DEF_STOPPED)) == 
//						(DEF_READING|DEF_STOPPED))
	if ((dep->de_flags &DEF_STOPPED) == DEF_STOPPED)
	{
		/* The chip is stopped, and all arrived packets are 
		 * delivered.
		 */
		dp_reset(dep);
	}
}

/*===========================================================================*
 *				dp_recv					     *
 *===========================================================================*/
static void dp_recv(dep)
dpeth_t *dep;
{
	dp_rcvhdr_t header;
	//unsigned pageno, curr, next;
	int pageno, curr, next;
	vir_bytes length;
	int packet_processed, r;
	u16_t eth_type;

	packet_processed = FALSE;
	pageno = inb_reg0(dep, DP_BNRY) + 1;
	if (pageno == dep->de_stoppage) pageno = dep->de_startpage;

	do
	{
		outb_reg0(dep, DP_CR, CR_PS_P1 | CR_EXTRA);
		curr = inb_reg1(dep, DP_CURR);
		outb_reg0(dep, DP_CR, CR_PS_P0 | CR_EXTRA);

		if (curr == pageno) break;

		(dep->de_getblockf)(dep, pageno, (size_t)0, sizeof(header),
			&header);
		(dep->de_getblockf)(dep, pageno, sizeof(header) +
			2*sizeof(ether_addr_t), sizeof(eth_type), &eth_type);

		length = (header.dr_rbcl | (header.dr_rbch << 8)) -
			sizeof(dp_rcvhdr_t);
		next = header.dr_next;
		if (length < ETH_MIN_PACK_SIZE ||
			length > ETH_MAX_PACK_SIZE_TAGGED)
		{
			printf("%s: packet with strange length arrived: %d\n",
				dep->de_name, (int) length);
			next= curr;
		}
		else if (next < dep->de_startpage || next >= dep->de_stoppage)
		{
			printf("%s: strange next page\n", dep->de_name);
			next= curr;
		}
/*		else if (eth_type == eth_ign_proto)
		{
*/			/* Hack: ignore packets of a given protocol, useful
			 * if you share a net with 80 computers sending
			 * Amoeba FLIP broadcasts.  (Protocol 0x8146.)
			 */
/*			static int first= 1;
			if (first)
			{
				first= 0;
				printf("%s: dropping proto 0x%04x packets\n",
					dep->de_name,
					ntohs(eth_ign_proto));
			}
			dep->de_stat.ets_packetR++;
		}
*/		else if (header.dr_status &RSR_FO)
		{
			/* This is very serious, so we issue a warning and
			 * reset the buffers */
			printf("%s: fifo overrun, resetting receive buffer\n",
				dep->de_name);
			dep->de_stat.ets_fifoOver++;
			next = curr;
		}
		else if ((header.dr_status &RSR_PRX) && 
					   (dep->de_flags &DEF_ENABLED))
		{
//			if (dep->de_safecopy_read)
//				r = dp_pkt2user_s(dep, pageno, length);
//			else
				r = dp_pkt2user(dep, pageno, length);
			if (r != OK)
				return;

			packet_processed = TRUE;
			dep->de_stat.ets_packetR++;
		}
		if (next == dep->de_startpage)
			outb_reg0(dep, DP_BNRY, dep->de_stoppage - 1);
		else
			outb_reg0(dep, DP_BNRY, next - 1);

		pageno = next;
	}
	while (!packet_processed);
}

/*===========================================================================*
 *				dp_send					     *
 *===========================================================================*/
static void dp_send(dep)
dpeth_t *dep;
{
	packet_t packet;

//	if (!(dep->de_flags &DEF_SEND_AVAIL))
//		return;

	if(dep->packet_queue){
		packet = dep->packet_queue;
		dep->packet_queue = pq_detach(packet);
		do_pwrite(dep, packet, TRUE);
		netif_pq_release(packet_get_id(packet));
		-- dep->packet_count;
	}
//	if(! dep->packet_queue){
//		dep->de_flags &= ~DEF_SEND_AVAIL;
//	}
/*	switch(dep->de_sendmsg.m_type)
	{
	case DL_WRITE:	do_vwrite(&dep->de_sendmsg, TRUE, FALSE);	break;
	case DL_WRITEV:	do_vwrite(&dep->de_sendmsg, TRUE, TRUE);	break;
	case DL_WRITEV_S: do_vwrite_s(&dep->de_sendmsg, TRUE);	break;
	default:
		panic("", "dp8390: wrong type", dep->de_sendmsg.m_type);
		break;
	}
*/
}

/*===========================================================================*
 *				dp_getblock				     *
 *===========================================================================*/
static void dp_getblock(dep, page, offset, size, dst)
dpeth_t *dep;
int page;
size_t offset;
size_t size;
void *dst;
{
//	int r;

	offset = page * DP_PAGESIZE + offset;

	memcpy(dst, dep->de_locmem + offset, size);
}

/*===========================================================================*
 *				dp_pio8_getblock			     *
 *===========================================================================*/
static void dp_pio8_getblock(dep, page, offset, size, dst)
dpeth_t *dep;
int page;
size_t offset;
size_t size;
void *dst;
{
	offset = page * DP_PAGESIZE + offset;
	outb_reg0(dep, DP_RBCR0, size &0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, offset &0xFF);
	outb_reg0(dep, DP_RSAR1, offset >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);

	insb(dep->de_data_port, dst, size);
}

/*===========================================================================*
 *				dp_pio16_getblock			     *
 *===========================================================================*/
static void dp_pio16_getblock(dep, page, offset, size, dst)
dpeth_t *dep;
int page;
size_t offset;
size_t size;
void *dst;
{
	offset = page * DP_PAGESIZE + offset;
	outb_reg0(dep, DP_RBCR0, size &0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, offset &0xFF);
	outb_reg0(dep, DP_RSAR1, offset >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);

	assert (!(size &1));
	insw(dep->de_data_port, dst, size);
}

/*===========================================================================*
 *				dp_pkt2user				     *
 *===========================================================================*/
static int dp_pkt2user(dep, page, length)
dpeth_t *dep;
int page, length;
{
	int last, count;
	packet_t packet;

//	if (!(dep->de_flags &DEF_READING))
//		return EGENERIC;

	packet = netif_packet_get_1(length);
	if(! packet){
		return ENOMEM;
	}
	dep->de_read_iovec.iod_iovec[0].iov_addr = (vir_bytes) packet_suffix(packet, length);
	dep->de_read_iovec.iod_iovec[0].iov_size = length;
	dep->de_read_iovec.iod_iovec_s = 1;
	dep->de_read_iovec.iod_iovec_addr = NULL;

	last = page + (length - 1) / DP_PAGESIZE;
	if (last >= dep->de_stoppage)
	{
		count = (dep->de_stoppage - page) * DP_PAGESIZE -
			sizeof(dp_rcvhdr_t);

		/* Save read_iovec since we need it twice. */
		dep->de_tmp_iovec = dep->de_read_iovec;
		(dep->de_nic2userf)(dep, page * DP_PAGESIZE +
			sizeof(dp_rcvhdr_t), &dep->de_tmp_iovec, 0, count);
		(dep->de_nic2userf)(dep, dep->de_startpage * DP_PAGESIZE, 
			&dep->de_read_iovec, count, length - count);
	}
	else
	{
		(dep->de_nic2userf)(dep, page * DP_PAGESIZE +
			sizeof(dp_rcvhdr_t), &dep->de_read_iovec, 0, length);
	}

	dep->de_read_s = length;
	dep->de_flags |= DEF_PACK_RECV;
//	dep->de_flags &= ~DEF_READING;

	if(dep->received_count >= MAX_PACKETS){
		netif_pq_release(packet_get_id(packet));
		return ELIMIT;
	}else{
		if(pq_add(&dep->received_queue, packet, 0, 0) == EOK){
			++ dep->received_count;
		}else{
			netif_pq_release(packet_get_id(packet));
		}
	}
	return OK;
}

/*===========================================================================*
 *				dp_user2nic				     *
 *===========================================================================*/
static void dp_user2nic(dep, iovp, offset, nic_addr, count)
dpeth_t *dep;
iovec_dat_t *iovp;
vir_bytes offset;
int nic_addr;
vir_bytes count;
{
	vir_bytes vir_hw;//, vir_user;
	//int bytes, i, r;
	int i, r;
	vir_bytes bytes;

	vir_hw = (vir_bytes)dep->de_locmem + nic_addr;

	i= 0;
	while (count > 0)
	{
		if (i >= IOVEC_NR)
		{
			dp_next_iovec(iovp);
			i= 0;
			continue;
		}
		assert(i < iovp->iod_iovec_s);
		if (offset >= iovp->iod_iovec[i].iov_size)
		{
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;

		r= sys_vircopy(iovp->iod_proc_nr, D,
			iovp->iod_iovec[i].iov_addr + offset,
			SELF, D, vir_hw, bytes);
		if (r != OK)
			panic("DP8390", "dp_user2nic: sys_vircopy failed", r);

		count -= bytes;
		vir_hw += bytes;
		offset += bytes;
	}
	assert(count == 0);
}

/*===========================================================================*
 *				dp_pio8_user2nic			     *
 *===========================================================================*/
static void dp_pio8_user2nic(dep, iovp, offset, nic_addr, count)
dpeth_t *dep;
iovec_dat_t *iovp;
vir_bytes offset;
int nic_addr;
vir_bytes count;
{
//	phys_bytes phys_user;
	int i;
	vir_bytes bytes;

	outb_reg0(dep, DP_ISR, ISR_RDC);

	outb_reg0(dep, DP_RBCR0, count &0xFF);
	outb_reg0(dep, DP_RBCR1, count >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr &0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);

	i= 0;
	while (count > 0)
	{
		if (i >= IOVEC_NR)
		{
			dp_next_iovec(iovp);
			i= 0;
			continue;
		}
		assert(i < iovp->iod_iovec_s);
		if (offset >= iovp->iod_iovec[i].iov_size)
		{
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;

		do_vir_outsb(dep->de_data_port, iovp->iod_proc_nr,
			iovp->iod_iovec[i].iov_addr + offset, bytes);
		count -= bytes;
		offset += bytes;
	}
	assert(count == 0);

	for (i= 0; i<100; i++)
	{
		if (inb_reg0(dep, DP_ISR) &ISR_RDC)
			break;
	}
	if (i == 100)
	{
		panic("", "dp8390: remote dma failed to complete", NO_NUM);
	}
}

/*===========================================================================*
 *				dp_pio16_user2nic			     *
 *===========================================================================*/
static void dp_pio16_user2nic(dep, iovp, offset, nic_addr, count)
dpeth_t *dep;
iovec_dat_t *iovp;
vir_bytes offset;
int nic_addr;
vir_bytes count;
{
	vir_bytes vir_user;
	vir_bytes ecount;
	int i, r, user_proc;
	vir_bytes bytes;
	//u8_t two_bytes[2];
	u16_t two_bytes;
	int odd_byte;

	ecount= (count+1) &~1;
	odd_byte= 0;

	outb_reg0(dep, DP_ISR, ISR_RDC);
	outb_reg0(dep, DP_RBCR0, ecount &0xFF);
	outb_reg0(dep, DP_RBCR1, ecount >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr &0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);

	i= 0;
	while (count > 0)
	{
		if (i >= IOVEC_NR)
		{
			dp_next_iovec(iovp);
			i= 0;
			continue;
		}
		assert(i < iovp->iod_iovec_s);
		if (offset >= iovp->iod_iovec[i].iov_size)
		{
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;

		user_proc= iovp->iod_proc_nr;
		vir_user= iovp->iod_iovec[i].iov_addr + offset;
		if (odd_byte)
		{
			r= sys_vircopy(user_proc, D, vir_user, 
			//	SELF, D, (vir_bytes)&two_bytes[1], 1);
				SELF, D, (vir_bytes)&(((u8_t *)&two_bytes)[1]), 1);
			if (r != OK)
			{
				panic("DP8390",
					"dp_pio16_user2nic: sys_vircopy failed",
					r);
			}
			//outw(dep->de_data_port, *(u16_t *)two_bytes);
			outw(dep->de_data_port, two_bytes);
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 0;
			if (!bytes)
				continue;
		}
		ecount= bytes &~1;
		if (ecount != 0)
		{
			do_vir_outsw(dep->de_data_port, user_proc, vir_user,
				ecount);
			count -= ecount;
			offset += ecount;
			bytes -= ecount;
			vir_user += ecount;
		}
		if (bytes)
		{
			assert(bytes == 1);
			r= sys_vircopy(user_proc, D, vir_user, 
			//	SELF, D, (vir_bytes)&two_bytes[0], 1);
				SELF, D, (vir_bytes)&(((u8_t *)&two_bytes)[0]), 1);
			if (r != OK)
			{
				panic("DP8390",
					"dp_pio16_user2nic: sys_vircopy failed",
					r);
			}
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 1;
		}
	}
	assert(count == 0);

	if (odd_byte)
		//outw(dep->de_data_port, *(u16_t *)two_bytes);
		outw(dep->de_data_port, two_bytes);

	for (i= 0; i<100; i++)
	{
		if (inb_reg0(dep, DP_ISR) &ISR_RDC)
			break;
	}
	if (i == 100)
	{
		panic("", "dp8390: remote dma failed to complete", NO_NUM);
	}
}

/*===========================================================================*
 *				dp_nic2user				     *
 *===========================================================================*/
static void dp_nic2user(dep, nic_addr, iovp, offset, count)
dpeth_t *dep;
int nic_addr;
iovec_dat_t *iovp;
vir_bytes offset;
vir_bytes count;
{
	vir_bytes vir_hw;//, vir_user;
	vir_bytes bytes;
	int i, r;

	vir_hw = (vir_bytes)dep->de_locmem + nic_addr;

	i= 0;
	while (count > 0)
	{
		if (i >= IOVEC_NR)
		{
			dp_next_iovec(iovp);
			i= 0;
			continue;
		}
		assert(i < iovp->iod_iovec_s);
		if (offset >= iovp->iod_iovec[i].iov_size)
		{
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;

		r= sys_vircopy(SELF, D, vir_hw,
			iovp->iod_proc_nr, D,
			iovp->iod_iovec[i].iov_addr + offset, bytes);
		if (r != OK)
			panic("DP8390", "dp_nic2user: sys_vircopy failed", r);

		count -= bytes;
		vir_hw += bytes;
		offset += bytes;
	}
	assert(count == 0);
}

/*===========================================================================*
 *				dp_pio8_nic2user			     *
 *===========================================================================*/
static void dp_pio8_nic2user(dep, nic_addr, iovp, offset, count)
dpeth_t *dep;
int nic_addr;
iovec_dat_t *iovp;
vir_bytes offset;
vir_bytes count;
{
//	phys_bytes phys_user;
	int i;
	vir_bytes bytes;

	outb_reg0(dep, DP_RBCR0, count &0xFF);
	outb_reg0(dep, DP_RBCR1, count >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr &0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);

	i= 0;
	while (count > 0)
	{
		if (i >= IOVEC_NR)
		{
			dp_next_iovec(iovp);
			i= 0;
			continue;
		}
		assert(i < iovp->iod_iovec_s);
		if (offset >= iovp->iod_iovec[i].iov_size)
		{
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;

		do_vir_insb(dep->de_data_port, iovp->iod_proc_nr,
			iovp->iod_iovec[i].iov_addr + offset, bytes);
		count -= bytes;
		offset += bytes;
	}
	assert(count == 0);
}

/*===========================================================================*
 *				dp_pio16_nic2user			     *
 *===========================================================================*/
static void dp_pio16_nic2user(dep, nic_addr, iovp, offset, count)
dpeth_t *dep;
int nic_addr;
iovec_dat_t *iovp;
vir_bytes offset;
vir_bytes count;
{
	vir_bytes vir_user;
	vir_bytes ecount;
	int i, r, user_proc;
	vir_bytes bytes;
	//u8_t two_bytes[2];
	u16_t two_bytes;
	int odd_byte;

	ecount= (count+1) &~1;
	odd_byte= 0;

	outb_reg0(dep, DP_RBCR0, ecount &0xFF);
	outb_reg0(dep, DP_RBCR1, ecount >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr &0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);

	i= 0;
	while (count > 0)
	{
		if (i >= IOVEC_NR)
		{
			dp_next_iovec(iovp);
			i= 0;
			continue;
		}
		assert(i < iovp->iod_iovec_s);
		if (offset >= iovp->iod_iovec[i].iov_size)
		{
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;

		user_proc= iovp->iod_proc_nr;
		vir_user= iovp->iod_iovec[i].iov_addr + offset;
		if (odd_byte)
		{
			//r= sys_vircopy(SELF, D, (vir_bytes)&two_bytes[1],
			r= sys_vircopy(SELF, D, (vir_bytes)&(((u8_t *)&two_bytes)[1]),
				user_proc, D, vir_user,  1);
			if (r != OK)
			{
				panic("DP8390",
					"dp_pio16_nic2user: sys_vircopy failed",
					r);
			}
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 0;
			if (!bytes)
				continue;
		}
		ecount= bytes &~1;
		if (ecount != 0)
		{
			do_vir_insw(dep->de_data_port, user_proc, vir_user,
				ecount);
			count -= ecount;
			offset += ecount;
			bytes -= ecount;
			vir_user += ecount;
		}
		if (bytes)
		{
			assert(bytes == 1);
			//*(u16_t *)two_bytes= inw(dep->de_data_port);
			two_bytes= inw(dep->de_data_port);
			//r= sys_vircopy(SELF, D, (vir_bytes)&two_bytes[0],
			r= sys_vircopy(SELF, D, (vir_bytes)&(((u8_t *)&two_bytes)[0]),
				user_proc, D, vir_user,  1);
			if (r != OK)
			{
				panic("DP8390",
					"dp_pio16_nic2user: sys_vircopy failed",
					r);
			}
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 1;
		}
	}
	assert(count == 0);
}

/*===========================================================================*
 *				dp_next_iovec				     *
 *===========================================================================*/
static void dp_next_iovec(iovp)
iovec_dat_t *iovp;
{
	assert(iovp->iod_iovec_s > IOVEC_NR);

	iovp->iod_iovec_s -= IOVEC_NR;

	iovp->iod_iovec_addr += IOVEC_NR * sizeof(iovec_t);

	get_userdata(iovp->iod_proc_nr, iovp->iod_iovec_addr, 
		(iovp->iod_iovec_s > IOVEC_NR ? IOVEC_NR : iovp->iod_iovec_s) *
		sizeof(iovec_t), iovp->iod_iovec); 
}

/*===========================================================================*
 *				conf_hw					     *
 *===========================================================================*/
static void conf_hw(dep)
dpeth_t *dep;
{
//	static eth_stat_t empty_stat = {0, 0, 0, 0, 0, 0 	/* ,... */};

//	int ifnr;
//	dp_conf_t *dcp;

//	dep->de_mode= DEM_DISABLED;	/* Superfluous */
//	ifnr= dep-de_table;

//	dcp= &dp_conf[ifnr];
//	update_conf(dep, dcp);
//	if (dep->de_mode != DEM_ENABLED)
//		return;
	if (!wdeth_probe(dep) && !ne_probe(dep) && !el2_probe(dep))
	{
		printf("%s: No ethernet card found at 0x%x\n", 
			dep->de_name, dep->de_base_port);
		dep->de_mode= DEM_DISABLED;
		return;
	}

/* XXX */ if (dep->de_linmem == 0) dep->de_linmem= 0xFFFF0000;

	dep->de_mode = DEM_ENABLED;

	dep->de_flags = DEF_EMPTY;
//	dep->de_stat = empty_stat;
}

/*===========================================================================*
 *				map_hw_buffer				     *
 *===========================================================================*/
static void map_hw_buffer(dep)
dpeth_t *dep;
{
//	int r;
//	size_t o, size;
//	char *buf, *abuf;

	if (dep->de_prog_IO)
	{
#if 0
		if(debug){
			printf(
			"map_hw_buffer: programmed I/O, no need to map buffer\n");
		}
#endif
		dep->de_locmem = (char *)-dep->de_ramsize; /* trap errors */
		return;
	}else{
		printf("map_hw_buffer: no buffer!\n");
	}

//	size = dep->de_ramsize + PAGE_SIZE;	/* Add PAGE_SIZE for
//						 * alignment
//						 */
//	buf= malloc(size);
//	if (buf == NULL)
//		panic(__FILE__, "map_hw_buffer: cannot malloc size", size);
//	o= PAGE_SIZE - ((vir_bytes)buf % PAGE_SIZE);
//	abuf= buf + o;
//	printf("buf at 0x%x, abuf at 0x%x\n", buf, abuf);

//	r= sys_vm_map(SELF, 1 /* map */, (vir_bytes)abuf,
//			dep->de_ramsize, (phys_bytes)dep->de_linmem);
//	if (r != OK)
//		panic(__FILE__, "map_hw_buffer: sys_vm_map failed", r);
//	dep->de_locmem = abuf;
}

/*===========================================================================*
 *				reply					     *
 *===========================================================================*/
static void reply(dep, err, may_block)
dpeth_t *dep;
int err;
int may_block;
{
/*	message reply;
	int status;
	int r;

	status = 0;
	if (dep->de_flags &DEF_PACK_SEND)
		status |= DL_PACK_SEND;
	if (dep->de_flags &DEF_PACK_RECV)
		status |= DL_PACK_RECV;

	reply.m_type = DL_TASK_REPLY;
	reply.DL_PORT = dep - de_table;
	reply.DL_PROC = dep->de_client;
	reply.DL_STAT = status | ((u32_t) err << 16);
	reply.DL_COUNT = dep->de_read_s;
	reply.DL_CLCK = 0;	*//* Don't know */
/*	r= send(dep->de_client, &reply);

	if (r == ELOCKED && may_block)
	{
#if 0
		printf("send locked\n");
#endif
		return;
	}

	if (r < 0)
		panic("", "dp8390: send failed:", r);
	
*/	dep->de_read_s = 0;
//	dep->de_flags &= ~(DEF_PACK_SEND | DEF_PACK_RECV);
}

/*===========================================================================*
 *				get_userdata				     *
 *===========================================================================*/
static void get_userdata(user_proc, user_addr, count, loc_addr)
int user_proc;
vir_bytes user_addr;
vir_bytes count;
void *loc_addr;
{
	int r;

	r= sys_vircopy(user_proc, D, user_addr,
		SELF, D, (vir_bytes)loc_addr, count);
	if (r != OK)
		panic("DP8390", "get_userdata: sys_vircopy failed", r);
}

static void insb(port_t port, void *buf, size_t size)
{
	size_t i;

	for(i = 0; i < size; ++ i){
		*((uint8_t *) buf + i) = inb(port);
	}
}

static void insw(port_t port, void *buf, size_t size)
{
	size_t i;

	for(i = 0; i * 2 < size; ++ i){
		*((uint16_t *) buf + i) = inw(port);
	}
}

static void outsb(port_t port, void *buf, size_t size)
{
	size_t i;

	for(i = 0; i < size; ++ i){
		outb(port, *((uint8_t *) buf + i));
	}
}

static void outsw(port_t port, void *buf, size_t size)
{
	size_t i;

	for(i = 0; i * 2 < size; ++ i){
		outw(port, *((uint16_t *) buf + i));
	}
}

/*
 * $PchId: dp8390.c,v 1.25 2005/02/10 17:32:07 philip Exp $
 */

/** @}
 */
