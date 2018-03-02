/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <gzip.h>
#include <stdlib.h>
#include "tga.gz.h"
#include "tga.h"

/** Decode gzipped Truevision TGA format
 *
 * Decode gzipped Truevision TGA format and create a surface
 * from it. The supported variants of TGA are limited those
 * supported by decode_tga().
 *
 * @param[in] data  Memory representation of gzipped TGA.
 * @param[in] size  Size of the representation (in bytes).
 * @param[in] flags Surface creation flags.
 *
 * @return Newly allocated surface with the decoded content.
 * @return NULL on error or unsupported format.
 *
 */
surface_t *decode_tga_gz(void *data, size_t size, surface_flags_t flags)
{
	void *data_expanded;
	size_t size_expanded;

	errno_t ret = gzip_expand(data, size, &data_expanded, &size_expanded);
	if (ret != EOK)
		return NULL;

	surface_t *surface = decode_tga(data_expanded, size_expanded, flags);

	free(data_expanded);
	return surface;
}

/** Encode gzipped Truevision TGA format
 *
 * Encode gzipped Truevision TGA format into an array.
 *
 * @param[in]  surface Surface to be encoded into gzipped TGA.
 * @param[out] pdata   Pointer to the resulting array.
 * @param[out] psize   Pointer to the size of the resulting array.
 *
 * @return True on succesful encoding.
 * @return False on failure.
 *
 */
bool encode_tga_gz(surface_t *surface, void **pdata, size_t *psize)
{
	// TODO
	return false;
}

/** @}
 */
