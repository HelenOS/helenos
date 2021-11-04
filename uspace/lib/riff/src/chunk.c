/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libriff
 * @{
 */
/**
 * @file RIFF chunk.
 */

#include <assert.h>
#include <byteorder.h>
#include <errno.h>
#include <macros.h>
#include <riff/chunk.h>
#include <stdbool.h>
#include <stdlib.h>

/** Open RIFF file for writing
 *
 * @param fname File name
 * @param rrw   Place to store pointer to RIFF writer
 *
 * @return EOK on success, ENOMEM if out of memory, EIO if failed to open
 *         file.
 */
errno_t riff_wopen(const char *fname, riffw_t **rrw)
{
	riffw_t *rw;

	rw = calloc(1, sizeof(riffw_t));
	if (rw == NULL)
		return ENOMEM;

	rw->f = fopen(fname, "wb");
	if (rw->f == NULL) {
		free(rw);
		return EIO;
	}

	*rrw = rw;
	return EOK;
}

/** Close RIFF for writing.
 *
 * @param rw RIFF writer
 * @return EOK on success. On write error EIO is returned and RIFF writer
 *         is destroyed anyway.
 */
errno_t riff_wclose(riffw_t *rw)
{
	int rv;

	rv = fclose(rw->f);
	free(rw);

	return (rv == 0) ? EOK : EIO;
}

/** Write uint32_t value into RIFF file
 *
 * @param rw RIFF writer
 * @param v  Value
 * @return EOK on success, EIO on error.
 */
errno_t riff_write_uint32(riffw_t *rw, uint32_t v)
{
	uint32_t vle;

	vle = host2uint32_t_le(v);
	if (fwrite(&vle, 1, sizeof(vle), rw->f) < sizeof(vle))
		return EIO;

	return EOK;
}

/** Begin writing chunk.
 *
 * @param rw     RIFF writer
 * @param ckid   Chunk ID
 * @param wchunk Pointer to chunk structure to fill in
 *
 * @return EOK on success, EIO on write error
 */
errno_t riff_wchunk_start(riffw_t *rw, riff_ckid_t ckid, riff_wchunk_t *wchunk)
{
	long pos;
	errno_t rc;

	pos = ftell(rw->f);
	if (pos < 0)
		return EIO;

	wchunk->ckstart = pos + 2 * sizeof(uint32_t);

	rc = riff_write_uint32(rw, ckid);
	if (rc != EOK) {
		assert(rc == EIO);
		return EIO;
	}

	rc = riff_write_uint32(rw, 0);
	if (rc != EOK) {
		assert(rc == EIO);
		return EIO;
	}

	return EOK;
}

/** Finish writing chunk.
 *
 * @param rw     RIFF writer
 * @param wchunk Pointer to chunk structure
 *
 * @return EOK on success, EIO error.
 */
errno_t riff_wchunk_end(riffw_t *rw, riff_wchunk_t *wchunk)
{
	long pos;
	long cksize;
	uint8_t pad;
	size_t nw;
	errno_t rc;

	pos = ftell(rw->f);
	if (pos < 0)
		return EIO;

	cksize = pos - wchunk->ckstart;
	if (pos % 2 != 0) {
		++pos;
		pad = 0;
		nw = fwrite(&pad, 1, sizeof(pad), rw->f);
		if (nw != sizeof(pad))
			return EIO;
	}

	if (fseek(rw->f, wchunk->ckstart - 4, SEEK_SET) < 0)
		return EIO;

	rc = riff_write_uint32(rw, cksize);
	if (rc != EOK) {
		assert(rc == EIO);
		return EIO;
	}

	if (fseek(rw->f, pos, SEEK_SET) < 0)
		return EIO;

	return EOK;
}

/** Write data into RIFF file.
 *
 * @param rw    RIFF writer
 * @param data  Pointer to data
 * @param bytes Number of bytes to write
 *
 * @return EOK on success, EIO on error.
 */
errno_t riff_write(riffw_t *rw, void *data, size_t bytes)
{
	size_t nw;

	nw = fwrite(data, 1, bytes, rw->f);
	if (nw != bytes)
		return EIO;

	return EOK;
}

/** Open RIFF file for reading.
 *
 * @param fname File name
 * @param riffck Place to store root (RIFF) chunk
 * @param rrr   Place to store pointer to RIFF reader
 *
 * @return EOK on success, ENOMEM if out of memory, EIO if failed to open
 *         file..
 */
errno_t riff_ropen(const char *fname, riff_rchunk_t *riffck, riffr_t **rrr)
{
	riffr_t *rr;
	riff_rchunk_t fchunk;
	long fsize;
	errno_t rc;
	int rv;

	rr = calloc(1, sizeof(riffr_t));
	if (rr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rr->pos = 0;

	rr->f = fopen(fname, "rb");
	if (rr->f == NULL) {
		rc = EIO;
		goto error;
	}

	rv = fseek(rr->f, 0, SEEK_END);
	if (rv < 0) {
		rc = EIO;
		goto error;
	}

	fsize = ftell(rr->f);
	if (fsize < 0) {
		rc = EIO;
		goto error;
	}

	rv = fseek(rr->f, 0, SEEK_SET);
	if (rv < 0) {
		rc = EIO;
		goto error;
	}

	fchunk.riffr = rr;
	fchunk.ckstart = 0;
	fchunk.cksize = fsize;

	rc = riff_rchunk_start(&fchunk, riffck);
	if (rc != EOK)
		goto error;

	*rrr = rr;
	return EOK;
error:
	if (rr != NULL && rr->f != NULL)
		fclose(rr->f);
	free(rr);
	return rc;
}

/** Close RIFF for reading.
 *
 * @param rr RIFF reader
 * @return EOK on success, EIO on error.
 */
errno_t riff_rclose(riffr_t *rr)
{
	errno_t rc;

	rc = fclose(rr->f);
	free(rr);
	return rc == 0 ? EOK : EIO;
}

/** Read uint32_t from RIFF file.
 *
 * @param rchunk RIFF chunk
 * @param v  Place to store value
 * @return EOK on success, ELIMIT if at the end of parent chunk, EIO on error.
 */
errno_t riff_read_uint32(riff_rchunk_t *rchunk, uint32_t *v)
{
	uint32_t vle;
	errno_t rc;
	size_t nread;

	rc = riff_read(rchunk, &vle, sizeof(vle), &nread);
	if (rc != EOK)
		return rc;

	if (nread != sizeof(vle))
		return ELIMIT;

	*v = uint32_t_le2host(vle);
	return EOK;
}

/** Start reading RIFF chunk.
 *
 * @param parent Parent chunk
 * @param rchunk Pointer to chunk structure to fill in
 *
 * @return EOK on success, ELIMIT if at the end of parent chunk,
 *         EIO on error.
 */
errno_t riff_rchunk_start(riff_rchunk_t *parent, riff_rchunk_t *rchunk)
{
	errno_t rc;

	rchunk->riffr = parent->riffr;
	rchunk->ckstart = parent->riffr->pos + 8;
	rc = riff_read_uint32(parent, &rchunk->ckid);
	if (rc != EOK)
		goto error;
	rc = riff_read_uint32(parent, &rchunk->cksize);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Find and start reading RIFF chunk of with specific chunk ID.
 * Other types of chunks are skipped.
 *
 * @param parent Parent chunk
 * @param rchunk Pointer to chunk structure to fill in
 *
 * @return EOK on success, ENOENT chunk was not found and end was reached
 *         EIO on error.
 */
errno_t riff_rchunk_match(riff_rchunk_t *parent, riff_ckid_t ckid,
    riff_rchunk_t *rchunk)
{
	errno_t rc;

	while (true) {
		rc = riff_rchunk_start(parent, rchunk);
		if (rc == ELIMIT)
			return ENOENT;
		if (rc != EOK)
			return rc;

		if (rchunk->ckid == ckid)
			break;

		rc = riff_rchunk_end(rchunk);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Find and start reading RIFF LIST chunk of specified type.
 * Other chunks or LIST chunks of other type are skipped.
 *
 * @param parent Parent chunk
 * @param rchunk Pointer to chunk structure to fill in
 *
 * @return EOK on success, ENOENT chunk was not found and end was reached
 *         EIO on error.
 */
errno_t riff_rchunk_list_match(riff_rchunk_t *parent, riff_ltype_t ltype,
    riff_rchunk_t *rchunk)
{
	errno_t rc;
	riff_ltype_t rltype;

	while (true) {
		rc = riff_rchunk_match(parent, CKID_LIST, rchunk);
		if (rc == ELIMIT)
			return ENOENT;
		if (rc != EOK)
			return rc;

		rc = riff_read_uint32(parent, &rltype);
		if (rc != EOK)
			return rc;

		if (rltype == ltype)
			break;

		rc = riff_rchunk_end(rchunk);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Seek to position in chunk.
 *
 * @param rchunk RIFF chunk
 * @param offset Offset
 * @param whence SEEK_SET, SEEK_CUR or SEEK_END
 * @return EOK on success or an error code
 */
errno_t riff_rchunk_seek(riff_rchunk_t *rchunk, long offset, int whence)
{
	long dest;
	int rv;

	switch (whence) {
	case SEEK_SET:
		dest = rchunk->ckstart + offset;
		break;
	case SEEK_END:
		dest = rchunk->ckstart + rchunk->cksize + offset;
		break;
	case SEEK_CUR:
		dest = rchunk->riffr->pos + offset;
		break;
	default:
		return EINVAL;
	}

	if (dest < rchunk->ckstart || (unsigned long) dest >
	    (unsigned long) rchunk->ckstart + rchunk->cksize) {
		return ELIMIT;
	}

	rv = fseek(rchunk->riffr->f, dest, SEEK_SET);
	if (rv < 0)
		return EIO;

	rchunk->riffr->pos = dest;
	return EOK;
}

/** Return chunk data size.
 *
 * @param rchunk RIFF chunk
 * @return Pure data size (excluding type+size header) in bytes
 */
uint32_t riff_rchunk_size(riff_rchunk_t *rchunk)
{
	return rchunk->cksize;
}

/** Return file offset where chunk ends
 *
 * @param rchunk RIFF chunk
 * @return File offset just after last data byte of the chunk
 */
static long riff_rchunk_get_end(riff_rchunk_t *rchunk)
{
	return rchunk->ckstart + rchunk->cksize;
}

/** Return file offset of first (non-padding) byte after end of chunk.
 *
 * @param rchunk RIFF chunk
 * @return File offset of first non-padding byte after end of chunk
 */
static long riff_rchunk_get_ndpos(riff_rchunk_t *rchunk)
{
	long ckend;

	ckend = riff_rchunk_get_end(rchunk);
	if ((ckend % 2) != 0)
		return ckend + 1;
	else
		return ckend;
}

/** Finish reading RIFF chunk.
 *
 * Seek to the first byte after end of chunk. It is allowed, though,
 * to return to the chunk later, e.g. using riff_rchunk_seek(@a rchunk, ..).
 *
 * @param rchunk Chunk structure
 * @return EOK on success, EIO on error.
 */
errno_t riff_rchunk_end(riff_rchunk_t *rchunk)
{
	long ckend;
	uint8_t byte;
	size_t nread;

	ckend = riff_rchunk_get_ndpos(rchunk);
	if (rchunk->riffr->pos < ckend &&
	    ckend <= rchunk->riffr->pos + 512) {
		/* (Buffered) reading is faster than seeking */
		while (rchunk->riffr->pos < ckend) {
			nread = fread(&byte, 1, sizeof(byte), rchunk->riffr->f);
			if (nread != sizeof(byte))
				return EIO;

			rchunk->riffr->pos += sizeof(byte);
		}

	} else if (rchunk->riffr->pos != ckend) {
		/* Need to seek (backwards or too far) */
		if (fseek(rchunk->riffr->f, ckend, SEEK_SET) < 0)
			return EIO;

		rchunk->riffr->pos = ckend;
	}

	return EOK;
}

/** Read data from RIFF chunk.
 *
 * Attempt to read @a bytes bytes from the chunk. If there is less data
 * left until the end of the chunk, less will be read. The actual number
 * of bytes read is returned in @a *nbytes (can even be 0).
 *
 * @param rchunk RIFF chunk for reading
 * @param buf Buffer to read to
 * @param bytes Number of bytes to read
 * @param nread Place to store number of bytes actually read
 *
 * @return EOK on success, ELIMIT if file position is not within @a rchunk,
 *         EIO on I/O error.
 */
errno_t riff_read(riff_rchunk_t *rchunk, void *buf, size_t bytes,
    size_t *nread)
{
	long pos;
	long ckend;
	long toread;

	pos = rchunk->riffr->pos;

	ckend = riff_rchunk_get_end(rchunk);
	if (pos < rchunk->ckstart || pos > ckend)
		return ELIMIT;

	toread = min(bytes, (size_t)(ckend - pos));
	if (toread == 0) {
		*nread = 0;
		return EOK;
	}

	*nread = fread(buf, 1, toread, rchunk->riffr->f);
	if (*nread == 0)
		return EIO;

	rchunk->riffr->pos += *nread;
	return EOK;
}

/** @}
 */
