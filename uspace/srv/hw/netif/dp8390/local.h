/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  Network interface probe functions.
 */

#ifndef __NET_NETIF_DP8390_CONFIG_H__
#define __NET_NETIF_DP8390_CONFIG_H__

#include "dp8390_port.h"

struct dpeth;

int ne_probe(struct dpeth * dep);

#endif

/** @}
 */
