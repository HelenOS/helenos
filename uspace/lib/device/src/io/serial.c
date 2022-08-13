/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <async.h>
#include <errno.h>
#include <io/serial.h>
#include <ipc/serial_ctl.h>
#include <ipc/services.h>
#include <stdlib.h>

/** Open serial port device.
 *
 * @param sess Session with the serial port device
 * @param rchardev Place to store pointer to the new serial port device structure
 *
 * @return EOK on success, ENOMEM if out of memory, EIO on I/O error
 */
errno_t serial_open(async_sess_t *sess, serial_t **rserial)
{
	serial_t *serial;

	serial = calloc(1, sizeof(serial_t));
	if (serial == NULL)
		return ENOMEM;

	serial->sess = sess;
	*rserial = serial;

	return EOK;
}

/** Close serial port device.
 *
 * Frees the serial port device structure. The underlying session is
 * not affected.
 *
 * @param serial Serial port device or @c NULL
 */
void serial_close(serial_t *serial)
{
	free(serial);
}

/** Set serial port communication properties. */
errno_t serial_set_comm_props(serial_t *serial, unsigned rate,
    serial_parity_t parity, unsigned datab, unsigned stopb)
{
	async_exch_t *exch = async_exchange_begin(serial->sess);

	errno_t rc = async_req_4_0(exch, SERIAL_SET_COM_PROPS, rate, parity,
	    datab, stopb);

	async_exchange_end(exch);
	return rc;
}

/** Get serial port communication properties. */
errno_t serial_get_comm_props(serial_t *serial, unsigned *rrate,
    serial_parity_t *rparity, unsigned *rdatab, unsigned *rstopb)
{
	async_exch_t *exch = async_exchange_begin(serial->sess);
	sysarg_t rate, parity, datab, stopb;

	errno_t rc = async_req_0_4(exch, SERIAL_GET_COM_PROPS, &rate, &parity,
	    &datab, &stopb);

	async_exchange_end(exch);
	if (rc != EOK)
		return rc;

	*rrate = rate;
	*rparity = parity;
	*rdatab = datab;
	*rstopb = stopb;

	return EOK;
}

/** @}
 */
