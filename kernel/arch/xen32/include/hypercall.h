/*
 * Copyright (C) 2006 Martin Decky
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

#ifndef KERN_xen32_HYPERCALL_H_
#define KERN_xen32_HYPERCALL_H_

#include <arch/types.h>
#include <macros.h>

typedef uint16_t domid_t;

typedef struct {
	uint8_t vector;     /**< Exception vector */
	uint8_t flags;      /**< 0-3: privilege level; 4: clear event enable */
	uint16_t cs;        /**< Code selector */
	void *address;      /**< Code offset */
} trap_info_t;


typedef struct {
	evtchn_t port;
} evtchn_send_t;

typedef struct {
	uint32_t cmd;
	union {
		evtchn_send_t send;
    };
} evtchn_op_t;


#define XEN_SET_TRAP_TABLE		0
#define XEN_MMU_UPDATE			1
#define XEN_SET_CALLBACKS		4
#define XEN_UPDATE_VA_MAPPING	14
#define XEN_EVENT_CHANNEL_OP	16
#define XEN_VERSION				17
#define XEN_CONSOLE_IO			18
#define XEN_VM_ASSIST			21
#define XEN_MMUEXT_OP			26


/*
 * Commands for XEN_CONSOLE_IO
 */
#define CONSOLE_IO_WRITE	0
#define CONSOLE_IO_READ		1


#define MMUEXT_PIN_L1_TABLE      0
#define MMUEXT_PIN_L2_TABLE      1
#define MMUEXT_PIN_L3_TABLE      2
#define MMUEXT_PIN_L4_TABLE      3
#define MMUEXT_UNPIN_TABLE       4
#define MMUEXT_NEW_BASEPTR       5
#define MMUEXT_TLB_FLUSH_LOCAL   6
#define MMUEXT_INVLPG_LOCAL      7
#define MMUEXT_TLB_FLUSH_MULTI   8
#define MMUEXT_INVLPG_MULTI      9
#define MMUEXT_TLB_FLUSH_ALL    10
#define MMUEXT_INVLPG_ALL       11
#define MMUEXT_FLUSH_CACHE      12
#define MMUEXT_SET_LDT          13
#define MMUEXT_NEW_USER_BASEPTR 15


#define EVTCHNOP_SEND			4


#define UVMF_NONE				0        /**< No flushing at all */
#define UVMF_TLB_FLUSH			1        /**< Flush entire TLB(s) */
#define UVMF_INVLPG				2        /**< Flush only one entry */
#define UVMF_FLUSHTYPE_MASK		3
#define UVMF_MULTI				0        /**< Flush subset of TLBs */
#define UVMF_LOCAL				0        /**< Flush local TLB */
#define UVMF_ALL				(1 << 2) /**< Flush all TLBs */


/*
 * Commands to XEN_VM_ASSIST
 */
#define VMASST_CMD_ENABLE				0
#define VMASST_CMD_DISABLE				1
#define VMASST_TYPE_4GB_SEGMENTS		0
#define VMASST_TYPE_4GB_SEGMENTS_NOTIFY	1
#define VMASST_TYPE_WRITABLE_PAGETABLES	2


#define DOMID_SELF (0x7FF0U)
#define DOMID_IO   (0x7FF1U)


#define force_evtchn_callback() ((void) xen_version(0, 0))

#define hypercall0(id)	\
	({	\
		unative_t ret;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret)	\
			:	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall1(id, p1)	\
	({	\
		unative_t ret, __ign1;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1)	\
			: "1" (p1)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall2(id, p1, p2)	\
	({	\
		unative_t ret, __ign1, __ign2;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2)	\
			: "1" (p1),	\
			  "2" (p2)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall3(id, p1, p2, p3)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall4(id, p1, p2, p3, p4)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3, __ign4;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3),	\
			  "=S" (__ign4)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3),	\
			  "4" (p4)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall5(id, p1, p2, p3, p4, p5)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3, __ign4, __ign5;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3),	\
			  "=S" (__ign4),	\
			  "=D" (__ign5)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3),	\
			  "4" (p4),	\
			  "5" (p5)	\
			: "memory"	\
		);	\
		ret;	\
	})


static inline int xen_console_io(const unsigned int cmd, const unsigned int count, const char *str)
{
	return hypercall3(XEN_CONSOLE_IO, cmd, count, str);
}

static inline int xen_vm_assist(const unsigned int cmd, const unsigned int type)
{
    return hypercall2(XEN_VM_ASSIST, cmd, type);
}

static inline int xen_set_callbacks(const unsigned int event_selector, const void *event_address, const	unsigned int failsafe_selector, void *failsafe_address)
{
	return hypercall4(XEN_SET_CALLBACKS, event_selector, event_address, failsafe_selector, failsafe_address);
}

static inline int xen_set_trap_table(const trap_info_t *table)
{
	return hypercall1(XEN_SET_TRAP_TABLE, table);
}

static inline int xen_version(const unsigned int cmd, const void *arg)
{
	return hypercall2(XEN_VERSION, cmd, arg);
}

static inline int xen_notify_remote(evtchn_t channel)
{
    evtchn_op_t op;
	
    op.cmd = EVTCHNOP_SEND;
    op.send.port = channel;
    return hypercall1(XEN_EVENT_CHANNEL_OP, &op);
}

#endif
