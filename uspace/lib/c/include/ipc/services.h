/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/**
 * @file  services.h
 * @brief List of all known services and their codes.
 */

#ifndef _LIBC_SERVICES_H_
#define _LIBC_SERVICES_H_

#include <abi/fourcc.h>

typedef enum {
	SERVICE_NONE       = 0,
	SERVICE_LOADER     = FOURCC('l', 'o', 'a', 'd'),
	SERVICE_VFS        = FOURCC('v', 'f', 's', ' '),
	SERVICE_LOC        = FOURCC('l', 'o', 'c', ' '),
	SERVICE_LOGGER     = FOURCC('l', 'o', 'g', 'g'),
	SERVICE_DEVMAN     = FOURCC('d', 'e', 'v', 'n'),
} service_t;

#define SERVICE_NAME_CHARDEV_TEST_SMALLX "chardev-test/smallx"
#define SERVICE_NAME_CHARDEV_TEST_LARGEX "chardev-test/largex"
#define SERVICE_NAME_CHARDEV_TEST_PARTIALX "chardev-test/partialx"
#define SERVICE_NAME_CLIPBOARD "clipboard"
#define SERVICE_NAME_CORECFG  "corecfg"
#define SERVICE_NAME_DISPLAY  "hid/display"
#define SERVICE_NAME_DHCP     "net/dhcp"
#define SERVICE_NAME_DNSR     "net/dnsr"
#define SERVICE_NAME_INET     "net/inet"
#define SERVICE_NAME_IPC_TEST "ipc-test"
#define SERVICE_NAME_NETCONF  "net/netconf"
#define SERVICE_NAME_UDP      "net/udp"
#define SERVICE_NAME_TCP      "net/tcp"
#define SERVICE_NAME_VBD      "vbd"
#define SERVICE_NAME_VOLSRV   "volsrv"

#endif

/** @}
 */
