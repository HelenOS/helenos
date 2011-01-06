/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  DP8390 network interface core implementation.
 */

#include <assert.h>
#include <byteorder.h>
#include <errno.h>
#include <netif_local.h>
#include <net/packet.h>
#include <packet_client.h>
#include "dp8390_drv.h"
#include "dp8390_port.h"
#include "dp8390.h"
#include "ne2000.h"

/** Queues the outgoing packet.
 *  @param[in] dep The network interface structure.
 *  @param[in] packet The outgoing packet.
 *  @return EOK on success.
 *  @return EINVAL
 */
int queue_packet(dpeth_t * dep, packet_t *packet);

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

/* Some clones of the dp8390 and the PC emulator 'Bochs' require the CR_STA
 * on writes to the CR register. Additional CR_STAs do not appear to hurt
 * genuine dp8390s
 */
#define CR_EXTRA  CR_STA

static void dp_init(dpeth_t *dep);
static void dp_reinit(dpeth_t *dep);
static void dp_reset(dpeth_t *dep);
static void dp_recv(dpeth_t *dep);
static void dp_send(dpeth_t *dep);
static void dp_pio8_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst);
static void dp_pio16_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst);
static int dp_pkt2user(dpeth_t *dep, int page, int length);
static void dp_pio8_user2nic(dpeth_t *dep, iovec_dat_t *iovp, size_t offset, int nic_addr, size_t count);
static void dp_pio16_user2nic(dpeth_t *dep, iovec_dat_t *iovp, size_t offset, int nic_addr, size_t count);
static void dp_pio8_nic2user(dpeth_t *dep, int nic_addr, iovec_dat_t *iovp, size_t offset, size_t count);
static void dp_pio16_nic2user(dpeth_t *dep, int nic_addr, iovec_dat_t *iovp, size_t offset, size_t count);
static void dp_next_iovec(iovec_dat_t *iovp);
static void conf_hw(dpeth_t *dep);
static void reply(dpeth_t *dep, int err, int may_block);
static void insb(port_t port, void *buf, size_t size);
static void insw(port_t port, void *buf, size_t size);

int do_probe(dpeth_t *dep)
{
	/* This is the default, try to (re)locate the device. */
	conf_hw(dep);
	if (dep->de_mode == DEM_DISABLED)
		/* Probe failed, or the device is configured off. */
		return EXDEV;
	
	if (dep->de_mode == DEM_ENABLED)
		dp_init(dep);
	
	return EOK;
}

int do_init(dpeth_t *dep, int mode)
{
	if (dep->de_mode == DEM_DISABLED)
		// might call do_probe()
		return EXDEV;
	
	if (dep->de_mode == DEM_SINK) {
//		strncpy((char *) dep->de_address.ea_addr, "ZDP", 6);
//		dep->de_address.ea_addr[5] = port;
//		reply_mess.m_type = DL_CONF_REPLY;
//		reply_mess.m3_i1 = mp->DL_PORT;
//		reply_mess.m3_i2 = DE_PORT_NR;
//		*(ether_addr_t *) reply_mess.m3_ca1 = dep->de_address;
//		mess_reply(mp, &reply_mess);
//		return;
		return EOK;
	}
	
	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags & DEF_ENABLED);
	
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

void do_stop(dpeth_t *dep)
{
	if ((dep->de_mode != DEM_SINK)
	    && (dep->de_mode == DEM_ENABLED)
	    && (dep->de_flags & DEF_ENABLED)) {
		outb_reg0(dep, DP_CR, CR_STP | CR_DM_ABORT);
		(dep->de_stopf)(dep);
		dep->de_flags = DEF_EMPTY;
	}
}

int queue_packet(dpeth_t *dep, packet_t *packet)
{
	packet_t *tmp;
	
	if (dep->packet_count >= MAX_PACKETS) {
		netif_pq_release(packet_get_id(packet));
		return ELIMIT;
	}
	
	tmp = dep->packet_queue;
	while (pq_next(tmp))
		tmp = pq_next(tmp);
	
	if (pq_add(&tmp, packet, 0, 0) != EOK)
		return EINVAL;
	
	if (!dep->packet_count)
		dep->packet_queue = packet;
	
	++dep->packet_count;
	return EBUSY;
}

int do_pwrite(dpeth_t *dep, packet_t *packet, int from_int)
{
//	int port, count, size;
	int size;
	int sendq_head;
/*	dpeth_t *dep;

	port = mp->DL_PORT;
	count = mp->DL_COUNT;
	if (port < 0 || port >= DE_PORT_NR)
		fprintf(stderr, "dp8390: illegal port\n");
	dep= &de_table[port];
	dep->de_client= mp->DL_PROC;
*/
	if (dep->de_mode == DEM_SINK) {
		assert(!from_int);
//		dep->de_flags |= DEF_PACK_SEND;
		reply(dep, EOK, false);
//		return;
		return EOK;
	}
	
	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags &DEF_ENABLED);
	
	if ((dep->packet_queue) && (!from_int)) {
//	if (dep->de_flags &DEF_SEND_AVAIL){
//		fprintf(stderr, "dp8390: send already in progress\n");
		return queue_packet(dep, packet);
	}
	
	sendq_head= dep->de_sendq_head;
//	if (dep->de_sendq[sendq_head].sq_filled) {
//		if (from_int)
//			fprintf(stderr, "dp8390: should not be sending\n");
//		dep->de_sendmsg= *mp;
//		dep->de_flags |= DEF_SEND_AVAIL;
//		reply(dep, EOK, false);
//		return;
//		return queue_packet(dep, packet);
//	}
//	assert(!(dep->de_flags &DEF_PACK_SEND));
	
/*	if (vectored) {
		get_userdata(mp->DL_PROC, (vir_bytes) mp->DL_ADDR,
			(count > IOVEC_NR ? IOVEC_NR : count) *
			sizeof(iovec_t), dep->de_write_iovec.iod_iovec);
		dep->de_write_iovec.iod_iovec_s = count;
		dep->de_write_iovec.iod_iovec_addr = (vir_bytes) mp->DL_ADDR;
		
		dep->de_tmp_iovec = dep->de_write_iovec;
		size = calc_iovec_size(&dep->de_tmp_iovec);
	} else {
		dep->de_write_iovec.iod_iovec[0].iov_addr =
			(vir_bytes) mp->DL_ADDR;
		dep->de_write_iovec.iod_iovec[0].iov_size =
			mp->DL_COUNT;
		dep->de_write_iovec.iod_iovec_s = 1;
		dep->de_write_iovec.iod_iovec_addr = 0;
		size= mp->DL_COUNT;
	}
*/
	size = packet_get_data_length(packet);
	dep->de_write_iovec.iod_iovec[0].iov_addr = (void *) packet_get_data(packet);
	dep->de_write_iovec.iod_iovec[0].iov_size = size;
	dep->de_write_iovec.iod_iovec_s = 1;
	dep->de_write_iovec.iod_iovec_addr = NULL;
	
	if (size < ETH_MIN_PACK_SIZE || size > ETH_MAX_PACK_SIZE_TAGGED) {
		fprintf(stderr, "dp8390: invalid packet size\n");
		return EINVAL;
	}
	
	(dep->de_user2nicf)(dep, &dep->de_write_iovec, 0,
	    dep->de_sendq[sendq_head].sq_sendpage * DP_PAGESIZE,
	    size);
	dep->de_sendq[sendq_head].sq_filled= true;
	if (dep->de_sendq_tail == sendq_head) {
		outb_reg0(dep, DP_TPSR, dep->de_sendq[sendq_head].sq_sendpage);
		outb_reg0(dep, DP_TBCR1, size >> 8);
		outb_reg0(dep, DP_TBCR0, size & 0xff);
		outb_reg0(dep, DP_CR, CR_TXP | CR_EXTRA);  /* there it goes.. */
	} else
		dep->de_sendq[sendq_head].sq_size = size;
	
	if (++sendq_head == dep->de_sendq_nr)
		sendq_head= 0;
	
	assert(sendq_head < SENDQ_NR);
	dep->de_sendq_head = sendq_head;
	
//	dep->de_flags |= DEF_PACK_SEND;
	
	/*
	 * If the interrupt handler called, don't send a reply. The reply
	 * will be sent after all interrupts are handled. 
	 */
	if (from_int)
		return EOK;
	
	reply(dep, EOK, false);
	
	assert(dep->de_mode == DEM_ENABLED);
	assert(dep->de_flags & DEF_ENABLED);
	
	return EOK;
}

void dp_init(dpeth_t *dep)
{
	int dp_rcr_reg;
	int i;
	
	/* General initialization */
	dep->de_flags = DEF_EMPTY;
	(*dep->de_initf)(dep);
	
	printf("%s: Ethernet address ", dep->de_name);
	for (i = 0; i < 6; i++)
		printf("%x%c", dep->de_address.ea_addr[i], i < 5 ? ':' : '\n');
	
	/*
	 * Initialization of the dp8390 following the mandatory procedure
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
	
	if (dep->de_flags & DEF_PROMISC)
		dp_rcr_reg |= RCR_AB | RCR_PRO | RCR_AM;
	
	if (dep->de_flags & DEF_BROAD)
		dp_rcr_reg |= RCR_AB;
	
	if (dep->de_flags & DEF_MULTI)
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
	
	inb_reg0(dep, DP_CNTR0);  /* Reset counters by reading */
	inb_reg0(dep, DP_CNTR1);
	inb_reg0(dep, DP_CNTR2);
	
	/* Finish the initialization. */
	dep->de_flags |= DEF_ENABLED;
	for (i = 0; i < dep->de_sendq_nr; i++)
		dep->de_sendq[i].sq_filled= 0;
	
	dep->de_sendq_head = 0;
	dep->de_sendq_tail = 0;
	
	if (dep->de_16bit) {
		dep->de_user2nicf= dp_pio16_user2nic;
		dep->de_nic2userf= dp_pio16_nic2user;
		dep->de_getblockf= dp_pio16_getblock;
	} else {
		dep->de_user2nicf= dp_pio8_user2nic;
		dep->de_nic2userf= dp_pio8_nic2user;
		dep->de_getblockf= dp_pio8_getblock;
	}
}

static void dp_reinit(dpeth_t *dep)
{
	int dp_rcr_reg;
	
	outb_reg0(dep, DP_CR, CR_PS_P0 | CR_EXTRA);
	
	dp_rcr_reg = 0;
	
	if (dep->de_flags & DEF_PROMISC)
		dp_rcr_reg |= RCR_AB | RCR_PRO | RCR_AM;
	
	if (dep->de_flags & DEF_BROAD)
		dp_rcr_reg |= RCR_AB;
	
	if (dep->de_flags & DEF_MULTI)
		dp_rcr_reg |= RCR_AM;
	
	outb_reg0(dep, DP_RCR, dp_rcr_reg);
}

static void dp_reset(dpeth_t *dep)
{
	int i;
	
	/* Stop chip */
	outb_reg0(dep, DP_CR, CR_STP | CR_DM_ABORT);
	outb_reg0(dep, DP_RBCR0, 0);
	outb_reg0(dep, DP_RBCR1, 0);
	
	for (i = 0; i < 0x1000 && ((inb_reg0(dep, DP_ISR) & ISR_RST) == 0); i++)
		; /* Do nothing */
	
	outb_reg0(dep, DP_TCR, TCR_1EXTERNAL | TCR_OFST);
	outb_reg0(dep, DP_CR, CR_STA | CR_DM_ABORT);
	outb_reg0(dep, DP_TCR, TCR_NORMAL);
	
	/* Acknowledge the ISR_RDC (remote DMA) interrupt. */
	for (i = 0; i < 0x1000 && ((inb_reg0(dep, DP_ISR) &ISR_RDC) == 0); i++)
		; /* Do nothing */
	
	outb_reg0(dep, DP_ISR, inb_reg0(dep, DP_ISR) & ~ISR_RDC);
	
	/*
	 * Reset the transmit ring. If we were transmitting a packet, we
	 * pretend that the packet is processed. Higher layers will
	 * retransmit if the packet wasn't actually sent.
	 */
	dep->de_sendq_head = 0;
	dep->de_sendq_tail = 0;
	
	for (i = 0; i < dep->de_sendq_nr; i++)
		dep->de_sendq[i].sq_filled = 0;
	
	dp_send(dep);
	dep->de_flags &= ~DEF_STOPPED;
}

void dp_check_ints(dpeth_t *dep, int isr)
{
	int tsr;
	int size, sendq_tail;
	
	if (!(dep->de_flags & DEF_ENABLED))
		fprintf(stderr, "dp8390: got premature interrupt\n");
	
	for (; isr; isr = inb_reg0(dep, DP_ISR)) {
		outb_reg0(dep, DP_ISR, isr);
		
		if (isr & (ISR_PTX | ISR_TXE)) {
			if (isr & ISR_TXE)
				dep->de_stat.ets_sendErr++;
			else {
				tsr = inb_reg0(dep, DP_TSR);
				
				if (tsr & TSR_PTX)
					dep->de_stat.ets_packetT++;
				
				if (tsr & TSR_COL)
					dep->de_stat.ets_collision++;
				
				if (tsr & TSR_ABT)
					dep->de_stat.ets_transAb++;
				
				if (tsr & TSR_CRS)
					dep->de_stat.ets_carrSense++;
				
				if ((tsr & TSR_FU) && (++dep->de_stat.ets_fifoUnder <= 10))
					printf("%s: fifo underrun\n", dep->de_name);
				
				if ((tsr & TSR_CDH) && (++dep->de_stat.ets_CDheartbeat <= 10))
					printf("%s: CD heart beat failure\n", dep->de_name);
				
				if (tsr & TSR_OWC)
					dep->de_stat.ets_OWC++;
			}
			
			sendq_tail = dep->de_sendq_tail;
			
			if (!(dep->de_sendq[sendq_tail].sq_filled)) {
				/* Or hardware bug? */
				printf("%s: transmit interrupt, but not sending\n", dep->de_name);
				continue;
			}
			
			dep->de_sendq[sendq_tail].sq_filled = 0;
			
			if (++sendq_tail == dep->de_sendq_nr)
				sendq_tail = 0;
			
			dep->de_sendq_tail = sendq_tail;
			
			if (dep->de_sendq[sendq_tail].sq_filled) {
				size = dep->de_sendq[sendq_tail].sq_size;
				outb_reg0(dep, DP_TPSR,
				    dep->de_sendq[sendq_tail].sq_sendpage);
				outb_reg0(dep, DP_TBCR1, size >> 8);
				outb_reg0(dep, DP_TBCR0, size & 0xff);
				outb_reg0(dep, DP_CR, CR_TXP | CR_EXTRA);
			}
			
//			if (dep->de_flags &DEF_SEND_AVAIL)
				dp_send(dep);
		}
		
		if (isr & ISR_PRX) {
			/* Only call dp_recv if there is a read request */
//			if (dep->de_flags) &DEF_READING)
				dp_recv(dep);
		}
		
		if (isr & ISR_RXE)
			dep->de_stat.ets_recvErr++;
		
		if (isr & ISR_CNT) {
			dep->de_stat.ets_CRCerr += inb_reg0(dep, DP_CNTR0);
			dep->de_stat.ets_frameAll += inb_reg0(dep, DP_CNTR1);
			dep->de_stat.ets_missedP += inb_reg0(dep, DP_CNTR2);
		}
		
		if (isr & ISR_OVW) {
			dep->de_stat.ets_OVW++;
/*			if (dep->de_flags & DEF_READING) {
				printf("dp_check_ints: strange: overwrite warning and pending read request\n");
				dp_recv(dep);
			}
*/		}
		
		if (isr & ISR_RDC) {
			/* Nothing to do */
		}
		
		if (isr & ISR_RST) {
			/*
			 * This means we got an interrupt but the ethernet 
			 * chip is shutdown. We set the flag DEF_STOPPED,
			 * and continue processing arrived packets. When the
			 * receive buffer is empty, we reset the dp8390.
			 */
			dep->de_flags |= DEF_STOPPED;
			break;
		}
	}
	
//	if ((dep->de_flags & (DEF_READING | DEF_STOPPED)) == (DEF_READING | DEF_STOPPED))
	if ((dep->de_flags & DEF_STOPPED) == DEF_STOPPED) {
		/*
		 * The chip is stopped, and all arrived packets
		 * are delivered.
		 */
		dp_reset(dep);
	}
}

static void dp_recv(dpeth_t *dep)
{
	dp_rcvhdr_t header;
	//unsigned pageno, curr, next;
	int pageno, curr, next;
	size_t length;
	int packet_processed, r;
	uint16_t eth_type;
	
	packet_processed = false;
	pageno = inb_reg0(dep, DP_BNRY) + 1;
	if (pageno == dep->de_stoppage)
		pageno = dep->de_startpage;
	
	do {
		outb_reg0(dep, DP_CR, CR_PS_P1 | CR_EXTRA);
		curr = inb_reg1(dep, DP_CURR);
		outb_reg0(dep, DP_CR, CR_PS_P0 | CR_EXTRA);
		
		if (curr == pageno)
			break;
		
		(dep->de_getblockf)(dep, pageno, (size_t) 0, sizeof(header), &header);
		(dep->de_getblockf)(dep, pageno, sizeof(header) + 2 * sizeof(ether_addr_t), sizeof(eth_type), &eth_type);
		
		length = (header.dr_rbcl | (header.dr_rbch << 8)) - sizeof(dp_rcvhdr_t);
		next = header.dr_next;
		if ((length < ETH_MIN_PACK_SIZE) || (length > ETH_MAX_PACK_SIZE_TAGGED)) {
			printf("%s: packet with strange length arrived: %d\n", dep->de_name, (int) length);
			next= curr;
		} else if ((next < dep->de_startpage) || (next >= dep->de_stoppage)) {
			printf("%s: strange next page\n", dep->de_name);
			next= curr;
		} else if (header.dr_status & RSR_FO) {
			/*
			 * This is very serious, so we issue a warning and
			 * reset the buffers
			 */
			printf("%s: fifo overrun, resetting receive buffer\n", dep->de_name);
			dep->de_stat.ets_fifoOver++;
			next = curr;
		} else if ((header.dr_status & RSR_PRX) && (dep->de_flags & DEF_ENABLED)) {
			r = dp_pkt2user(dep, pageno, length);
			if (r != EOK)
				return;
			
			packet_processed = true;
			dep->de_stat.ets_packetR++;
		}
		
		if (next == dep->de_startpage)
			outb_reg0(dep, DP_BNRY, dep->de_stoppage - 1);
		else
			outb_reg0(dep, DP_BNRY, next - 1);
		
		pageno = next;
	} while (!packet_processed);
}

static void dp_send(dpeth_t *dep)
{
	packet_t *packet;
	
//	if (!(dep->de_flags &DEF_SEND_AVAIL))
//		return;
	
	if (dep->packet_queue) {
		packet = dep->packet_queue;
		dep->packet_queue = pq_detach(packet);
		do_pwrite(dep, packet, true);
		netif_pq_release(packet_get_id(packet));
		--dep->packet_count;
	}
	
//	if (!dep->packet_queue)
//		dep->de_flags &= ~DEF_SEND_AVAIL;
	
/*	switch (dep->de_sendmsg.m_type) {
	case DL_WRITE:
		do_vwrite(&dep->de_sendmsg, true, false);
		break;
	case DL_WRITEV:
		do_vwrite(&dep->de_sendmsg, true, true);
		break;
	case DL_WRITEV_S:
		do_vwrite_s(&dep->de_sendmsg, true);
		break;
	default:
		fprintf(stderr, "dp8390: wrong type\n");
		break;
	}
*/
}

static void dp_pio8_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst)
{
	offset = page * DP_PAGESIZE + offset;
	outb_reg0(dep, DP_RBCR0, size &0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, offset &0xFF);
	outb_reg0(dep, DP_RSAR1, offset >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	insb(dep->de_data_port, dst, size);
}

static void dp_pio16_getblock(dpeth_t *dep, int page, size_t offset, size_t size, void *dst)
{
	offset = page * DP_PAGESIZE + offset;
	outb_reg0(dep, DP_RBCR0, size &0xFF);
	outb_reg0(dep, DP_RBCR1, size >> 8);
	outb_reg0(dep, DP_RSAR0, offset &0xFF);
	outb_reg0(dep, DP_RSAR1, offset >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	assert(!(size & 1));
	
	insw(dep->de_data_port, dst, size);
}

static int dp_pkt2user(dpeth_t *dep, int page, int length)
{
	int last, count;
	packet_t *packet;
	
//	if (!(dep->de_flags & DEF_READING))
//		return EINVAL;
	
	packet = netif_packet_get_1(length);
	if (!packet)
		return ENOMEM;
	
	dep->de_read_iovec.iod_iovec[0].iov_addr = (void *) packet_suffix(packet, length);
	dep->de_read_iovec.iod_iovec[0].iov_size = length;
	dep->de_read_iovec.iod_iovec_s = 1;
	dep->de_read_iovec.iod_iovec_addr = NULL;
	
	last = page + (length - 1) / DP_PAGESIZE;
	if (last >= dep->de_stoppage) {
		count = (dep->de_stoppage - page) * DP_PAGESIZE - sizeof(dp_rcvhdr_t);
		
		/* Save read_iovec since we need it twice. */
		dep->de_tmp_iovec = dep->de_read_iovec;
		(dep->de_nic2userf)(dep, page * DP_PAGESIZE +
		    sizeof(dp_rcvhdr_t), &dep->de_tmp_iovec, 0, count);
		(dep->de_nic2userf)(dep, dep->de_startpage * DP_PAGESIZE,
		    &dep->de_read_iovec, count, length - count);
	} else {
		(dep->de_nic2userf)(dep, page * DP_PAGESIZE +
		    sizeof(dp_rcvhdr_t), &dep->de_read_iovec, 0, length);
	}
	
	dep->de_read_s = length;
	dep->de_flags |= DEF_PACK_RECV;
//	dep->de_flags &= ~DEF_READING;
	
	if (dep->received_count >= MAX_PACKETS) {
		netif_pq_release(packet_get_id(packet));
		return ELIMIT;
	} else {
		if (pq_add(&dep->received_queue, packet, 0, 0) == EOK)
			++dep->received_count;
		else
			netif_pq_release(packet_get_id(packet));
	}
	
	return EOK;
}

static void dp_pio8_user2nic(dpeth_t *dep, iovec_dat_t *iovp, size_t offset, int nic_addr, size_t count)
{
//	phys_bytes phys_user;
	int i;
	size_t bytes;
	
	outb_reg0(dep, DP_ISR, ISR_RDC);
	
	outb_reg0(dep, DP_RBCR0, count & 0xFF);
	outb_reg0(dep, DP_RBCR1, count >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);
	
	i = 0;
	while (count > 0) {
		if (i >= IOVEC_NR) {
			dp_next_iovec(iovp);
			i = 0;
			continue;
		}
		
		assert(i < iovp->iod_iovec_s);
		
		if (offset >= iovp->iod_iovec[i].iov_size) {
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;
		
		do_vir_outsb(dep->de_data_port, iovp->iod_iovec[i].iov_addr + offset, bytes);
		count -= bytes;
		offset += bytes;
	}
	
	assert(count == 0);
	
	for (i = 0; i < 100; i++) {
		if (inb_reg0(dep, DP_ISR) & ISR_RDC)
			break;
	}
	
	if (i == 100)
		fprintf(stderr, "dp8390: remote dma failed to complete\n");
}

static void dp_pio16_user2nic(dpeth_t *dep, iovec_dat_t *iovp, size_t offset, int nic_addr, size_t count)
{
	void *vir_user;
	size_t ecount;
	int i;
	size_t bytes;
	//uint8_t two_bytes[2];
	uint16_t two_bytes;
	int odd_byte;
	
	ecount = (count + 1) & ~1;
	odd_byte = 0;
	
	outb_reg0(dep, DP_ISR, ISR_RDC);
	outb_reg0(dep, DP_RBCR0, ecount & 0xFF);
	outb_reg0(dep, DP_RBCR1, ecount >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RW | CR_PS_P0 | CR_STA);
	
	i = 0;
	while (count > 0) {
		if (i >= IOVEC_NR) {
			dp_next_iovec(iovp);
			i = 0;
			continue;
		}
		
		assert(i < iovp->iod_iovec_s);
		
		if (offset >= iovp->iod_iovec[i].iov_size) {
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;
		
		vir_user = iovp->iod_iovec[i].iov_addr + offset;
		
		if (odd_byte) {
			memcpy(&(((uint8_t *) &two_bytes)[1]), vir_user, 1);
			
			//outw(dep->de_data_port, *(uint16_t *) two_bytes);
			outw(dep->de_data_port, two_bytes);
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 0;
			
			if (!bytes)
				continue;
		}
		
		ecount= bytes & ~1;
		if (ecount != 0) {
			do_vir_outsw(dep->de_data_port, vir_user, ecount);
			count -= ecount;
			offset += ecount;
			bytes -= ecount;
			vir_user += ecount;
		}
		
		if (bytes) {
			assert(bytes == 1);
			
			memcpy(&(((uint8_t *) &two_bytes)[0]), vir_user, 1);
			
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 1;
		}
	}
	
	assert(count == 0);
	
	if (odd_byte)
		//outw(dep->de_data_port, *(uint16_t *) two_bytes);
		outw(dep->de_data_port, two_bytes);
	
	for (i = 0; i < 100; i++) {
		if (inb_reg0(dep, DP_ISR) &ISR_RDC)
			break;
	}
	
	if (i == 100)
		fprintf(stderr, "dp8390: remote dma failed to complete\n");
}

static void dp_pio8_nic2user(dpeth_t *dep, int nic_addr, iovec_dat_t *iovp, size_t offset, size_t count)
{
//	phys_bytes phys_user;
	int i;
	size_t bytes;
	
	outb_reg0(dep, DP_RBCR0, count & 0xFF);
	outb_reg0(dep, DP_RBCR1, count >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	i = 0;
	while (count > 0) {
		if (i >= IOVEC_NR) {
			dp_next_iovec(iovp);
			i= 0;
			continue;
		}
		
		assert(i < iovp->iod_iovec_s);
		
		if (offset >= iovp->iod_iovec[i].iov_size) {
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;
		
		do_vir_insb(dep->de_data_port, iovp->iod_iovec[i].iov_addr + offset, bytes);
		count -= bytes;
		offset += bytes;
	}
	
	assert(count == 0);
}

static void dp_pio16_nic2user(dpeth_t *dep, int nic_addr, iovec_dat_t *iovp, size_t offset, size_t count)
{
	void *vir_user;
	size_t ecount;
	int i;
	size_t bytes;
	//uint8_t two_bytes[2];
	uint16_t two_bytes;
	int odd_byte;
	
	ecount = (count + 1) & ~1;
	odd_byte = 0;
	
	outb_reg0(dep, DP_RBCR0, ecount & 0xFF);
	outb_reg0(dep, DP_RBCR1, ecount >> 8);
	outb_reg0(dep, DP_RSAR0, nic_addr & 0xFF);
	outb_reg0(dep, DP_RSAR1, nic_addr >> 8);
	outb_reg0(dep, DP_CR, CR_DM_RR | CR_PS_P0 | CR_STA);
	
	i = 0;
	while (count > 0) {
		if (i >= IOVEC_NR) {
			dp_next_iovec(iovp);
			i = 0;
			continue;
		}
		
		assert(i < iovp->iod_iovec_s);
		
		if (offset >= iovp->iod_iovec[i].iov_size) {
			offset -= iovp->iod_iovec[i].iov_size;
			i++;
			continue;
		}
		
		bytes = iovp->iod_iovec[i].iov_size - offset;
		if (bytes > count)
			bytes = count;
		
		vir_user = iovp->iod_iovec[i].iov_addr + offset;
		
		if (odd_byte) {
			memcpy(vir_user, &(((uint8_t *) &two_bytes)[1]), 1);
			
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 0;
			
			if (!bytes)
				continue;
		}
		
		ecount= bytes & ~1;
		if (ecount != 0) {
			do_vir_insw(dep->de_data_port, vir_user, ecount);
			count -= ecount;
			offset += ecount;
			bytes -= ecount;
			vir_user += ecount;
		}
		
		if (bytes) {
			assert(bytes == 1);
			
			//*(uint16_t *) two_bytes= inw(dep->de_data_port);
			two_bytes = inw(dep->de_data_port);
			memcpy(vir_user, &(((uint8_t *) &two_bytes)[0]), 1);
			
			count--;
			offset++;
			bytes--;
			vir_user++;
			odd_byte= 1;
		}
	}
	
	assert(count == 0);
}

static void dp_next_iovec(iovec_dat_t *iovp)
{
	assert(iovp->iod_iovec_s > IOVEC_NR);
	
	iovp->iod_iovec_s -= IOVEC_NR;
	iovp->iod_iovec_addr += IOVEC_NR * sizeof(iovec_t);
	
	memcpy(iovp->iod_iovec, iovp->iod_iovec_addr,
	    (iovp->iod_iovec_s > IOVEC_NR ? IOVEC_NR : iovp->iod_iovec_s) *
	    sizeof(iovec_t));
}

static void conf_hw(dpeth_t *dep)
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
	
	if (!ne_probe(dep)) {
		printf("%s: No ethernet card found at %#lx\n",
		    dep->de_name, dep->de_base_port);
		dep->de_mode= DEM_DISABLED;
		return;
	}
	
	dep->de_mode = DEM_ENABLED;
	dep->de_flags = DEF_EMPTY;
//	dep->de_stat = empty_stat;
}

static void reply(dpeth_t *dep, int err, int may_block)
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
	reply.DL_CLCK = 0;  *//* Don't know */
	
/*	r = send(dep->de_client, &reply);
	if (r == ELOCKED && may_block) {
		printf("send locked\n");
		return;
	}
	
	if (r < 0)
		fprintf(stderr, "dp8390: send failed\n");
	
*/	dep->de_read_s = 0;
//	dep->de_flags &= ~(DEF_PACK_SEND | DEF_PACK_RECV);
}

static void insb(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i < size; i++)
		*((uint8_t *) buf + i) = inb(port);
}

static void insw(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i * 2 < size; i++)
		*((uint16_t *) buf + i) = inw(port);
}

static void outsb(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i < size; i++)
		outb(port, *((uint8_t *) buf + i));
}

static void outsw(port_t port, void *buf, size_t size)
{
	size_t i;
	
	for (i = 0; i * 2 < size; i++)
		outw(port, *((uint16_t *) buf + i));
}

/** @}
 */
