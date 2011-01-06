/** @addtogroup ne2k
 *  @{
 */

/** @file
 *  NE1000 and NE2000 network interface definitions.
 */

#ifndef __NET_NETIF_NE2000_H__
#define __NET_NETIF_NE2000_H__

#include <libarch/ddi.h>
#include "dp8390_port.h"

/** DP8390 register offset.
 */
#define NE_DP8390  0x00

/** Data register.
 */
#define NE_DATA  0x10

/** Reset register.
 */
#define NE_RESET  0x1f

/** NE1000 data start.
 */
#define NE1000_START  0x2000

/** NE1000 data size.
 */
#define NE1000_SIZE  0x2000

/** NE2000 data start.
 */
#define NE2000_START  0x4000

/** NE2000 data size.
 */
#define NE2000_SIZE  0x4000

/** Reads 1 byte register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @returns The read value.
 */
#define inb_ne(dep, reg)  (inb(dep->de_base_port + reg))

/** Writes 1 byte register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @param[in] data The value to be written.
 */
#define outb_ne(dep, reg, data)  (outb(dep->de_base_port + reg, data))

/** Reads 1 word (2 bytes) register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @returns The read value.
 */
#define inw_ne(dep, reg)  (inw(dep->de_base_port + reg))

/** Writes 1 word (2 bytes) register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @param[in] data The value to be written.
 */
#define outw_ne(dep, reg, data)  (outw(dep->de_base_port + reg, data))

struct dpeth;

int ne_probe(struct dpeth *dep);

#endif

/** @}
 */
