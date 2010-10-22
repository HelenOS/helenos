/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief USB Host Controller Driver.
 */
#ifndef LIBUSB_HCD_H_
#define LIBUSB_HCD_H_

#include "usb.h"

#include <ipc/ipc.h>
#include <async.h>

/** Maximum size of transaction payload. */
#define USB_MAX_PAYLOAD_SIZE 1020

/** Opaque handle of active USB transaction.
 * This handle is when informing about transaction outcome (or status).
 */
typedef ipcarg_t usb_transaction_handle_t;

/** USB transaction outcome. */
typedef enum {
	USB_OUTCOME_OK,
	USB_OUTCOME_CRCERROR,
	USB_OUTCOME_BABBLE
} usb_transaction_outcome_t;

/** USB packet identifier. */
typedef enum {
#define _MAKE_PID_NIBBLE(tag, type) \
	((uint8_t)(((tag) << 2) | (type)))
#define _MAKE_PID(tag, type) \
	( \
	    _MAKE_PID_NIBBLE(tag, type) \
	    | ((~_MAKE_PID_NIBBLE(tag, type)) << 4) \
	)
	USB_PID_OUT = _MAKE_PID(0, 1),
	USB_PID_IN = _MAKE_PID(2, 1),
	USB_PID_SOF = _MAKE_PID(1, 1),
	USB_PID_SETUP = _MAKE_PID(3, 1),
	
	USB_PID_DATA0 = _MAKE_PID(0 ,3),
	USB_PID_DATA1 = _MAKE_PID(2 ,3),
	
	USB_PID_ACK = _MAKE_PID(0 ,2),
	USB_PID_NAK = _MAKE_PID(2 ,2),
	USB_PID_STALL = _MAKE_PID(3 ,2),
	
	USB_PID_PRE = _MAKE_PID(3 ,0),
	/* USB_PID_ = _MAKE_PID( ,), */
#undef _MAKE_PID
#undef _MAKE_PID_NIBBLE
} usb_packet_id;

const char * usb_str_transaction_outcome(usb_transaction_outcome_t o);

/** IPC methods for HCD. */
typedef enum {
	/** Send data over USB to a function.
	 * This method initializes large data transfer that must follow
	 * immediatelly.
	 * The recipient of this method must issue immediately data reception
	 * and answer this call after data buffer was transfered.
	 * 
	 * Arguments of the call:
	 * - USB address of the function
	 * - endpoint of the function
	 * - transfer type
	 * - flags (not used)
	 * 
	 * Answer:
	 * - EOK - ready to accept the data buffer
	 * - ELIMIT - too many transactions for current connection
	 * - ENOENT - callback connection does not exist
	 * - EINVAL - other kind of error
	 * 
	 * Arguments of the answer:
	 * - opaque transaction handle (used in callbacks)
	 */
	IPC_M_USB_HCD_SEND_DATA = IPC_FIRST_USER_METHOD,
	
	/** Initiate data receive from a function.
	 * This method announces the HCD that some data will come.
	 * When this data arrives, the HCD will call back with
	 * IPC_M_USB_HCD_DATA_RECEIVED.
	 * 
	 * Arguments of the call:
	 * - USB address of the function
	 * - endpoint of the function
	 * - transfer type
	 * - buffer size
	 * - flags (not used)
	 *
	 * Answer:
	 * - EOK - HCD accepted the request
	 * - ELIMIT - too many transactions for current connection
	 * - ENOENT - callback connection does not exist
	 *
	 * Arguments of the answer:
	 * - opaque transaction handle (used in callbacks)
	 */
	IPC_M_USB_HCD_RECEIVE_DATA,
	
	/** Tell maximum size of the transaction buffer (payload).
	 * 
	 * Arguments of the call:
	 *  (none)
	 * 
	 * Answer:
	 * - EOK - always
	 * 
	 * Arguments of the answer:
	 * - buffer size (in bytes):
	 */
	IPC_M_USB_HCD_TRANSACTION_SIZE,
	
	
	IPC_M_USB_HCD_INTERRUPT_OUT,
	IPC_M_USB_HCD_INTERRUPT_IN,
	
	IPC_M_USB_HCD_CONTROL_WRITE_SETUP,
	IPC_M_USB_HCD_CONTROL_WRITE_DATA,
	IPC_M_USB_HCD_CONTROL_WRITE_STATUS,
	
	IPC_M_USB_HCD_CONTROL_READ_SETUP,
	IPC_M_USB_HCD_CONTROL_READ_DATA,
	IPC_M_USB_HCD_CONTROL_READ_STATUS,
	/* IPC_M_USB_HCD_ */
} usb_hcd_method_t;

/** IPC methods for callbacks from HCD. */
typedef enum {
	/** Confimation after data sent.
	 * 
	 * Arguments of the call:
	 * - transaction handle
	 * - transaction outcome
	 */
	IPC_M_USB_HCD_DATA_SENT = IPC_FIRST_USER_METHOD,
	
	/** Notification of data received.
	 * This call initiates sending a data buffer from HCD to the client.
	 * See IPC_M_USB_HCD_SEND_DATA for details for buffer transfer is
	 * done.
	 * 
	 * Arguments of the call:
	 * - transaction handle
	 * - transaction outcome
	 * - actual data length
	 */
	IPC_M_USB_HCD_DATA_RECEIVED,
	
	/** Notification about a serious trouble with HC.
	 */
	IPC_M_USB_HCD_CONTROLLER_FAILURE,
	
	/* IPC_M_USB_HCD_ */
} usb_hcd_callback_method_t;


int usb_hcd_create_phones(const char *, async_client_conn_t);
int usb_hcd_send_data_to_function(int, usb_target_t, usb_transfer_type_t,
    void *, size_t, usb_transaction_handle_t *);
int usb_hcd_prepare_data_reception(int, usb_target_t, usb_transfer_type_t,
    size_t, usb_transaction_handle_t *);


int usb_hcd_transfer_interrupt_out(int, usb_target_t,
    void *, size_t, usb_transaction_handle_t *);
int usb_hcd_transfer_interrupt_in(int, usb_target_t,
    size_t, usb_transaction_handle_t *);

int usb_hcd_transfer_control_write_setup(int, usb_target_t,
    void *, size_t, usb_transaction_handle_t *);
int usb_hcd_transfer_control_write_data(int, usb_target_t,
    void *, size_t, usb_transaction_handle_t *);
int usb_hcd_transfer_control_write_status(int, usb_target_t,
    usb_transaction_handle_t *);

int usb_hcd_transfer_control_read_setup(int, usb_target_t,
    void *, size_t, usb_transaction_handle_t *);
int usb_hcd_transfer_control_read_data(int, usb_target_t,
    size_t, usb_transaction_handle_t *);
int usb_hcd_transfer_control_read_status(int, usb_target_t,
    usb_transaction_handle_t *);

#endif
/**
 * @}
 */
