/*
 * Copyright (c) 2015 Jan Kolarik
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

/** @addtogroup libnet
 *  @{
 */

/** @file ieee80211_impl.c
 * 
 * IEEE 802.11 default functions implementation.
 */

#include <errno.h>

#include <ieee80211_impl.h>

static int ieee80211_freq_to_channel(uint16_t freq)
{
	return (freq - IEEE80211_FIRST_FREQ) / IEEE80211_CHANNEL_GAP + 1;
}

static int ieee80211_probe_request(ieee80211_dev_t *ieee80211_dev)
{
	size_t buffer_size = sizeof(ieee80211_header_t);
	void *buffer = malloc(buffer_size);
	
	/* TODO */
	
	ieee80211_freq_to_channel(ieee80211_dev->current_freq);
	
	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);
	
	free(buffer);
	
	return EOK;
}

/**
 * Default implementation of IEEE802.11 scan function.
 * 
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * 
 * @return EOK if succeed, negative error code otherwise. 
 */
int ieee80211_scan_impl(ieee80211_dev_t *ieee80211_dev)
{
	/* TODO */
	int rc = ieee80211_probe_request(ieee80211_dev);
	if(rc != EOK)
		return rc;
	
	return EOK;
}