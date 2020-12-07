/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Coordinates
 */

#include <gfx/coord.h>
#include <macros.h>
#include <stdbool.h>
#include <stddef.h>

/** Divide @a a by @a b and round towards negative numbers.
 *
 * Regular integer division always rounds towards zero. This is not useful
 * e.g. for scaling down, where we always need to round towards negative
 * numbers.
 *
 * @param a Dividend
 * @param b Divisor
 * @return Quotient
 */
gfx_coord_t gfx_coord_div_rneg(gfx_coord_t a, gfx_coord_t b)
{
	if ((a > 0 && b > 0) || (a < 0 && b < 0)) {
		/* Result is non-negative, round towards zero */
		return a / b;
	} else {
		/* Result is negative, round away from zero */
		return (a - b + 1) / b;
	}
}

/** Add two vectors.
 *
 * @param a First vector
 * @param b Second vector
 * @param d Destination
 */
void gfx_coord2_add(gfx_coord2_t *a, gfx_coord2_t *b, gfx_coord2_t *d)
{
	d->x = a->x + b->x;
	d->y = a->y + b->y;
}

/** Subtract two vectors.
 *
 * @param a First vector
 * @param b Second vector
 * @param d Destination
 */
void gfx_coord2_subtract(gfx_coord2_t *a, gfx_coord2_t *b, gfx_coord2_t *d)
{
	d->x = a->x - b->x;
	d->y = a->y - b->y;
}

/** Clip point coordinates to be within a rectangle.
 *
 * @param a Pixel coordinates
 * @param clip Clipping rectangle
 * @param d Place to store clipped coordinates
 */
void gfx_coord2_clip(gfx_coord2_t *a, gfx_rect_t *clip, gfx_coord2_t *d)
{
	gfx_rect_t sclip;
	gfx_coord2_t t;

	gfx_rect_points_sort(clip, &sclip);

	t.x = min(a->x, clip->p1.x - 1);
	t.y = min(a->y, clip->p1.y - 1);

	d->x = max(clip->p0.x, t.x);
	d->y = max(clip->p0.y, t.y);
}

/** Transform coordinates via rectangle to rectangle projection.
 *
 * Transform pixel coordinate via a projection that maps one rectangle
 * onto another rectangle. The source rectangle must have both dimensions
 * greater than one.
 *
 * @param a Pixel coordinates
 * @param srect Source rectangle
 * @param drect Destination rectangle
 * @param d Place to store resulting coordinates.
 */
void gfx_coord2_project(gfx_coord2_t *a, gfx_rect_t *srect, gfx_rect_t *drect,
    gfx_coord2_t *d)
{
	gfx_rect_t sr;
	gfx_rect_t dr;

	gfx_rect_points_sort(srect, &sr);
	gfx_rect_points_sort(drect, &dr);

	d->x = dr.p0.x + (a->x - sr.p0.x) * (dr.p1.x - dr.p0.x - 1) /
	    (sr.p1.x - sr.p0.x - 1);
	d->y = dr.p0.y + (a->y - sr.p0.y) * (dr.p1.y - dr.p0.y - 1) /
	    (sr.p1.y - sr.p0.y - 1);
}

/** Sort points of a span.
 *
 * Sort the begin and end points so that the begin point has the lower
 * coordinate (i.e. if needed, the span is transposed, if not, it is simply
 * copied).
 *
 * @param s0 Source span start point
 * @param s1 Source span end point
 * @param d0 Destination span start point
 * @param d1 Destination span end point
 */
void gfx_span_points_sort(gfx_coord_t s0, gfx_coord_t s1, gfx_coord_t *d0,
    gfx_coord_t *d1)
{
	if (s0 <= s1) {
		*d0 = s0;
		*d1 = s1;
	} else {
		*d0 = s1 + 1;
		*d1 = s0 + 1;
	}
}

/** Move (translate) rectangle.
 *
 * @param trans Translation offset
 * @param src Source rectangle
 * @param dest Destination rectangle
 */
void gfx_rect_translate(gfx_coord2_t *trans, gfx_rect_t *src, gfx_rect_t *dest)
{
	gfx_coord2_add(&src->p0, trans, &dest->p0);
	gfx_coord2_add(&src->p1, trans, &dest->p1);
}

/** Reverse move (translate) rectangle.
 *
 * @param trans Translation offset
 * @param src Source rectangle
 * @param dest Destination rectangle
 */
void gfx_rect_rtranslate(gfx_coord2_t *trans, gfx_rect_t *src, gfx_rect_t *dest)
{
	gfx_coord2_subtract(&src->p0, trans, &dest->p0);
	gfx_coord2_subtract(&src->p1, trans, &dest->p1);
}

/** Compute envelope of two rectangles.
 *
 * Envelope is the minimal rectangle covering all pixels of both rectangles.
 *
 * @param a First rectangle
 * @param b Second rectangle
 * @param dest Place to store enveloping rectangle
 */
void gfx_rect_envelope(gfx_rect_t *a, gfx_rect_t *b, gfx_rect_t *dest)
{
	gfx_rect_t sa, sb;

	if (gfx_rect_is_empty(a)) {
		*dest = *b;
		return;
	}

	if (gfx_rect_is_empty(b)) {
		*dest = *a;
		return;
	}

	/* a and b are both non-empty */

	gfx_rect_points_sort(a, &sa);
	gfx_rect_points_sort(b, &sb);

	dest->p0.x = min(sa.p0.x, sb.p0.x);
	dest->p0.y = min(sa.p0.y, sb.p0.y);
	dest->p1.x = max(sa.p1.x, sb.p1.x);
	dest->p1.y = max(sa.p1.y, sb.p1.y);
}

/** Compute intersection of two rectangles.
 *
 * If the two rectangles do not intersect, the result will be an empty
 * rectangle (check with gfx_rect_is_empty()). The resulting rectangle
 * is always sorted. If @a clip is NULL, no clipping is performed.
 *
 * @param rect Source rectangle
 * @param clip Clipping rectangle or @c NULL
 * @param dest Place to store clipped rectangle
 */
void gfx_rect_clip(gfx_rect_t *rect, gfx_rect_t *clip, gfx_rect_t *dest)
{
	gfx_rect_t srect, sclip;

	if (clip == NULL) {
		*dest = *rect;
		return;
	}

	gfx_rect_points_sort(rect, &srect);
	gfx_rect_points_sort(clip, &sclip);

	dest->p0.x = min(max(srect.p0.x, sclip.p0.x), sclip.p1.x);
	dest->p0.y = min(max(srect.p0.y, sclip.p0.y), sclip.p1.y);
	dest->p1.x = max(sclip.p0.x, min(srect.p1.x, sclip.p1.x));
	dest->p1.y = max(sclip.p0.y, min(srect.p1.y, sclip.p1.y));
}

/** Center rectangle on rectangle.
 *
 * Translate rectangle @a a so that its center coincides with the
 * center of rectangle @a b, saving the result in @a dest.
 *
 * @param a Rectnagle to translate
 * @param b Rectangle on which to center
 * @param dest Place to store resulting rectangle
 */
void gfx_rect_ctr_on_rect(gfx_rect_t *a, gfx_rect_t *b, gfx_rect_t *dest)
{
	gfx_coord2_t adim;
	gfx_coord2_t bdim;

	gfx_rect_dims(a, &adim);
	gfx_rect_dims(b, &bdim);

	dest->p0.x = b->p0.x + bdim.x / 2 - adim.x / 2;
	dest->p0.y = b->p0.y + bdim.y / 2 - adim.y / 2;

	dest->p1.x = dest->p0.x + adim.x;
	dest->p1.y = dest->p0.y + adim.y;
}

/** Sort points of a rectangle.
 *
 * Shuffle around coordinates of a rectangle so that p0.x < p1.x and
 * p0.y < p0.y.
 *
 * @param src Source rectangle
 * @param dest Destination (sorted) rectangle
 */
void gfx_rect_points_sort(gfx_rect_t *src, gfx_rect_t *dest)
{
	gfx_span_points_sort(src->p0.x, src->p1.x, &dest->p0.x, &dest->p1.x);
	gfx_span_points_sort(src->p0.y, src->p1.y, &dest->p0.y, &dest->p1.y);
}

/** Determine if rectangle contains no pixels
 *
 * @param rect Rectangle
 * @return @c true iff rectangle contains no pixels
 */
bool gfx_rect_is_empty(gfx_rect_t *rect)
{
	return rect->p0.x == rect->p1.x || rect->p0.y == rect->p1.y;
}

/** Determine if two rectangles share any pixels
 *
 * @param a First rectangle
 * @param b Second rectangle
 * @return @c true iff rectangles share any pixels
 */
bool gfx_rect_is_incident(gfx_rect_t *a, gfx_rect_t *b)
{
	gfx_rect_t r;

	gfx_rect_clip(a, b, &r);
	return !gfx_rect_is_empty(&r);
}

/** Return true if rectangle @a a is contained in rectangle @a b.
 *
 * @param a Inside rectangle
 * @param b Outside rectangle
 * @return @c true iff @a a is (non-strictly) inside @a b
 */
bool gfx_rect_is_inside(gfx_rect_t *a, gfx_rect_t *b)
{
	gfx_rect_t sa;
	gfx_rect_t sb;

	gfx_rect_points_sort(a, &sa);
	gfx_rect_points_sort(b, &sb);

	if (sa.p0.x < sb.p0.x || sa.p0.y < sb.p0.y)
		return false;

	if (sa.p1.x > sb.p1.x || sa.p1.y > sb.p1.y)
		return false;

	return true;
}

/** Get rectangle dimensions.
 *
 * Get a vector containing the x, y dimensions of a rectangle. These are
 * always nonnegative.
 *
 * @param rect Rectangle
 * @param dims Place to store dimensions
 */
void gfx_rect_dims(gfx_rect_t *rect, gfx_coord2_t *dims)
{
	gfx_rect_t srect;

	gfx_rect_points_sort(rect, &srect);
	gfx_coord2_subtract(&srect.p1, &srect.p0, dims);
}

/** Return true if pixel at coordinate @a coord lies within rectangle @a rect. */
bool gfx_pix_inside_rect(gfx_coord2_t *coord, gfx_rect_t *rect)
{
	gfx_rect_t sr;

	gfx_rect_points_sort(rect, &sr);

	if (coord->x < sr.p0.x || coord->y < sr.p0.y)
		return false;

	if (coord->x >= sr.p1.x || coord->y >= sr.p1.y)
		return false;

	return true;
}

/** @}
 */
