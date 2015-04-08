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

/** @addtogroup libieee80211
 * @{
 */

/** @file ieee80211_impl.c
 * 
 * IEEE 802.11 default device functions implementation.
 */

#include <stdio.h>
#include <crypto.h>
#include <stdlib.h>
#include <errno.h>

#include <ieee80211_impl.h>

/**
 * Default implementation of IEEE802.11 start function.
 * 
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * 
 * @return EOK. 
 */
int ieee80211_start_impl(ieee80211_dev_t *ieee80211_dev)
{
	return EOK;
}

/**
 * Default implementation of IEEE802.11 TX handler function.
 * 
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * @param buffer Buffer with data to send.
 * @param buffer_size Size of buffer.
 * 
 * @return EOK. 
 */
int ieee80211_tx_handler_impl(ieee80211_dev_t *ieee80211_dev, void *buffer, 
	size_t buffer_size)
{
	return EOK;
}

/**
 * Default implementation of IEEE802.11 set frequency function.
 * 
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * @param freq Value of frequency to be switched on.
 * 
 * @return EOK. 
 */
int ieee80211_set_freq_impl(ieee80211_dev_t *ieee80211_dev, uint16_t freq)
{
	return EOK;
}

/**
 * Default implementation of IEEE802.11 BSSID change function.
 * 
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * 
 * @return EOK. 
 */
int ieee80211_bssid_change_impl(ieee80211_dev_t *ieee80211_dev)
{
	return EOK;
}

/**
 * Default implementation of IEEE802.11 key config function.
 * 
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * 
 * @return EOK. 
 */
int ieee80211_key_config_impl(ieee80211_dev_t *ieee80211_dev,
	ieee80211_key_config_t *key_conf, bool insert)
{
	return EOK;
}

/**
 * Default implementation of IEEE802.11 scan function.
 * 
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * @param clear Whether to clear current scan results.
 * 
 * @return EOK if succeed, negative error code otherwise. 
 */
int ieee80211_scan_impl(ieee80211_dev_t *ieee80211_dev)
{
	if(ieee80211_is_connected(ieee80211_dev))
		return EOK;
	
	fibril_mutex_lock(&ieee80211_dev->ap_list.scan_mutex);
	
	/* Remove old entries we don't receive beacons from. */
	ieee80211_scan_result_list_t *result_list = 
		&ieee80211_dev->ap_list;
	list_foreach_safe(result_list->list, cur_link, next_link) {
		ieee80211_scan_result_link_t *cur_result = 
			list_get_instance(cur_link,
			ieee80211_scan_result_link_t, 
			link);
		if((time(NULL) - cur_result->last_beacon) > 
			MAX_KEEP_SCAN_SPAN_SEC) {
			ieee80211_scan_result_list_remove(result_list, 
				cur_result);
		}
	}
	
	fibril_mutex_unlock(&ieee80211_dev->ap_list.scan_mutex);
	
	uint16_t orig_freq = ieee80211_dev->current_freq;
	
	for(uint16_t freq = IEEE80211_FIRST_FREQ;
		freq <= IEEE80211_MAX_FREQ; 
		freq += IEEE80211_CHANNEL_GAP) {
		ieee80211_dev->ops->set_freq(ieee80211_dev, freq);
		ieee80211_probe_request(ieee80211_dev, NULL);
		
		/* Wait for probe responses. */
		usleep(100000);
	}
	
	ieee80211_dev->ops->set_freq(ieee80211_dev, orig_freq);
	
	fibril_mutex_lock(&ieee80211_dev->ap_list.scan_mutex);
	time(&ieee80211_dev->ap_list.last_scan);
	fibril_mutex_unlock(&ieee80211_dev->ap_list.scan_mutex);
	
	return EOK;
}

/**
 * Pseudorandom function used for IEEE 802.11 pairwise key computation.
 * 
 * @param key Key with PBKDF2 encrypted passphrase.
 * @param data Concatenated sequence of both mac addresses and nonces.
 * @param hash Output parameter for result hash (48 byte value).
 * @param hash_sel Hash function selector.
 * 
 * @return EINVAL when key or data not specified, ENOMEM when pointer for 
 * output hash result is not allocated, otherwise EOK. 
 */
int ieee80211_prf(uint8_t *key, uint8_t *data, uint8_t *hash, 
	hash_func_t hash_sel)
{
	if(!key || !data)
		return EINVAL;
	
	if(!hash)
		return ENOMEM;
	
	size_t result_length = (hash_sel == HASH_MD5) ? 
		IEEE80211_PTK_TKIP_LENGTH : IEEE80211_PTK_CCMP_LENGTH;
	size_t iters = ((result_length * 8) + 159) / 160;
	
	const char *a = "Pairwise key expansion";
	uint8_t result[hash_sel*iters];
	uint8_t temp[hash_sel];
	size_t data_size = PRF_CRYPT_DATA_LENGTH + str_size(a) + 2;
	uint8_t work_arr[data_size];
	memset(work_arr, 0, data_size);
	
	memcpy(work_arr, a, str_size(a));
	memcpy(work_arr + str_size(a) + 1, data, PRF_CRYPT_DATA_LENGTH);

	for(uint8_t i = 0; i < iters; i++) {
		memcpy(work_arr + data_size - 1, &i, 1);
		hmac(key, PBKDF2_KEY_LENGTH, work_arr, data_size, temp, 
			hash_sel);
		memcpy(result + i*hash_sel, temp, hash_sel);
	}
	
	memcpy(hash, result, result_length);
	
	return EOK;
}

int ieee80211_aes_key_unwrap(uint8_t *kek, uint8_t *data, size_t data_size,
	uint8_t *output)
{
	if(!kek || !data)
		return EINVAL;
	
	if(!output)
		return ENOMEM;

	uint32_t n = data_size/8 - 1;
	uint8_t work_data[n*8];
	uint8_t work_input[AES_CIPHER_LENGTH];
	uint8_t work_output[AES_CIPHER_LENGTH];
	uint8_t *work_block;
	uint8_t a[8];
	memcpy(a, data, 8);
	uint64_t mask = 0xFF;
	uint8_t shift, shb;
	
	memcpy(work_data, data + 8, n*8);
	for(int j = 5; j >=0; j--) {
		for(int i = n; i > 0; i--) {
			for(size_t k = 0; k < 8; k++) {
				shift = 56 - 8*k;
				shb = ((n*j+i) & (mask << shift)) >> shift;
				a[k] ^= shb;
			}
			work_block = work_data + (i-1)*8;
			memcpy(work_input, a, 8);
			memcpy(work_input + 8, work_block, 8);
			aes_decrypt(kek, work_input, work_output);
			memcpy(a, work_output, 8);
			memcpy(work_data + (i-1)*8, work_output + 8, 8);
		}
	}
	
	size_t it;
	for(it = 0; it < 8; it++) {
		if(a[it] != 0xA6)
			break;
	}
	
	if(it == 8) {
		memcpy(output, work_data, n*8);
		return EOK;
	} else {
		return EINVAL;
	}
}

int rnd_sequence(uint8_t *sequence, size_t length)
{
	if(!sequence)
		return ENOMEM;
	
	for(size_t i = 0; i < length; i++) {
		sequence[i] = (uint8_t) rand();
	}
	
	return EOK;
}

uint8_t *min_sequence(uint8_t *seq1, uint8_t *seq2, size_t size)
{
	if(!seq1 || !seq2)
		return NULL;
	
	for(size_t i = 0; i < size; i++) {
		if(seq1[i] < seq2[i]) {
			return seq1;
		} else if(seq1[i] > seq2[i]) {
			return seq2;
		}
	}
	
	return seq1;
}

uint8_t *max_sequence(uint8_t *seq1, uint8_t *seq2, size_t size)
{
	uint8_t *min = min_sequence(seq1, seq2, size);
	if(min == seq1) {
		return seq2;
	} else {
		return seq1;
	}
}

/** @}
 */