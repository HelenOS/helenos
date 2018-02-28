/*
 * Copyright (c) 2012 Jan Vesely
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
/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <async.h>
#include <devman.h>
#include <ddf/log.h>
#include <errno.h>
#include <macros.h>
#include <str.h>
#include <as.h>

#include "audio_pcm_iface.h"
#include "ddf/driver.h"

typedef enum {
	IPC_M_AUDIO_PCM_GET_INFO_STR,
	IPC_M_AUDIO_PCM_QUERY_CAPS,
	IPC_M_AUDIO_PCM_REGISTER_EVENTS,
	IPC_M_AUDIO_PCM_UNREGISTER_EVENTS,
	IPC_M_AUDIO_PCM_TEST_FORMAT,
	IPC_M_AUDIO_PCM_GET_BUFFER,
	IPC_M_AUDIO_PCM_RELEASE_BUFFER,
	IPC_M_AUDIO_PCM_GET_BUFFER_POS,
	IPC_M_AUDIO_PCM_START_PLAYBACK,
	IPC_M_AUDIO_PCM_STOP_PLAYBACK,
	IPC_M_AUDIO_PCM_START_CAPTURE,
	IPC_M_AUDIO_PCM_STOP_CAPTURE,
} audio_pcm_iface_funcs_t;

/**
 * Get human readable capability name.
 * @param cap audio capability.
 * @return Valid string
 */
const char *audio_pcm_cap_str(audio_cap_t cap)
{
	static const char *caps[] = {
		[AUDIO_CAP_CAPTURE] = "CAPTURE",
		[AUDIO_CAP_PLAYBACK] = "PLAYBACK",
		[AUDIO_CAP_MAX_BUFFER] = "MAXIMUM BUFFER SIZE",
		[AUDIO_CAP_BUFFER_POS] = "KNOWS BUFFER POSITION",
		[AUDIO_CAP_INTERRUPT] = "FRAGMENT INTERRUPTS",
		[AUDIO_CAP_INTERRUPT_MIN_FRAMES] = "MINIMUM FRAGMENT SIZE",
		[AUDIO_CAP_INTERRUPT_MAX_FRAMES] = "MAXIMUM FRAGMENT SIZE",
	};
	if (cap >= ARRAY_SIZE(caps))
		return "UNKNOWN CAP";
	return caps[cap];

}

/**
 * Get human readable event name.
 * @param event Audio device event
 * @return Valid string
 */
const char *audio_pcm_event_str(pcm_event_t event)
{
	static const char *events[] = {
		[PCM_EVENT_PLAYBACK_STARTED] = "PLAYBACK STARTED",
		[PCM_EVENT_CAPTURE_STARTED] = "CAPTURE STARTED",
		[PCM_EVENT_FRAMES_PLAYED] = "FRAGMENT PLAYED",
		[PCM_EVENT_FRAMES_CAPTURED] = "FRAGMENT CAPTURED",
		[PCM_EVENT_PLAYBACK_TERMINATED] = "PLAYBACK TERMINATED",
		[PCM_EVENT_CAPTURE_TERMINATED] = "CAPTURE TERMINATED",
	};
	if (event >= ARRAY_SIZE(events))
		return "UNKNOWN EVENT";
	return events[event];
}

/*
 * CLIENT SIDE
 */

/**
 * Open audio session with the first registered device.
 *
 * @return Pointer to a new audio device session, NULL on failure.
 */
audio_pcm_sess_t *audio_pcm_open_default(void)
{
	static bool resolved = false;
	static category_id_t pcm_id = 0;
	if (!resolved) {
		const errno_t ret = loc_category_get_id("audio-pcm", &pcm_id,
		    IPC_FLAG_BLOCKING);
		if (ret != EOK)
			return NULL;
		resolved = true;
	}

	service_id_t *svcs = NULL;
	size_t count = 0;
	const errno_t ret = loc_category_get_svcs(pcm_id, &svcs, &count);
	if (ret != EOK)
		return NULL;

	audio_pcm_sess_t *session = NULL;
	if (count)
		session = audio_pcm_open_service(svcs[0]);
	free(svcs);
	return session;
}

/**
 * Open audio session with device identified by location service string.
 *
 * @param name Location service string.
 * @return Pointer to a new audio device session, NULL on failure.
 */
audio_pcm_sess_t *audio_pcm_open(const char *name)
{
	devman_handle_t device_handle = 0;
	const errno_t ret = devman_fun_get_handle(name, &device_handle, 0);
	if (ret != EOK)
		return NULL;
	return devman_device_connect(device_handle, IPC_FLAG_BLOCKING);
}

/**
 * Open audio session with device identified by location service id
 *
 * @param name Location service id.
 * @return Pointer to a new audio device session, NULL on failure.
 */
audio_pcm_sess_t *audio_pcm_open_service(service_id_t id)
{
	return loc_service_connect(id, INTERFACE_DDF, IPC_FLAG_BLOCKING);
}

/**
 * Close open audio device session.
 *
 * @param name Open audio device session.
 *
 * @note Calling this function on already closed or invalid session results
 * in undefined behavior.
 */
void audio_pcm_close(audio_pcm_sess_t *sess)
{
	if (sess)
		async_hangup(sess);
}

/**
 * Get a short description string.
 *
 * @param sess Audio device session.
 * @param name Place to store newly allocated string.
 *
 * @return Error code.
 *
 * @note Caller is responsible for freeing newly allocated memory.
 */
errno_t audio_pcm_get_info_str(audio_pcm_sess_t *sess, char **name)
{
	if (!name)
		return EINVAL;
	async_exch_t *exch = async_exchange_begin(sess);
	sysarg_t name_size;
	const errno_t ret = async_req_1_1(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_GET_INFO_STR, &name_size);
	if (ret == EOK) {
		char *name_place = calloc(1, name_size);
		if (!name_place) {
			/* Make the other side fail
			 * as it waits for read request */
			async_data_read_start(exch, (void*)-1, 0);
			async_exchange_end(exch);
			return ENOMEM;
		}
		const errno_t ret =
		    async_data_read_start(exch, name_place, name_size);
		if (ret != EOK) {
			free(name_place);
			async_exchange_end(exch);
			return ret;
		}
		*name = name_place;
	}
	async_exchange_end(exch);
	return ret;
}


/**
 * Query value of specified capability.
 *
 * @param sess Audio device session.
 * @param cap  Audio device capability.
 * @param[out] val  Place to store queried value.
 *
 * @return Error code.
 */
errno_t audio_pcm_query_cap(audio_pcm_sess_t *sess, audio_cap_t cap, sysarg_t *value)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_2_1(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE), IPC_M_AUDIO_PCM_QUERY_CAPS,
	    cap, value);
	async_exchange_end(exch);
	return ret;
}

/**
 * Query current position in device buffer.
 *
 * @param sess Audio device session.
 * @param pos Place to store the result.
 *
 * @return Error code.
 *
 * Works for both playback and capture.
 */
errno_t audio_pcm_get_buffer_pos(audio_pcm_sess_t *sess, size_t *pos)
{
	if (!pos)
		return EINVAL;
	async_exch_t *exch = async_exchange_begin(sess);
	sysarg_t value = 0;
	const errno_t ret = async_req_1_1(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_GET_BUFFER_POS, &value);
	if (ret == EOK)
		*pos = value;
	async_exchange_end(exch);
	return ret;
}

/**
 * Test format parameters for device support.
 *
 * @param sess Audio device session.
 * @param channels Number of channels
 * @param rate Sampling rate.
 * @format Sample format.
 *
 * @return Error code.
 *
 * Works for both playback and capture. This function modifies provided
 * parameters to the nearest values supported by the device.
 */
errno_t audio_pcm_test_format(audio_pcm_sess_t *sess, unsigned *channels,
    unsigned *rate, pcm_sample_format_t *format)
{
	async_exch_t *exch = async_exchange_begin(sess);
	sysarg_t channels_arg = channels ? *channels : 0;
	sysarg_t rate_arg = rate ? *rate : 0;
	sysarg_t format_arg = format ? *format : 0;
	const errno_t ret = async_req_4_3(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_TEST_FORMAT, channels_arg, rate_arg, format_arg,
	    &channels_arg, &rate_arg, &format_arg);
	async_exchange_end(exch);

	/* All OK or something has changed. Verify that it was not one of the
	 * params we care about */
	if ((ret == EOK || ret == ELIMIT)
	    && (!channels || *channels == channels_arg)
	    && (!rate || *rate == rate_arg)
	    && (!format || *format == format_arg))
		return EOK;
	if (channels)
		*channels = channels_arg;
	if (rate)
		*rate = rate_arg;
	if (format)
		*format = format_arg;
	return ret;
}

/**
 * Register callback for device generated events.
 *
 * @param sess Audio device session.
 * @param event_rec Event callback function.
 * @param arg Event callback custom parameter.
 *
 * @return Error code.
 */
errno_t audio_pcm_register_event_callback(audio_pcm_sess_t *sess,
    async_port_handler_t event_callback, void *arg)
{
	if (!event_callback)
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);

	errno_t ret = async_req_1_0(exch, DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_REGISTER_EVENTS);
	if (ret == EOK) {
		port_id_t port;
		ret = async_create_callback_port(exch, INTERFACE_AUDIO_PCM_CB, 0, 0,
		    event_callback, arg, &port);
	}

	async_exchange_end(exch);
	return ret;
}

/**
 * Unregister callback for device generated events.
 *
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_unregister_event_callback(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_1_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_UNREGISTER_EVENTS);
	async_exchange_end(exch);
	return ret;
}

/**
 * Get device accessible playback/capture buffer.
 *
 * @param sess Audio device session.
 * @param buffer Place to store pointer to the buffer.
 * @param size Place to store buffer size (bytes).
 *
 * @return Error code.
 */
errno_t audio_pcm_get_buffer(audio_pcm_sess_t *sess, void **buffer, size_t *size)
{
	if (!buffer || !size)
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);

	sysarg_t buffer_size = *size;
	errno_t ret = async_req_2_1(exch, DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_GET_BUFFER, (sysarg_t)buffer_size, &buffer_size);
	if (ret == EOK) {
		void *dst = NULL;
		ret = async_share_in_start_0_0(exch, buffer_size, &dst);
		if (ret != EOK) {
			async_exchange_end(exch);
			return ret;
		}
		*buffer = dst;
		*size = buffer_size;
	}
	async_exchange_end(exch);
	return ret;
}

/**
 * Release device accessible playback/capture buffer.
 *
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_release_buffer(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_1_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_RELEASE_BUFFER);
	async_exchange_end(exch);
	return ret;
}

/**
 * Start playback on buffer from position 0.
 *
 * @param sess Audio device session.
 * @param frames Size of fragment (in frames).
 * @param channels Number of channels.
 * @param sample_rate Sampling rate (for one channel).
 * @param format Sample format.
 *
 * @return Error code.
 *
 * Event will be generated after every fragment. Set fragment size to
 * 0 to turn off event generation.
 */
errno_t audio_pcm_start_playback_fragment(audio_pcm_sess_t *sess, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	if (channels > UINT16_MAX)
		return EINVAL;
	assert((format & UINT16_MAX) == format);
	const sysarg_t packed = (channels << 16) | (format & UINT16_MAX);
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_4_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_START_PLAYBACK,
	    frames, sample_rate, packed);
	async_exchange_end(exch);
	return ret;
}
/**
 * Stops playback after current fragment.
 *
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_last_playback_fragment(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_2_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_PLAYBACK, false);
	async_exchange_end(exch);
	return ret;
}

/**
 * Start playback on buffer from the current position.
 *
 * @param sess Audio device session.
 * @param channels Number of channels.
 * @param sample_rate Sampling rate (for one channel).
 * @param format Sample format.
 *
 * @return Error code.
 */
errno_t audio_pcm_start_playback(audio_pcm_sess_t *sess,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	return audio_pcm_start_playback_fragment(
	    sess, 0, channels, sample_rate, format);
}

/**
 * Immediately stops current playback.
 *
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_stop_playback_immediate(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_2_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_PLAYBACK, true);
	async_exchange_end(exch);
	return ret;
}

/**
 * Stops playback at the end of the current fragment.
 *
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_stop_playback(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_2_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_PLAYBACK, false);
	async_exchange_end(exch);
	return ret;
}

/**
 * Start capture on buffer from the current position.
 *
 * @param sess Audio device session.
 * @param frames Size of fragment (in frames).
 * @param channels Number of channels.
 * @param sample_rate Sampling rate (for one channel).
 * @param format Sample format.
 *
 * @return Error code.
 *
 * Event will be generated after every fragment. Set fragment size to
 * 0 to turn off event generation.
 */
errno_t audio_pcm_start_capture_fragment(audio_pcm_sess_t *sess, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	if (channels > UINT16_MAX)
		return EINVAL;
	assert((format & UINT16_MAX) == format);
	const sysarg_t packed = (channels << 16) | (format & UINT16_MAX);
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_4_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE), IPC_M_AUDIO_PCM_START_CAPTURE,
	    frames, sample_rate, packed);
	async_exchange_end(exch);
	return ret;
}

/**
 * Start capture on buffer from the current position.
 *
 * @param sess Audio device session.
 * @param channels Number of channels.
 * @param sample_rate Sampling rate (for one channel).
 * @param format Sample format.
 *
 * @return Error code.
 */
errno_t audio_pcm_start_capture(audio_pcm_sess_t *sess,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	return audio_pcm_start_capture_fragment(
	    sess, 0, channels, sample_rate, format);
}

/**
 * Stops capture at the end of current fragment.
 *
 * Won't work if capture was started with fragment size 0.
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_last_capture_fragment(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_2_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_CAPTURE, false);
	async_exchange_end(exch);
	return ret;
}

/**
 * Immediately stops current capture.
 *
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_stop_capture_immediate(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_2_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_CAPTURE, true);
	async_exchange_end(exch);
	return ret;
}

/**
 * Stops capture at the end of the current fragment.
 *
 * @param sess Audio device session.
 *
 * @return Error code.
 */
errno_t audio_pcm_stop_capture(audio_pcm_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_2_0(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_CAPTURE, false);
	async_exchange_end(exch);
	return ret;
}

/*
 * SERVER SIDE
 */
static void remote_audio_pcm_get_info_str(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_query_caps(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_events_register(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_events_unregister(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_get_buffer_pos(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_test_format(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_get_buffer(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_release_buffer(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_start_playback(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_stop_playback(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_start_capture(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_stop_capture(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote audio pcm buffer interface operations. */
static const remote_iface_func_ptr_t remote_audio_pcm_iface_ops[] = {
	[IPC_M_AUDIO_PCM_GET_INFO_STR] = remote_audio_pcm_get_info_str,
	[IPC_M_AUDIO_PCM_QUERY_CAPS] = remote_audio_pcm_query_caps,
	[IPC_M_AUDIO_PCM_REGISTER_EVENTS] = remote_audio_pcm_events_register,
	[IPC_M_AUDIO_PCM_UNREGISTER_EVENTS] = remote_audio_pcm_events_unregister,
	[IPC_M_AUDIO_PCM_GET_BUFFER_POS] = remote_audio_pcm_get_buffer_pos,
	[IPC_M_AUDIO_PCM_TEST_FORMAT] = remote_audio_pcm_test_format,
	[IPC_M_AUDIO_PCM_GET_BUFFER] = remote_audio_pcm_get_buffer,
	[IPC_M_AUDIO_PCM_RELEASE_BUFFER] = remote_audio_pcm_release_buffer,
	[IPC_M_AUDIO_PCM_START_PLAYBACK] = remote_audio_pcm_start_playback,
	[IPC_M_AUDIO_PCM_STOP_PLAYBACK] = remote_audio_pcm_stop_playback,
	[IPC_M_AUDIO_PCM_START_CAPTURE] = remote_audio_pcm_start_capture,
	[IPC_M_AUDIO_PCM_STOP_CAPTURE] = remote_audio_pcm_stop_capture,
};

/** Remote audio mixer interface structure. */
const remote_iface_t remote_audio_pcm_iface = {
	.method_count = ARRAY_SIZE(remote_audio_pcm_iface_ops),
	.methods = remote_audio_pcm_iface_ops
};

void remote_audio_pcm_get_info_str(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;

	if (!pcm_iface->get_info_str) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const char *name = NULL;
	const errno_t ret = pcm_iface->get_info_str(fun, &name);
	const size_t name_size = name ? str_size(name) + 1 : 0;
	async_answer_1(callid, ret, name_size);
	/* Send the string. */
	if (ret == EOK && name_size > 0) {
		size_t size;
		ipc_callid_t name_id;
		if (!async_data_read_receive(&name_id, &size)) {
			async_answer_0(name_id, EPARTY);
			return;
		}
		if (size != name_size) {
			async_answer_0(name_id, ELIMIT);
			return;
		}
		async_data_read_finalize(name_id, name, name_size);
	}
}

void remote_audio_pcm_query_caps(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;
	const audio_cap_t cap = DEV_IPC_GET_ARG1(*call);
	if (pcm_iface->query_cap) {
		const unsigned value = pcm_iface->query_cap(fun, cap);
		async_answer_1(callid, EOK, value);
	} else {
		async_answer_0(callid, ENOTSUP);
	}
}

static void remote_audio_pcm_events_register(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;
	if (!pcm_iface->get_event_session ||
	    !pcm_iface->set_event_session) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	async_answer_0(callid, EOK);

	ipc_call_t callback_call;
	ipc_callid_t callback_id = async_get_call(&callback_call);
	async_sess_t *sess =
	    async_callback_receive_start(EXCHANGE_ATOMIC, &callback_call);
	if (sess == NULL) {
		ddf_msg(LVL_DEBUG, "Failed to create event callback");
		async_answer_0(callback_id, EAGAIN);
		return;
	}
	const errno_t ret = pcm_iface->set_event_session(fun, sess);
	if (ret != EOK) {
		ddf_msg(LVL_DEBUG, "Failed to set event callback.");
		async_hangup(sess);
		async_answer_0(callback_id, ret);
		return;
	}
	async_answer_0(callback_id, EOK);
}

static void remote_audio_pcm_events_unregister(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;
	if (!pcm_iface->get_event_session ||
	    !pcm_iface->set_event_session) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	async_sess_t *sess = pcm_iface->get_event_session(fun);
	if (sess) {
		async_hangup(sess);
		pcm_iface->set_event_session(fun, NULL);
	}
	async_answer_0(callid, EOK);
}

void remote_audio_pcm_get_buffer_pos(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;
	size_t pos = 0;
	const errno_t ret = pcm_iface->get_buffer_pos ?
	    pcm_iface->get_buffer_pos(fun, &pos) : ENOTSUP;
	async_answer_1(callid, ret, pos);
}

void remote_audio_pcm_test_format(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;
	unsigned channels = DEV_IPC_GET_ARG1(*call);
	unsigned rate = DEV_IPC_GET_ARG2(*call);
	pcm_sample_format_t format = DEV_IPC_GET_ARG3(*call);
	const errno_t ret = pcm_iface->test_format ?
	    pcm_iface->test_format(fun, &channels, &rate, &format) : ENOTSUP;
	async_answer_3(callid, ret, channels, rate, format);
}

void remote_audio_pcm_get_buffer(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;

	if (!pcm_iface->get_buffer ||
	    !pcm_iface->release_buffer) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	void *buffer = NULL;
	size_t size = DEV_IPC_GET_ARG1(*call);
	errno_t ret = pcm_iface->get_buffer(fun, &buffer, &size);
	async_answer_1(callid, ret, size);
	if (ret != EOK || size == 0)
		return;

	/* Share the buffer. */
	size_t share_size = 0;
	ipc_callid_t share_id = 0;

	ddf_msg(LVL_DEBUG2, "Receiving share request.");
	if (!async_share_in_receive(&share_id, &share_size)) {
		ddf_msg(LVL_DEBUG, "Failed to share pcm buffer.");
		pcm_iface->release_buffer(fun);
		async_answer_0(share_id, EPARTY);
		return;
	}

	ddf_msg(LVL_DEBUG2, "Checking requested share size.");
	if (share_size != size) {
		ddf_msg(LVL_DEBUG, "Incorrect pcm buffer size requested.");
		pcm_iface->release_buffer(fun);
		async_answer_0(share_id, ELIMIT);
		return;
	}

	ddf_msg(LVL_DEBUG2, "Calling share finalize.");
	ret = async_share_in_finalize(share_id, buffer, AS_AREA_WRITE
	| AS_AREA_READ);
	if (ret != EOK) {
		ddf_msg(LVL_DEBUG, "Failed to share buffer.");
		pcm_iface->release_buffer(fun);
		return;
	}

	ddf_msg(LVL_DEBUG2, "Buffer shared with size %zu.", share_size);
}

void remote_audio_pcm_release_buffer(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;

	const errno_t ret = pcm_iface->release_buffer ?
	    pcm_iface->release_buffer(fun) : ENOTSUP;
	async_answer_0(callid, ret);
}

void remote_audio_pcm_start_playback(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;

	const unsigned frames = DEV_IPC_GET_ARG1(*call);
	const unsigned rate = DEV_IPC_GET_ARG2(*call);
	const unsigned channels = (DEV_IPC_GET_ARG3(*call) >> 16) & UINT8_MAX;
	const pcm_sample_format_t format = DEV_IPC_GET_ARG3(*call) & UINT16_MAX;

	const errno_t ret = pcm_iface->start_playback
	    ? pcm_iface->start_playback(fun, frames, channels, rate, format)
	    : ENOTSUP;
	async_answer_0(callid, ret);
}

void remote_audio_pcm_stop_playback(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;
	const bool immediate = DEV_IPC_GET_ARG1(*call);

	const errno_t ret = pcm_iface->stop_playback ?
	    pcm_iface->stop_playback(fun, immediate) : ENOTSUP;
	async_answer_0(callid, ret);
}

void remote_audio_pcm_start_capture(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;

	const unsigned frames = DEV_IPC_GET_ARG1(*call);
	const unsigned rate = DEV_IPC_GET_ARG2(*call);
	const unsigned channels = (DEV_IPC_GET_ARG3(*call) >> 16) & UINT16_MAX;
	const pcm_sample_format_t format = DEV_IPC_GET_ARG3(*call) & UINT16_MAX;

	const errno_t ret = pcm_iface->start_capture
	    ? pcm_iface->start_capture(fun, frames, channels, rate, format)
	    : ENOTSUP;
	async_answer_0(callid, ret);
}

void remote_audio_pcm_stop_capture(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_iface_t *pcm_iface = iface;
	const bool immediate = DEV_IPC_GET_ARG1(*call);

	const errno_t ret = pcm_iface->stop_capture ?
	    pcm_iface->stop_capture(fun, immediate) : ENOTSUP;
	async_answer_0(callid, ret);
}

/**
 * @}
 */

