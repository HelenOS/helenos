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

/** Default implementation of IEEE802.11 start function.
 *
 * @param ieee80211_dev Structure of IEEE802.11 device.
 *
 * @return EOK.
 *
 */
errno_t ieee80211_start_impl(ieee80211_dev_t *ieee80211_dev)
{
	return EOK;
}

/** Default implementation of IEEE802.11 TX handler function.
 *
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * @param buffer        Buffer with data to send.
 * @param buffer_size   Size of buffer.
 *
 * @return EOK.
 *
 */
errno_t ieee80211_tx_handler_impl(ieee80211_dev_t *ieee80211_dev, void *buffer,
    size_t buffer_size)
{
	return EOK;
}

/** Default implementation of IEEE802.11 set frequency function.
 *
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * @param freq          Value of frequency to be switched on.
 *
 * @return EOK.
 *
 */
errno_t ieee80211_set_freq_impl(ieee80211_dev_t *ieee80211_dev, uint16_t freq)
{
	return EOK;
}

/** Default implementation of IEEE802.11 BSSID change function.
 *
 * @param ieee80211_dev Structure of IEEE802.11 device.
 *
 * @return EOK.
 *
 */
errno_t ieee80211_bssid_change_impl(ieee80211_dev_t *ieee80211_dev,
    bool connected)
{
	return EOK;
}

/** Default implementation of IEEE802.11 key config function.
 *
 * @param ieee80211_dev Structure of IEEE802.11 device.
 *
 * @return EOK.
 *
 */
errno_t ieee80211_key_config_impl(ieee80211_dev_t *ieee80211_dev,
    ieee80211_key_config_t *key_conf, bool insert)
{
	return EOK;
}

/** Default implementation of IEEE802.11 scan function.
 *
 * @param ieee80211_dev Structure of IEEE802.11 device.
 * @param clear         Whether to clear current scan results.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_scan_impl(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->scan_mutex);

	if (ieee80211_get_auth_phase(ieee80211_dev) ==
	    IEEE80211_AUTH_DISCONNECTED) {
		fibril_mutex_lock(&ieee80211_dev->ap_list.results_mutex);

		/* Remove old entries we don't receive beacons from. */
		ieee80211_scan_result_list_t *result_list =
		    &ieee80211_dev->ap_list;

		list_foreach_safe(result_list->list, cur_link, next_link) {
			ieee80211_scan_result_link_t *cur_result =
			    list_get_instance(cur_link,
			    ieee80211_scan_result_link_t, link);

			if ((time(NULL) - cur_result->last_beacon) >
			    MAX_KEEP_SCAN_SPAN_SEC)
				ieee80211_scan_result_list_remove(result_list,
				    cur_result);
		}

		fibril_mutex_unlock(&ieee80211_dev->ap_list.results_mutex);

		uint16_t orig_freq = ieee80211_dev->current_freq;

		for (uint16_t freq = IEEE80211_FIRST_FREQ;
		    freq <= IEEE80211_MAX_FREQ; freq += IEEE80211_CHANNEL_GAP) {
			if (ieee80211_pending_connect_request(ieee80211_dev))
				break;

			ieee80211_dev->ops->set_freq(ieee80211_dev, freq);
			ieee80211_probe_request(ieee80211_dev, NULL);

			/* Wait for probe responses. */
			async_usleep(SCAN_CHANNEL_WAIT_USEC);
		}

		ieee80211_dev->ops->set_freq(ieee80211_dev, orig_freq);
	}

	fibril_mutex_unlock(&ieee80211_dev->scan_mutex);

	return EOK;
}

/** Pseudorandom function used for IEEE 802.11 pairwise key computation.
 *
 * Using SHA1 hash algorithm.
 *
 * @param key         Key with PBKDF2 encrypted passphrase.
 * @param data        Concatenated sequence of both MAC
 *                    addresses and nonces.
 * @param hash        Output parameter for result hash.
 * @param output_size Length of output sequence to be generated.
 *
 * @return EINVAL when key or data not specified,
 *         ENOMEM when pointer for output hash result
 *         is not allocated, otherwise EOK.
 *
 */
errno_t ieee80211_prf(uint8_t *key, uint8_t *data, uint8_t *hash,
    size_t output_size)
{
	if ((!key) || (!data))
		return EINVAL;

	if (!hash)
		return ENOMEM;

	size_t iters = ((output_size * 8) + 159) / 160;

	const char *a = "Pairwise key expansion";
	uint8_t result[HASH_SHA1 * iters];
	uint8_t temp[HASH_SHA1];

	size_t data_size = PRF_CRYPT_DATA_LENGTH + str_size(a) + 2;
	uint8_t work_arr[data_size];
	memset(work_arr, 0, data_size);

	memcpy(work_arr, a, str_size(a));
	memcpy(work_arr + str_size(a) + 1, data, PRF_CRYPT_DATA_LENGTH);

	for (uint8_t i = 0; i < iters; i++) {
		memcpy(work_arr + data_size - 1, &i, 1);
		hmac(key, PBKDF2_KEY_LENGTH, work_arr, data_size, temp,
		    HASH_SHA1);
		memcpy(result + i * HASH_SHA1, temp, HASH_SHA1);
	}

	memcpy(hash, result, output_size);

	return EOK;
}

errno_t ieee80211_rc4_key_unwrap(uint8_t *key, uint8_t *data, size_t data_size,
    uint8_t *output)
{
	return rc4(key, 32, data, data_size, 256, output);
}

errno_t ieee80211_aes_key_unwrap(uint8_t *kek, uint8_t *data, size_t data_size,
    uint8_t *output)
{
	if ((!kek) || (!data))
		return EINVAL;

	if (!output)
		return ENOMEM;

	uint32_t n = data_size / 8 - 1;
	uint8_t work_data[n * 8];
	uint8_t work_input[AES_CIPHER_LENGTH];
	uint8_t work_output[AES_CIPHER_LENGTH];
	uint8_t *work_block;
	uint8_t a[8];

	memcpy(a, data, 8);

	uint64_t mask = 0xff;
	uint8_t shift, shb;

	memcpy(work_data, data + 8, n * 8);
	for (int j = 5; j >= 0; j--) {
		for (int i = n; i > 0; i--) {
			for (size_t k = 0; k < 8; k++) {
				shift = 56 - 8 * k;
				shb = ((n * j + i) & (mask << shift)) >> shift;
				a[k] ^= shb;
			}

			work_block = work_data + (i - 1) * 8;
			memcpy(work_input, a, 8);
			memcpy(work_input + 8, work_block, 8);
			aes_decrypt(kek, work_input, work_output);
			memcpy(a, work_output, 8);
			memcpy(work_data + (i - 1) * 8, work_output + 8, 8);
		}
	}

	size_t it;
	for (it = 0; it < 8; it++) {
		if (a[it] != 0xa6)
			break;
	}

	if (it == 8) {
		memcpy(output, work_data, n * 8);
		return EOK;
	}

	return EINVAL;
}

static void ieee80211_michael_mic_block(uint32_t *l, uint32_t *r,
    uint32_t value)
{
	*l ^= value;
	*r ^= rotl_uint32(*l, 17);
	*l += *r;
	*r ^= ((*l & 0x00ff00ff) << 8) | ((*l & 0xff00ff00) >> 8);
	*l += *r;
	*r ^= rotl_uint32(*l, 3);
	*l += *r;
	*r ^= rotr_uint32(*l, 2);
	*l += *r;
}

errno_t ieee80211_michael_mic(uint8_t *key, uint8_t *buffer, size_t size,
    uint8_t *mic)
{
	if ((!key) || (!buffer))
		return EINVAL;

	if (!mic)
		return ENOMEM;

	uint32_t l = uint32le_from_seq(key);
	uint32_t r = uint32le_from_seq(key + 4);

	ieee80211_data_header_t *data_header =
	    (ieee80211_data_header_t *) buffer;

	uint8_t *data = buffer + sizeof(ieee80211_data_header_t) +
	    IEEE80211_TKIP_HEADER_LENGTH;
	size_t data_size = size - sizeof(ieee80211_data_header_t) -
	    IEEE80211_TKIP_HEADER_LENGTH;

	/* Process header. */
	uint8_t *src_addr =
	    ieee80211_is_fromds_frame(data_header->frame_ctrl) ?
	    data_header->address3 : data_header->address2;
	uint8_t *dest_addr =
	    ieee80211_is_tods_frame(data_header->frame_ctrl) ?
	    data_header->address3 : data_header->address1;

	ieee80211_michael_mic_block(&l, &r, uint32le_from_seq(dest_addr));
	ieee80211_michael_mic_block(&l, &r,
	    uint16le_from_seq(dest_addr + 4) |
	    (uint16le_from_seq(src_addr) << 16));
	ieee80211_michael_mic_block(&l, &r, uint32le_from_seq(src_addr + 2));
	ieee80211_michael_mic_block(&l, &r, 0);

	/* Process data. */
	size_t blocks = data_size / 4;
	size_t pad = data_size % 4;

	for (size_t k = 0; k < blocks; k++) {
		ieee80211_michael_mic_block(&l, &r,
		    uint32le_from_seq(&data[k * 4]));
	}

	/* Add padding. */
	uint32_t value = 0x5a;
	for (size_t i = pad; i > 0; i--) {
		value <<= 8;
		value |= data[blocks * 4 + (i - 1)];
	}

	ieee80211_michael_mic_block(&l, &r, value);
	ieee80211_michael_mic_block(&l, &r, 0);

	l = host2uint32_t_le(l);
	r = host2uint32_t_le(r);

	memcpy(mic, &l, 4);
	memcpy(mic + 4, &r, 4);

	return EOK;
}

uint16_t uint16le_from_seq(void *seq)
{
	uint16_t *u16 = (uint16_t *) seq;
	return uint16_t_le2host(*u16);
}

uint32_t uint32le_from_seq(void *seq)
{
	uint32_t *u32 = (uint32_t *) seq;
	return uint32_t_le2host(*u32);
}

uint16_t uint16be_from_seq(void *seq)
{
	uint16_t *u16 = (uint16_t *) seq;
	return uint16_t_be2host(*u16);
}

uint32_t uint32be_from_seq(void *seq)
{
	uint32_t *u32 = (uint32_t *) seq;
	return uint32_t_be2host(*u32);
}

errno_t rnd_sequence(uint8_t *sequence, size_t length)
{
	if (!sequence)
		return ENOMEM;

	for (size_t i = 0; i < length; i++)
		sequence[i] = (uint8_t) rand();

	return EOK;
}

uint8_t *min_sequence(uint8_t *seq1, uint8_t *seq2, size_t size)
{
	if ((!seq1) || (!seq2))
		return NULL;

	for (size_t i = 0; i < size; i++) {
		if (seq1[i] < seq2[i])
			return seq1;
		else if (seq1[i] > seq2[i])
			return seq2;
	}

	return seq1;
}

uint8_t *max_sequence(uint8_t *seq1, uint8_t *seq2, size_t size)
{
	uint8_t *min = min_sequence(seq1, seq2, size);
	if (min == seq1)
		return seq2;

	return seq1;
}

/** @}
 */
