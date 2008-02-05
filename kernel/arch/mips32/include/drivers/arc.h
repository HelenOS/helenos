/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup mips32	
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_ARC_H_
#define KERN_mips32_ARC_H_

#include <arch/types.h>
#include <console/chardev.h>

#define ARC_BASE_ADDR 0x1000;
#define ARC_MAGIC 0x53435241
/* Frame size used by ARC */
#define ARC_FRAME 4096

typedef enum {
	CmResourceTypeNull = 0,
	CmResourceTypePort,
	CmResourceTypeInterrupt,
	CmResourceTypeMemory,
	CmResourceTypeDma,
	CmResourceTypeDeviceSpecific,
	CmResourceTypeVendor,
	CmResourceTypeProductName,
	CmResourceTypeSerialNumber
} cm_resource_type;

typedef struct {
	uint8_t type;
	uint8_t sharedisposition;
	uint16_t flags;
	union {
		struct {
			long long start; /* 64-bit phys address */
			unsigned long length;
		}port;
		struct {
			unsigned long level;
			unsigned long vector;
			unsigned long reserved1;
		}interrupt;
		struct {
			long long start; /* 64-bit phys address */
			unsigned long length;
		}memory;
	}u;
} __attribute__ ((packed)) cm_resource_descriptor;

typedef struct {
	uint16_t version;
	uint16_t revision;
	unsigned long count;
	cm_resource_descriptor descr[1];
} __attribute__ ((packed)) cm_resource_list;

typedef enum {
	SystemClass = 0,
	ProcessorClass,
	CacheClass,
	AdapterClass,
	ControllerClass,
	PeripheralClass,
	MemoryClass
} arc_component_class;

typedef enum {
	ARC_type = 0,
	CPU_type,
	FPU_type,
	PrimaryICache,
	PrimaryDCache,
	SecondaryICache,
	SecondaryDCache,
	SecondaryCache,
	Memory, /* Not in NT PROM */
	EISAAdapter,
	TCAdapter,
	SCSIAdapter,
	DTIAdapter,
	MultiFunctionAdapter,
	DiskController,
	TapeController,
	CDROMController,
	WORMController,
	SerialController,
	NetworkController,
	DisplayController,
	ParallelController,
	PointerController,
	KeyboardController,
	AudioController,
	OtherController,
	DiskPeripheral,
	FloppyDiskPeripheral,
	TapePeripheral,
	ModemPeripheral,
	MonitorPeripheral,
	PrinterPeripheral,
	PointerPeripheral,
	KeyboardPeripheral,
	TerminalPeripheral,
	LinePeripheral,
	NetworkPeripheral,
	OtherPeripheral,
	XTalkAdapter,
	PCIAdapter,
	GIOAdapter,
	TPUAdapter,
	Anonymous
} arc_component_type;

typedef enum {
	Failed = 1,
	ReadOnly = 2,
	Removable = 4,
	ConsoleIn = 8,
	ConsoleOut = 16,
	Input = 32,
	Output = 64
} arc_component_flags;

typedef struct  {
	arc_component_class class;
	arc_component_type type;
	arc_component_flags flags;
	uint16_t revision;
	uint16_t version;
	uint32_t key;
	uint32_t affinitymask;
	uint32_t configdatasize;
	uint32_t identifier_len;
	char *identifier;
} __attribute__ ((packed)) arc_component;

typedef struct {
	uint16_t year;
	uint16_t month;
	uint16_t day;
	uint16_t hour;
	uint16_t minutes;
	uint16_t seconds;
	uint16_t mseconds;
} __attribute__ ((packed)) arc_timeinfo;

/* This is the SGI block structure, WinNT has it different */
typedef enum {
	ExceptionBlock,
	SystemParameterBlock,
	FreeContiguous,
	FreeMemory,
	BadMemory,
	LoadedProgram,
	FirmwareTemporary,
	FirmwarePermanent
} arc_memorytype_t;

typedef struct  {
	arc_memorytype_t type;
	uint32_t basepage;  /* *4096 = baseaddr */
	uint32_t basecount;
} arc_memdescriptor_t;

typedef struct {
	char vendorid[8];
	char prodid[8];
} arc_sysid_t;

typedef struct {
	long (*load)(void); /* ... */
	long (*invoke)(uint32_t eaddr,uint32_t saddr,uint32_t argc,char **argv,
		       char **envp);
	long (*execute)(char *path,uint32_t argc,char **argv,char **envp);
	void (*halt)(void);
	void (*powerdown)(void);
	void (*restart)(void);
	void (*reboot)(void);
	void (*enterinteractivemode)(void);
	long (*reserved)(void);
/* 10 */	
	arc_component * (*getpeer)(arc_component *c); 
	arc_component * (*getchild)(arc_component *c);
	arc_component * (*getparent)(arc_component *c);
	long (*getconfigurationdata)(void *configdata, arc_component *c);
	long (*addchild)(arc_component *c, arc_component *template,
			 void *configdata);
	long (*deletecomponet)(arc_component *current);
	long (*getcomponent)(char *path);
	long (*saveconfiguration)(void);
	arc_sysid_t (*getsystemid)(void);
	arc_memdescriptor_t * (*getmemorydescriptor)(arc_memdescriptor_t *cur);
/* 20 */
	long (*reserved2)(void);
	arc_timeinfo * (*gettime)(void);
	uint32_t (*getrelativetime)(void);
	long (*getdirectoryentry)();
	long (*open)(void); /* ... */
	long (*close)(uint32_t fileid);
	long (*read)(uint32_t fileid,void *buf,uint32_t n,uint32_t *cnt);
	long (*getreadstatus)(uint32_t fileid);
	long (*write)(uint32_t fileid, void *buf,uint32_t n,uint32_t *cnt);
	long (*seek)(void); /* ... */
/* 30 */
	long (*mount)(void); /* ... */
	char * (*getenvironmentvariable)(char *name);
	char * (*setenvironmentvariable)(char *name, char *value);
	long (*getfileinformation)(void); /* ... */
	long (*setfileinformation)(uint32_t fileid,uint32_t attflags,uint32_t attmask);
	void (*flushallcaches)(void);
	long (*testunicodecharacter)(void); /* ... */
	long (*getdisplaystatus)(void); /* ... */
} arc_func_vector_t;

typedef struct {
	uint32_t signature;
	uint32_t length;
	uint16_t version;
	uint16_t revision;
	void *restartblock;
	void *debugblock;
	void *gevector;
	void *utlbmissvector;
	uint32_t firmwarevectorlen;
	arc_func_vector_t *firmwarevector;
	uint32_t privvectorlen;
	void *privvector;
	uint32_t adaptercount;
} __attribute__ ((packed)) arc_sbp;

extern int arc_init(void);
extern int arc_reboot(void);
extern int arc_frame_init(void);
extern int arc_console(void);

#endif

 /** @}
 */
