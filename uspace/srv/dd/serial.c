#include <unistd.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/serial.h>
#include <ipc/devmap.h>
#include <ipc/ns.h>
#include <bool.h>
#include <errno.h>
#include <async.h>
#include <stdio.h>
#include <futex.h>
#include <assert.h>
#include <adt/list.h>
#include <string.h>

#include "isa.h"
#include "serial.h"
#include "pic.h"

#define NAME "serial"

#define REG_COUNT 7

#define MAX_NAME_LEN 8
#define BUF_LEN 256  // the length of the input buffer

struct cyclic_buffer {
	uint8_t buf[BUF_LEN];  // cyclic buffer 
	int start;
	int cnt;
};

typedef struct cyclic_buffer cyclic_buffer_t;

// returns false if the buffer is full
static bool buf_push_back(cyclic_buffer_t *buf, uint8_t item) 
{
	if (buf->cnt >= BUF_LEN) {
		return false;
	}
	
	int pos = (buf->start + buf->cnt) % BUF_LEN;
	buf->buf[pos] = item;
	buf->cnt++;
	return true;
}

static bool buf_is_empty(cyclic_buffer_t *buf) 
{
	return buf->cnt == 0;
}

// call it on non empty buffer!
static uint8_t buf_pop_front(cyclic_buffer_t *buf) 
{
	assert(!buf_is_empty(buf));
	
	uint8_t res = buf->buf[buf->start];
	buf->start = (buf->start + 1) % BUF_LEN;	
	buf->cnt--;
	return res;
}

static void buf_clear(cyclic_buffer_t *buf) 
{
	buf->cnt = 0;
}

struct serial_dev {
	link_t link;
	char name[MAX_NAME_LEN];
	int handle; // devmapper device handle
	int devno; // unique device number; used in irq registration (cannot be handle used instead of it?) 
	bool client_connected;
	ioport8_t *port;
	void *phys_addr;
	int irq;
	bridge_to_isa_t *parent;
	cyclic_buffer_t input_buffer;
	futex_t futex;
};

typedef struct serial_dev serial_dev_t;

static irq_cmd_t serial_cmds[] = {
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t serial_pseudocode = {
	sizeof(serial_cmds) / sizeof(irq_cmd_t),
	serial_cmds
};

static void * serial_phys_addresses[] = { (void *)0x3F8, (void *)0x2F8 };
static int serial_irqs[] = { 4, 3 }; // TODO - what about if there were more than two serial ports?
static int serial_phys_addr_cnt = sizeof(serial_phys_addresses)/sizeof(void *);
static int serial_irq_cnt = sizeof(serial_irqs)/sizeof(int);

// number, which should be assigned to a newly found serial device - increment first, then assign to the device
static int serial_idx = 0; 

static int serial_driver_phone = -1;

LIST_INITIALIZE(serial_devices_list);

static atomic_t serial_futex = FUTEX_INITIALIZER;


static void serial_init_port(ioport8_t *port);
static void serial_write_8(ioport8_t *port, uint8_t c);
static bool is_transmit_empty(ioport8_t *port);
static uint8_t serial_read_8(ioport8_t *port);
static bool serial_received(ioport8_t *port);
static void serial_probe(bridge_to_isa_t *parent);
static ioport8_t * serial_probe_port(void *phys_addr);
static int serial_device_register(int driver_phone, char *name, int *handle);
static int serial_driver_register(char *name);
static void serial_putchar(serial_dev_t *dev, ipc_callid_t rid, ipc_call_t *request);
static void serial_getchar(serial_dev_t *dev, ipc_callid_t rid);
static void serial_client_conn(ipc_callid_t iid, ipc_call_t *icall);
static void serial_irq_handler(ipc_callid_t iid, ipc_call_t *icall);
static serial_dev_t * serial_devno_to_dev(int devno);
static serial_dev_t * serial_handle_to_dev(int handle);
static void serial_enable_interrupt(serial_dev_t *dev);

static isa_drv_ops_t serial_isa_ops = {
	.probe = serial_probe	
};

static isa_drv_t serial_isa_drv = {
	.name = NAME,
	.ops = &serial_isa_ops	
};

int serial_init()
{
	// register driver by devmapper
	serial_driver_phone = serial_driver_register(NAME);
    if (serial_driver_phone < 0) {
		printf(NAME ": Unable to register driver\n");
		return 0;
	}
	
	// register irq handler
	printf(NAME ": Registering interrup notification callback function.\n");
	async_set_interrupt_received(serial_irq_handler);
	   
	// register this driver by generic isa bus driver	
	isa_register_driver(&serial_isa_drv);
	return 1;
}

static bool serial_received(ioport8_t *port) 
{
   return (pio_read_8(port + 5) & 1) != 0;
}

static uint8_t serial_read_8(ioport8_t *port) 
{
	return pio_read_8(port);
}

static bool is_transmit_empty(ioport8_t *port) 
{
   return (pio_read_8(port + 5) & 0x20) != 0;
}

static void serial_write_8(ioport8_t *port, uint8_t c) 
{
	while (!is_transmit_empty(port)) 
		;
	
	pio_write_8(port, c);
}

static void serial_init_port(ioport8_t *port) 
{	
	pio_write_8(port + 1, 0x00);    // Disable all interrupts
	pio_write_8(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	pio_write_8(port + 0, 0x60);    // Set divisor to 96 (lo byte) 1200 baud
	pio_write_8(port + 1, 0x00);    //                   (hi byte)
	pio_write_8(port + 3, 0x07);    // 8 bits, no parity, two stop bits
	pio_write_8(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	pio_write_8(port + 4, 0x0B);    // RTS/DSR set (Request to Send and Data Terminal Ready lines enabled), 
									// Aux Output2 set - needed for interrupts
	 
}

static void serial_enable_interrupt(serial_dev_t *dev)
{
	futex_down(&(dev->futex));
	// TODO do not call pic directly, do it more generally
	pic_enable_interrupt(dev->irq);
	pio_write_8(dev->port + 1 , 0x01);   // Interrupt when data received
	pio_write_8(dev->port + 4, 0x0B);
	futex_up(&(dev->futex));
}

static serial_dev_t * serial_alloc_dev()
{
	serial_dev_t *dev = (serial_dev_t *)malloc(sizeof(serial_dev_t));
	memset(dev, 0, sizeof(serial_dev_t));
	return dev;
}

static void serial_init_dev(serial_dev_t *dev, bridge_to_isa_t *parent, int idx)
{
	assert(dev != NULL);
	
	memset(dev, 0, sizeof(serial_dev_t));
	dev->parent = parent;
	dev->phys_addr = dev->parent->ops->absolutize(serial_phys_addresses[idx % serial_phys_addr_cnt]);
	dev->irq = serial_irqs[idx % serial_irq_cnt];
	snprintf(dev->name, MAX_NAME_LEN, "com%d", idx + 1);
	dev->devno = -1;
	dev->client_connected = false;
	futex_initialize(&(dev->futex), 1);
}

static bool serial_probe_dev(serial_dev_t *dev)
{
	assert(dev != NULL);
	
	return (dev->port = (ioport8_t *)serial_probe_port(dev->phys_addr)) != NULL;	
}

static void serial_delete_dev(serial_dev_t *dev) {
	free(dev);
}

static void serial_probe(bridge_to_isa_t *parent)
{
	printf(NAME " driver: probe()\n");	
	
	serial_dev_t *dev = serial_alloc_dev();
	
	int i;
	for (i = 0; i < serial_phys_addr_cnt; i++) {			
		serial_init_dev(dev, parent, serial_idx);
		printf(NAME ": probing %s. \n", dev->name);
		if (serial_probe_dev(dev)) {
			printf(NAME " driver: initializing %s.\n", dev->name);
			serial_init_port(dev->port);
			if (EOK != serial_device_register(serial_driver_phone, dev->name, &(dev->handle))) {
				printf(NAME ": unable to register device %s\n", dev->name);
			} else {
				dev->devno = device_assign_devno();
				
				// 3rd argument called method is equal devno,this enables us to identify the device 
				// which caused the interrupt in the irq notification callback function
				printf(NAME ": registering irq = %d for %s.\n", dev->irq, dev->name);
				ipc_register_irq(dev->irq, dev->devno, dev->devno, &serial_pseudocode);  
				list_append(&(dev->link), &serial_devices_list);
				printf(NAME ": enabling irq = %d for %s.\n", dev->irq, dev->name);
				serial_enable_interrupt(dev);
				dev = serial_alloc_dev();
			}
		} else {
			printf(NAME " driver: %s is not present \n", dev->name);
		}
		serial_idx++;
	}
	
	serial_delete_dev(dev);
}

// returns virtual address of the serial port, if the serial port is present at this physical address, NULL otherwise  
static ioport8_t * serial_probe_port(void *phys_addr)
{
	ioport8_t *port_addr = NULL;
	
	if (pio_enable(phys_addr, REG_COUNT, (void **)(&port_addr))) {  // Gain control over port's registers.
		printf(NAME ": Error - cannot gain the port %lx.\n", phys_addr);
		return NULL;
	}
	
	uint8_t olddata;
	
	olddata = pio_read_8(port_addr + 4);
	pio_write_8(port_addr + 4, 0x10);
	if ((pio_read_8(port_addr + 6) & 0xf0)) {
		return NULL;
	}
	
	pio_write_8(port_addr + 4, 0x1f);
	if ((pio_read_8(port_addr + 6) & 0xf0) != 0xf0) {
		return NULL;
	}
	pio_write_8(port_addr + 4, olddata);
	
	return port_addr;
}


static void serial_putchar(serial_dev_t *dev, ipc_callid_t rid, ipc_call_t *request)
{
	int c = IPC_GET_ARG1(*request);
	futex_down(&(dev->futex));
	serial_write_8(dev->port, (uint8_t)c);	
	futex_up(&(dev->futex));
	ipc_answer_0(rid, EOK);
}

static void serial_getchar(serial_dev_t *dev, ipc_callid_t rid)
{
	uint8_t c;
	printf(NAME ": trying to read from serial port %s\n", dev->name);
	while (true) {  // TODO: fix it - the queue of requests to read ?
		futex_down(&(dev->futex));
		if (!buf_is_empty(&(dev->input_buffer))) {
			c = buf_pop_front(&(dev->input_buffer));
			futex_up(&(dev->futex));
			break;
		}
		//printf(NAME "no data ready, trying to read again after some while.\n", dev->name);
		futex_up(&(dev->futex));
		async_usleep(10000);
	}
	printf(NAME ": serial_getchar: sending characer %c read from %s to client.\n", c, dev->name);
	ipc_answer_1(rid, EOK, c);		
}

static serial_dev_t * serial_handle_to_dev(int handle) 
{	
	futex_down(&serial_futex);	
	
	link_t *item = serial_devices_list.next;
	serial_dev_t *dev = NULL; 
	
	while (item != &serial_devices_list) {
		dev = list_get_instance(item, serial_dev_t, link);
		if (dev->handle == handle) {
			futex_up(&serial_futex);
			return dev;
		}
		item = item->next;
	}
	
	futex_up(&serial_futex);
	return NULL;
}

static serial_dev_t * serial_devno_to_dev(int devno)
{		
	futex_down(&serial_futex);	
	
	link_t *item = serial_devices_list.next;
	serial_dev_t *dev = NULL; 
	
	while (item != &serial_devices_list) {
		dev = list_get_instance(item, serial_dev_t, link);
		if (dev->devno == devno) {
			futex_up(&serial_futex);
			return dev;
		}
		item = item->next;
	}
	
	futex_up(&serial_futex);
	return NULL;
}


/** Handle one connection to the driver.
 *
 * @param iid		Hash of the request that opened the connection.
 * @param icall		Call data of the request that opened the connection.
 */
static void serial_client_conn(ipc_callid_t iid, ipc_call_t *icall)
{	
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call and remember the handle of the device to which the client connected.
	 */
	int handle = IPC_GET_ARG1(*icall); 
	serial_dev_t *dev = serial_handle_to_dev(handle);
	
	if (dev == NULL) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	if (dev->client_connected) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
		
	buf_clear(&(dev->input_buffer));  // synchronization with interrupt service routine ?
	dev->client_connected = true;
	ipc_answer_0(iid, EOK);
	
	while (1) {
		ipc_callid_t callid;
		ipc_call_t call;
	
		callid = async_get_call(&call);
		switch  (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			/*
			 * The other side has hung up.
			 * Answer the message and exit the fibril.
			 */
			ipc_answer_0(callid, EOK);
			dev->client_connected = false;
			return;
			break;
		case SERIAL_GETCHAR:
			serial_getchar(dev, callid);
			break;
		case SERIAL_PUTCHAR:
			serial_putchar(dev, callid, &call);
			break;
		default:
			ipc_answer_0(callid, ENOTSUP);
			break;
		}
	}	
}


/**
 *  Register the driver with the given name and return its newly created phone.
 */ 
static int serial_driver_register(char *name)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;
	int phone;
	ipcarg_t callback_phonehash;

	phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP, DEVMAP_DRIVER, 0);

	while (phone < 0) {
		usleep(10000);
		phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP, DEVMAP_DRIVER, 0);
	}
	
	req = async_send_2(phone, DEVMAP_DRIVER_REGISTER, 0, 0, &answer);

	retval = ipc_data_write_start(phone, (char *) name, str_length(name) + 1); 

	if (retval != EOK) {
		async_wait_for(req, NULL);
		return -1;
	}

	async_set_client_connection(serial_client_conn);  // set callback function which will serve client connections

	ipc_connect_to_me(phone, 0, 0, 0, &callback_phonehash);
	async_wait_for(req, &retval);

	return phone;
}

static int serial_device_register(int driver_phone, char *name, int *handle)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;
	
	if (handle != NULL) {
		*handle = -1;
	}

	req = async_send_2(driver_phone, DEVMAP_DEVICE_REGISTER, 0, 0, &answer);

	retval = ipc_data_write_start(driver_phone, (char *) name, str_length(name) + 1); 

	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}

	async_wait_for(req, &retval);
	
	if (EOK == retval) {
		if (NULL != handle)
			*handle = (int) IPC_GET_ARG1(answer);
	}
	
	return retval;
}

static void serial_read_from_device(serial_dev_t *dev)
{	
	bool cont = true;
	
	while (cont) {	
		futex_down(&(dev->futex));
		if (cont = serial_received(dev->port)) {
			uint8_t val = serial_read_8(dev->port);
			printf(NAME ": character %c read from %s.\n", val, dev->name);
			if (dev->client_connected) {
				if (!buf_push_back(&(dev->input_buffer), val)) {
					printf(NAME ": buffer overflow on %s.\n", dev->name);
				} else {
					printf(NAME ": the character %c saved to the buffer of %s.\n", val, dev->name);
				}
			} else {
				printf(NAME ": no client is connected to %s, discarding the character which was read.\n", dev->name);
			}
		}
		futex_up(&(dev->futex));
		usleep(10000);
	}	
}

// TODO - this is the only irq handling function registered by this task.
// (A task can register only one such function.)
// If more drivers within the task needed to handle interrupts, 
// there will have to be one generic handler redistributing
// irq notification messages among them. 
// Or the change of ansync framework will be needed.
static void serial_irq_handler(ipc_callid_t iid, ipc_call_t *icall)
{
	printf(NAME ": irq handler\n");
	int devno = IPC_GET_METHOD(*icall);
	serial_dev_t *dev = serial_devno_to_dev(devno);
	serial_read_from_device(dev);	
}

