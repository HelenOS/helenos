/*
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup graph
 * @{
 */
/**
 * @file
 */

#ifndef GRAPH_GRAPH_H_
#define GRAPH_GRAPH_H_

#include <stdbool.h>
#include <loc.h>
#include <async.h>
#include <atomic.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <io/mode.h>
#include <io/pixelmap.h>
#include <ipc/graph.h>

struct visualizer;
struct renderer;

typedef struct {
	/**
	 * Device driver shall allocate any necessary internal structures
	 * specific for a claimed visualizer. */
	errno_t (*claim)(struct visualizer *vs);

	/**
	 * Device driver shall deallocate any necessary internal structures
	 * specific for a claimed visualizer. Driver shall also check whether
	 * the mode is set and if so it shall change its internal state
	 * accordingly (e.g. deallocate frame buffers). */
	errno_t (*yield)(struct visualizer *vs);

	/**
	 * Device driver shall first try to claim all resources required for
	 * a new mode (e.g. allocate new framebuffers) and only if successful
	 * it shall free resources for the old mode. Although such behaviour
	 * might not be always possible, it is preferable since libgraph tries to
	 * keep current mode functional if the new mode cannot be set (for any
	 * reason). If it is convenient for the device driver (e.g. for better
	 * optimization), the pointer to the handle_damage operation can be
	 * changed at this point. */
	errno_t (*change_mode)(struct visualizer *vs, vslmode_t new_mode);

	/**
	 * Device driver shall render the cells from damaged region into its
	 * internal framebuffer. In case the driver use multi-buffering, it
	 * shall also switch internal buffers (e.g. by pageflip). Offsets
	 * are intended to support basic vertical and horizontal scrolling on
	 * the shared backbuffer (i.e. when reading from backbuffer, the offsets
	 * shall be added to the coordinates and if necessary the result shall be
	 * wrapped around the edge of the backbuffer). */
	errno_t (*handle_damage)(struct visualizer *vs,
	    sysarg_t x, sysarg_t y, sysarg_t width, sysarg_t height,
	    sysarg_t x_offset, sysarg_t y_offset);

	/**
	 * Upper layers of the graphic stack might report inactivity. In such
	 * case, device driver might enable power saving mode on the device
	 * corresponding to the visualizer. */
	errno_t (*suspend)(struct visualizer *vs);

	/**
	 * When upper layers detect activity on suspended visualizer, device
	 * driver shall disable power saving mode on the corresponding device. */
	errno_t (*wakeup)(struct visualizer *vs);
} visualizer_ops_t;

/**
 * Represents final output device (e.g. monitor connected to the port
 * on the graphic adapter, serial console, local/remote virtual monitor).
 */
typedef struct visualizer {
	/**
	 * Link to the list of all visualizers belonging to the graphic device.
	 * Field is fully managed by libgraph. */
	link_t link;

	/**
	 * When reference count equals 1, visualizer is claimed by a client,
	 * when equals 0, visualizer is not claimed. At the time, visualizer
	 * can be claimed only by a single client.
	 * Field is fully managed by libgraph. */
	atomic_t ref_cnt;

	/**
	 * Visualizer ID assigned by some particular registration service
	 * in the system (either location service or device manager). Intended
	 * for cleanup duties (e.g. unregistering visualizer).
	 * If the visualizer is registered through libgraph functions, then the
	 * field is fully managed by libgraph. Otherwise, it is a driver
	 * responsibility to set and update this field. */
	sysarg_t reg_svc_handle;

	/**
	 * Visualizer ID in the client context. When client gets notified by
	 * libgraph about some event, it can use this identification to lookup
	 * data structures corresponding to a particular visualizer (e.g. viewports
	 * in the compositor).
	 * Field is fully managed by libgraph. It is assigned when the visualizer
	 * gets claimed and is valid until it is yielded. */
	sysarg_t client_side_handle;

	/**
	 * Callback session to the client. Established by libgraph during initial
	 * phase of client connection. Can be used to notify client about
	 * external asynchronous changes to the output device state
	 * (e.g. monitor gets disconnected from the graphic adapter, virtual
	 * monitor is terminated by the user, pivot monitor is rotated by 90
	 * degrees, virtual monitor is resized by the user).
	 * Field is fully managed by libgraph. Device driver can use it indirectly
	 * through notification functions. */
	async_sess_t *notif_sess;

	/**
	 * Mutex protecting the mode list and default mode index. This is required
	 * for the case when device driver might asynchronously update these
	 * upon the request from the final output device (e.g. to change mode
	 * dimensions when virtual monitor is resized).
	 * Both device driver and libgraph must lock on this mutex when accessing
	 * modes list or default mode index. */
	fibril_mutex_t mode_mtx;

	/**
	 * List of all modes that can be set by this visualizer. List is populated
	 * by device driver when creating a new visualizer or when handling the
	 * request from the final output device to change the available modes.
	 * When this happens, the device driver is expected to increment version
	 * numbers in the modified modes. Modes in the list typically represent
	 * the intersection of modes supported by device driver (graphic adapter)
	 * and final output device (e.g. monitor).
	 * Field is fully managed by device driver, libgraph reads it with locked
	 * mutex. */
	list_t modes;

	/**
	 * Index of the default mode. Might come in handy to the clients that are
	 * not able to enumerate modes and present the choice to the user
	 * (either the client does not have such functionality or the client is
	 * bootstraping without any preliminary knowledge). Device driver shall
	 * maintain this field at the same time when it is doing any changes to the
	 * mode list.
	 * Field is fully managed by device driver, libgraph reads it with locked
	 * mutex. */
	sysarg_t def_mode_idx;

	/**
	 * Copy of the currently established mode. It is read by both libgraph and
	 * device driver when deallocating resources for the current mode. Device
	 * driver can also read it to properly interpret the cell type and its
	 * internal structures when handling the damage.
	 * Field is fully managed by libgraph, can be read by device driver. */
	vslmode_t cur_mode;

	/**
	 * Determines whether the visualizer is currently set to some mode or not,
	 * that is whether cur_mode field contains up-to-date data.
	 * Field is fully managed by libgraph, can be read by device driver. */
	bool mode_set;

	/**
	 * Device driver function pointers.
	 * Field is fully managed by device driver, libgraph invokes driver's
	 * functions through it. */
	visualizer_ops_t ops;

	/**
	 * Backbuffer shared with the client. Sharing is established by libgraph.
	 * Device driver reads the cells when handling damage. Cells shall be
	 * interpreted according to the currently set mode.
	 * Field is fully managed by libgraph, can be read by device driver. */
	pixelmap_t cells;

	/**
	 * Device driver context, completely opaque to the libgraph. Intended to
	 * contain pointers to frontbuffers or information representing the
	 * final output device (e.g. hardware port for physical monitor).
	 * Field is fully managed by device driver. */
	void *dev_ctx;
} visualizer_t;

typedef struct {
	// TODO
	int dummy;
} renderer_ops_t;

/**
 * NOTE: Following description is just a result of brainstorming on how the
 *       driver of real physical graphic accelerator could be supported by
 *       libgraph.
 *
 * Renderer represents the hardware graphic accelerator. If the device driver
 * handles more than one physical accelerator (e.g. graphic cards connected in
 * SLI mode or single graphic card with two GPUs), it is up to the driver
 * whether the load balancing will be exposed to the clients (that is the
 * driver will provide multiple renderers) or not (just a single renderer
 * handling the load balancing internally).
 *
 * At runtime, renderer is represented by scheduling thread and multiple
 * connection fibrils handling client requests. For each client, there is
 * command queue, condition variable and device context. Connection fibril puts
 * the command into the command queue and blocks on the condition variable.
 * Scheduling thread decides what client to serve, switches corresponding device
 * context into the accelerator, consumes the command from the command queue and
 * executes it on the accelerator. When the command execution is finished, the
 * condition variable si signalled and the connection fibril answers the client.
 *
 * Operations that are not implemented in the hardware of the accelerator might
 * be carried out by worker threads managed by scheduling thread. If the
 * accelerator physical memory is mapped to the address space of the driver, it
 * could be extended by allowing scheduling thread to page out the memory and
 * to handle the page faults.
 *
 * NOTE: It is not entirely clear which parts should be implemented in libgraph
 *       and which in the device driver. As a rough sketch, the connection
 *       fibril routine, command queue and memory paging should be handled
 *       by libgraph. The scheduling thread and device context should be
 *       provided by device driver.
 */
typedef struct renderer {
	// TODO
	link_t link;

	atomic_t ref_cnt;

	sysarg_t reg_svc_handle;

	renderer_ops_t ops;
} renderer_t;

/*----------------------------------------------------------------------------*/

/**
 * Fill in the basic visualizer structure. The device driver shall take the
 * created torso and to complete it by adding its specific structures
 * (device context, modes). */
extern void graph_init_visualizer(visualizer_t *);

extern void graph_init_renderer(renderer_t *);

/*----------------------------------------------------------------------------*/

/*
 * NOTE: All functions in this section are intended to be used only by various
 *       driver emulators (e.g. compositor client containing emulated driver
 *       to render the framebuffer of other compositor or console server).
 *       Driver of the physical hardware shall instead use similar functions
 *       from libdrv.
 */

/**
 * Allocate the visualizer so it can be initialized. */
extern visualizer_t *graph_alloc_visualizer(void);

/**
 * Register the completely prepared visualizer to the location service and
 * add it to the driver visualizer list. After the registration, the visualizer
 * is considered ready to handle client connection. Since visualizer
 * list is guarded by the mutex, visualizers might be added even after the
 * initialialization of the device driver. */
extern errno_t graph_register_visualizer(visualizer_t *);

/**
 * Retrieve the visualizer from the visualizer list according to its
 * service ID. */
extern visualizer_t *graph_get_visualizer(sysarg_t);

/**
 * Unregister the visualizer from the location service and remove it
 * from the driver visualizer list. Function shall be called by device driver
 * before deallocating the resources for the visualizer. */
extern errno_t graph_unregister_visualizer(visualizer_t *);

/**
 * Destroy the rest of the visualizer. Device driver shall call this function
 * only after it has unregistered the visualizer and deallocated all the
 * resources for which the driver is responsible. */
extern void graph_destroy_visualizer(visualizer_t *);

extern renderer_t *graph_alloc_renderer(void);
extern errno_t graph_register_renderer(renderer_t *);
extern renderer_t *graph_get_renderer(sysarg_t);
extern errno_t graph_unregister_renderer(renderer_t *);
extern void graph_destroy_renderer(renderer_t *);

/*----------------------------------------------------------------------------*/

/**
 * Device driver can call this function to notify the client through the
 * callback connection that the visualizer with a specified service ID should
 * be switched to the mode with the given index. */
extern errno_t graph_notify_mode_change(async_sess_t *, sysarg_t, sysarg_t);

/**
 * Device driver can call this function to notify the client through the
 * callback connection that the visualizer with a specified service ID has
 * lost its output device (e.g. virtual monitor was closed by a user). */
extern errno_t graph_notify_disconnect(async_sess_t *, sysarg_t);

/*----------------------------------------------------------------------------*/

/** Shall be registered to libdrv by physical device driver. */
extern void graph_visualizer_connection(visualizer_t *, ipc_callid_t, ipc_call_t *, void *);

/** Shall be registered to libdrv by physical device driver. */
extern void graph_renderer_connection(renderer_t *, ipc_callid_t, ipc_call_t *, void *);

/** Shall be registered to location service by emulated device driver. */
extern void graph_client_connection(ipc_callid_t, ipc_call_t *, void *);

#endif

/** @}
 */
