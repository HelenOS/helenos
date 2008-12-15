/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

 /** @addtogroup ia32	
 * @{
 */
/** @file
 */

#ifndef __APIC_H__
#define __APIC_H__

#include <arch/types.h>
#include <cpu.h>

#define FIXED		(0<<0)
#define LOPRI		(1<<0)

#define APIC_ID_COUNT	16

/* local APIC macros */
#define IPI_INIT 	0
#define IPI_STARTUP	0

/** Delivery modes. */
#define DELMOD_FIXED	0x0
#define DELMOD_LOWPRI	0x1
#define DELMOD_SMI	0x2
/* 0x3 reserved */
#define DELMOD_NMI	0x4
#define DELMOD_INIT	0x5
#define DELMOD_STARTUP	0x6
#define DELMOD_EXTINT	0x7

/** Destination modes. */
#define DESTMOD_PHYS	0x0
#define DESTMOD_LOGIC	0x1

/** Trigger Modes. */
#define TRIGMOD_EDGE	0x0
#define TRIGMOD_LEVEL	0x1

/** Levels. */
#define LEVEL_DEASSERT	0x0
#define LEVEL_ASSERT	0x1

/** Destination Shorthands. */
#define SHORTHAND_NONE		0x0
#define SHORTHAND_SELF		0x1
#define SHORTHAND_ALL_INCL	0x2
#define SHORTHAND_ALL_EXCL	0x3

/** Interrupt Input Pin Polarities. */
#define POLARITY_HIGH	0x0
#define POLARITY_LOW	0x1

/** Divide Values. (Bit 2 is always 0) */
#define DIVIDE_2	0x0
#define DIVIDE_4	0x1
#define DIVIDE_8	0x2
#define DIVIDE_16	0x3
#define DIVIDE_32	0x8
#define DIVIDE_64	0x9
#define DIVIDE_128	0xa
#define DIVIDE_1	0xb

/** Timer Modes. */
#define TIMER_ONESHOT	0x0
#define TIMER_PERIODIC	0x1

/** Delivery status. */
#define DELIVS_IDLE	0x0
#define DELIVS_PENDING	0x1

/** Destination masks. */
#define DEST_ALL	0xff

/** Dest format models. */
#define MODEL_FLAT	0xf
#define MODEL_CLUSTER	0x0

/** Interrupt Command Register. */
#define ICRlo		(0x300/sizeof(uint32_t))
#define ICRhi		(0x310/sizeof(uint32_t))
struct icr {
	union {
		uint32_t lo;
		struct {
			uint8_t vector;			/**< Interrupt Vector. */
			unsigned delmod : 3;		/**< Delivery Mode. */
			unsigned destmod : 1;		/**< Destination Mode. */
			unsigned delivs : 1;		/**< Delivery status (RO). */
			unsigned : 1;			/**< Reserved. */
			unsigned level : 1;		/**< Level. */
			unsigned trigger_mode : 1;	/**< Trigger Mode. */
			unsigned : 2;			/**< Reserved. */
			unsigned shorthand : 2;		/**< Destination Shorthand. */
			unsigned : 12;			/**< Reserved. */
		} __attribute__ ((packed));
	};
	union {
		uint32_t hi;
		struct {
			unsigned : 24;			/**< Reserved. */
			uint8_t dest;			/**< Destination field. */
		} __attribute__ ((packed));
	};
} __attribute__ ((packed));
typedef struct icr icr_t;

/* End Of Interrupt. */
#define EOI		(0x0b0/sizeof(uint32_t))

/** Error Status Register. */
#define ESR		(0x280/sizeof(uint32_t))
union esr {
	uint32_t value;
	uint8_t err_bitmap;
	struct {
		unsigned send_checksum_error : 1;
		unsigned receive_checksum_error : 1;
		unsigned send_accept_error : 1;
		unsigned receive_accept_error : 1;
		unsigned : 1;
		unsigned send_illegal_vector : 1;
		unsigned received_illegal_vector : 1;
		unsigned illegal_register_address : 1;
		unsigned : 24;
	} __attribute__ ((packed));
};
typedef union esr esr_t;

/* Task Priority Register */
#define TPR		(0x080/sizeof(uint32_t))
union tpr {
	uint32_t value;
	struct {
		unsigned pri_sc : 4;		/**< Task Priority Sub-Class. */
		unsigned pri : 4;		/**< Task Priority. */
	} __attribute__ ((packed));
};
typedef union tpr tpr_t;

/** Spurious-Interrupt Vector Register. */
#define SVR		(0x0f0/sizeof(uint32_t))
union svr {
	uint32_t value;
	struct {
		uint8_t vector;			/**< Spurious Vector. */
		unsigned lapic_enabled : 1;	/**< APIC Software Enable/Disable. */
		unsigned focus_checking : 1;	/**< Focus Processor Checking. */
		unsigned : 22;			/**< Reserved. */
	} __attribute__ ((packed));
};
typedef union svr svr_t;

/** Time Divide Configuration Register. */
#define TDCR		(0x3e0/sizeof(uint32_t))
union tdcr {
	uint32_t value;
	struct {
		unsigned div_value : 4;		/**< Divide Value, bit 2 is always 0. */
		unsigned : 28;			/**< Reserved. */
	} __attribute__ ((packed));
};
typedef union tdcr tdcr_t;

/* Initial Count Register for Timer */
#define ICRT		(0x380/sizeof(uint32_t))

/* Current Count Register for Timer */
#define CCRT		(0x390/sizeof(uint32_t))

/** LVT Timer register. */
#define LVT_Tm		(0x320/sizeof(uint32_t))
union lvt_tm {
	uint32_t value;
	struct {
		uint8_t vector;		/**< Local Timer Interrupt vector. */
		unsigned : 4;		/**< Reserved. */
		unsigned delivs : 1;	/**< Delivery status (RO). */
		unsigned : 3;		/**< Reserved. */
		unsigned masked : 1;	/**< Interrupt Mask. */
		unsigned mode : 1;	/**< Timer Mode. */
		unsigned : 14;		/**< Reserved. */
	} __attribute__ ((packed));
};
typedef union lvt_tm lvt_tm_t;

/** LVT LINT registers. */
#define LVT_LINT0	(0x350/sizeof(uint32_t))
#define LVT_LINT1	(0x360/sizeof(uint32_t))
union lvt_lint {
	uint32_t value;
	struct {
		uint8_t vector;			/**< LINT Interrupt vector. */
		unsigned delmod : 3;		/**< Delivery Mode. */
		unsigned : 1;			/**< Reserved. */
		unsigned delivs : 1;		/**< Delivery status (RO). */
		unsigned intpol : 1;		/**< Interrupt Input Pin Polarity. */
		unsigned irr : 1;		/**< Remote IRR (RO). */
		unsigned trigger_mode : 1;	/**< Trigger Mode. */
		unsigned masked : 1;		/**< Interrupt Mask. */
		unsigned : 15;			/**< Reserved. */
	} __attribute__ ((packed));
};
typedef union lvt_lint lvt_lint_t;

/** LVT Error register. */
#define LVT_Err		(0x370/sizeof(uint32_t))
union lvt_error {
	uint32_t value;
	struct {
		uint8_t vector;		/**< Local Timer Interrupt vector. */
		unsigned : 4;		/**< Reserved. */
		unsigned delivs : 1;	/**< Delivery status (RO). */
		unsigned : 3;		/**< Reserved. */
		unsigned masked : 1;	/**< Interrupt Mask. */
		unsigned : 15;		/**< Reserved. */
	} __attribute__ ((packed));
};
typedef union lvt_error lvt_error_t;

/** Local APIC ID Register. */
#define L_APIC_ID	(0x020/sizeof(uint32_t))
union l_apic_id {
	uint32_t value;
	struct {
		unsigned : 24;		/**< Reserved. */
		uint8_t apic_id;		/**< Local APIC ID. */
	} __attribute__ ((packed));
};
typedef union l_apic_id l_apic_id_t;

/** Local APIC Version Register */
#define LAVR		(0x030/sizeof(uint32_t))
#define LAVR_Mask	0xff
#define is_local_apic(x)	(((x)&LAVR_Mask&0xf0)==0x1)
#define is_82489DX_apic(x)	((((x)&LAVR_Mask&0xf0)==0x0))
#define is_local_xapic(x)	(((x)&LAVR_Mask)==0x14)

/** Logical Destination Register. */
#define  LDR		(0x0d0/sizeof(uint32_t))
union ldr {
	uint32_t value;
	struct {
		unsigned : 24;		/**< Reserved. */
		uint8_t id;		/**< Logical APIC ID. */
	} __attribute__ ((packed));
};
typedef union ldr ldr_t;

/** Destination Format Register. */
#define DFR		(0x0e0/sizeof(uint32_t))
union dfr {
	uint32_t value;
	struct {
		unsigned : 28;		/**< Reserved, all ones. */
		unsigned model : 4;	/**< Model. */
	} __attribute__ ((packed));
};
typedef union dfr dfr_t;

/* IO APIC */
#define IOREGSEL	(0x00/sizeof(uint32_t))
#define IOWIN		(0x10/sizeof(uint32_t))

#define IOAPICID	0x00
#define IOAPICVER	0x01
#define IOAPICARB	0x02
#define IOREDTBL	0x10

/** I/O Register Select Register. */
union io_regsel {
	uint32_t value;
	struct {
		uint8_t reg_addr;		/**< APIC Register Address. */
		unsigned : 24;		/**< Reserved. */
	} __attribute__ ((packed));
};
typedef union io_regsel io_regsel_t;

/** I/O Redirection Register. */
struct io_redirection_reg {
	union {
		uint32_t lo;
		struct {
			uint8_t intvec;			/**< Interrupt Vector. */
			unsigned delmod : 3;		/**< Delivery Mode. */
			unsigned destmod : 1; 		/**< Destination mode. */
			unsigned delivs : 1;		/**< Delivery status (RO). */
			unsigned intpol : 1;		/**< Interrupt Input Pin Polarity. */
			unsigned irr : 1;		/**< Remote IRR (RO). */
			unsigned trigger_mode : 1;	/**< Trigger Mode. */
			unsigned masked : 1;		/**< Interrupt Mask. */
			unsigned : 15;			/**< Reserved. */
		} __attribute__ ((packed));
	};
	union {
		uint32_t hi;
		struct {
			unsigned : 24;			/**< Reserved. */
			uint8_t dest : 8;			/**< Destination Field. */
		} __attribute__ ((packed));
	};
	
} __attribute__ ((packed));
typedef struct io_redirection_reg io_redirection_reg_t;


/** IO APIC Identification Register. */
union io_apic_id {
	uint32_t value;
	struct {
		unsigned : 24;		/**< Reserved. */
		unsigned apic_id : 4;	/**< IO APIC ID. */
		unsigned : 4;		/**< Reserved. */
	} __attribute__ ((packed));
};
typedef union io_apic_id io_apic_id_t;

extern volatile uint32_t *l_apic;
extern volatile uint32_t *io_apic;

extern uint32_t apic_id_mask;

extern void apic_init(void);

extern void l_apic_init(void);
extern void l_apic_eoi(void);
extern int l_apic_broadcast_custom_ipi(uint8_t vector);
extern int l_apic_send_init_ipi(uint8_t apicid);
extern void l_apic_debug(void);
extern uint8_t l_apic_id(void);

extern uint32_t io_apic_read(uint8_t address);
extern void io_apic_write(uint8_t address , uint32_t x);
extern void io_apic_change_ioredtbl(int pin, int dest, uint8_t v, int flags);
extern void io_apic_disable_irqs(uint16_t irqmask);
extern void io_apic_enable_irqs(uint16_t irqmask);

#endif

 /** @}
 */

