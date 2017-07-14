/*
 * Copyright (c) 2017 Michal Staruch
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The roothub structures abstraction.
 */
 
#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include "debug.h"
#include "hc.h"
#include "rh.h"

#include "hw_struct/trb.h"

static int handle_connected_device(xhci_hc_t* hc, xhci_port_regs_t* regs, uint8_t port_id) {
    uint8_t link_state = XHCI_REG_RD(regs, XHCI_PORT_PLS);
    if(link_state == 0) {
        // USB3 is automatically advance to enabled   
        uint8_t port_speed = XHCI_REG_RD(regs, XHCI_PORT_PS);
        usb_log_debug2("Detected new device on port %u, port speed id %u.", port_id, port_speed);
        // TODO: Assign device slot (specification 4.3.2) 
    }
    else if(link_state == 5) {
        // USB 3 failed to enable
        usb_log_debug("USB 3 port couldn't be enabled.");
    }
    else if(link_state == 7) {
        usb_log_debug("USB 2 device attached, issuing reset.");
        xhci_reset_hub_port(hc, port_id);
    }

    return EOK;
}

int xhci_handle_port_status_change_event(xhci_hc_t *hc, xhci_trb_t *trb) {
    uint8_t port_id = xhci_get_hub_port(trb);
    usb_log_debug("Port status change event detected for port %u.", port_id);
    xhci_port_regs_t* regs = &hc->op_regs->portrs[port_id];

    // Port reset change
    if(XHCI_REG_RD(regs, XHCI_PORT_PRC)) {
        // Clear the flag
        XHCI_REG_WR(regs, XHCI_PORT_PRC, 1);

        uint8_t port_speed = XHCI_REG_RD(regs, XHCI_PORT_PS);
        usb_log_debug2("Detected port reset on port %u, port speed id %u.", port_id, port_speed);
        // TODO: Assign device slot (specification 4.3.2) 
    }

    // Connection status change
    if(XHCI_REG_RD(regs, XHCI_PORT_CSC)) {
        XHCI_REG_WR(regs, XHCI_PORT_CSC, 1);

        if(XHCI_REG_RD(regs, XHCI_PORT_CCS) == 1) {
            handle_connected_device(hc, regs, port_id);
        }
        else {
            // Device disconnected
        }
    }
    return EOK;
}

int xhci_get_hub_port(xhci_trb_t *trb) {
    assert(trb);
    uint8_t port_id = XHCI_QWORD_EXTRACT(trb->parameter, 63, 56);
    return port_id;   
}

int xhci_reset_hub_port(xhci_hc_t* hc, uint8_t port) {
    usb_log_debug2("Resetting port %u.", port);
    xhci_port_regs_t regs = hc->op_regs->portrs[port];
    XHCI_REG_WR(&regs, XHCI_PORT_PR, 1);
    return EOK;
}

