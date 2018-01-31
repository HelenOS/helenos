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

/** @addtogroup libposix
 * @{
 */
/** @file Implementation of the scanf backend.
 */

#include <assert.h>

#include <errno.h>

#include "posix/stdio.h"
#include "posix/stdlib.h"
#include "posix/stddef.h"
#include "posix/string.h"
#include "posix/ctype.h"
#include "posix/sys/types.h"

#include "../internal/common.h"
#include "libc/malloc.h"
#include "libc/stdbool.h"

/** Unified data type for possible data sources for scanf. */
typedef union __data_source {
	FILE *stream; /**< Input file stream. */
	const char *string; /**< Input string. */
} _data_source;

/** Internal state of the input provider. */
enum {
	/** Partly constructed but not yet functional. */
	_PROV_CONSTRUCTED,
	/** Ready to serve any request. */
	_PROV_READY,
	/** Cursor is temporarily lent to the external entity. No action is
	  * possible until the cursor is returned.  */
	_PROV_CURSOR_LENT,
};

/** Universal abstraction over data input for scanf. */
typedef struct __input_provider {
	/** Source of data elements. */
	_data_source source;
	/** How many elements was already processed. */
	int consumed;
	/** How many elements was already fetched from the source. */
	int fetched;
	/** Elements are fetched from the source in batches (e.g. by getline())
	  * to allow using strtol/strtod family even on streams. */
	char *window;
	/** Size of the current window. */
	size_t window_size;
	/** Points to the next element to be processed inside the current window. */
	const char *cursor;
	/** Internal state of the provider. */
	int state;

	/** Take control over data source. Finish initialization of the internal
	  * structures (e.g. allocation of window). */
	void (*capture)(struct __input_provider *);
	/** Get a single element from the source and update the internal structures
	  * accordingly (e.g. greedy update of the window). Return -1 if the
	  * element cannot be obtained. */
	int (*pop)(struct __input_provider *);
	/** Undo the most recent not-undone pop operation. Might be necesarry to
	  * flush current window and seek data source backwards. Return 0 if the
	  * pop history is exhausted, non-zero on success. */
	int (*undo)(struct __input_provider *);
	/** Lend the cursor to the caller.  */
	const char * (*borrow_cursor)(struct __input_provider *);
	/** Take control over possibly incremented cursor and update the internal
	  * structures if necessary. */
	void (*return_cursor)(struct __input_provider *, const char *);
	/** Release the control over the source. That is, synchronize any
	  * fetched but non-consumed elements (e.g. by seeking) and destruct
	  * internal structures (e.g. window deallocation). */
	void (*release)(struct __input_provider *);
} _input_provider;

/** @see __input_provider */
static void _capture_stream(_input_provider *self)
{
	assert(self->source.stream);
	assert(self->state == _PROV_CONSTRUCTED);
	/* Caller could already pre-allocated the window. */
	assert((self->window == NULL && self->window_size == 0) ||
	    (self->window && self->window_size > 0));

	/* Initialize internal structures. */
	self->consumed = 0;
	ssize_t fetched = getline(
	    &self->window, &self->window_size, self->source.stream);
	if (fetched != -1) {
		self->fetched = fetched;
		self->cursor = self->window;
	} else {
		/* EOF encountered. */
		self->fetched = 0;
		self->cursor = NULL;
	}
	self->state = _PROV_READY;
}

/** @see __input_provider */
static void _capture_string(_input_provider *self)
{
	assert(self->source.string);
	assert(self->state == _PROV_CONSTRUCTED);

	/* Initialize internal structures. */
	self->consumed = 0;
	self->fetched = strlen(self->source.string);
	self->window = (char *) self->source.string;
	self->window_size = self->fetched + 1;
	self->cursor = self->window;
	self->state = _PROV_READY;
}

/** @see __input_provider */
static int _pop_stream(_input_provider *self)
{
	assert(self->state == _PROV_READY);

	if (self->cursor) {
		int c = *self->cursor;
		++self->consumed;
		++self->cursor;
		/* Do we need to fetch a new line from the source? */
		if (*self->cursor == '\0') {
			ssize_t fetched = getline(&self->window,
			    &self->window_size, self->source.stream);
			if (fetched != -1) {
				self->fetched += fetched;
				self->cursor = self->window;
			} else {
				/* EOF encountered. */
				self->cursor = NULL;
			}
		}
		return c;
	} else {
		/* Already at EOF. */
		return -1;
	}
}

/** @see __input_provider */
static int _pop_string(_input_provider *self)
{
	assert(self->state == _PROV_READY);

	if (*self->cursor != '\0') {
		int c = *self->cursor;
		++self->consumed;
		++self->cursor;
		return c;
	} else {
		/* String depleted. */
		return -1;
	}
}

/** @see __input_provider */
static int _undo_stream(_input_provider *self)
{
	assert(self->state == _PROV_READY);

	if (self->consumed == 0) {
		/* Undo history exhausted. */
		return 0;
	}

	if (!self->cursor || self->window == self->cursor) {
		/* Complex case. Either at EOF (cursor == NULL) or there is no more
		 * place to retreat to inside the window. Seek the source backwards
		 * and flush the window. Regarding the scanf, this could happend only
		 * when matching unbounded string (%s) or unbounded scanset (%[) not
		 * containing newline, while at the same time newline is the character
		 * that breaks the matching process. */
		int rc = fseek(self->source.stream, -1, SEEK_CUR);
		if (rc == -1) {
			/* Seek failed.  */
			return 0;
		}
		ssize_t fetched = getline(&self->window,
		    &self->window_size, self->source.stream);
		if (fetched != -1) {
			assert(fetched == 1);
			self->fetched = self->consumed + 1;
			self->cursor = self->window;
		} else {
			/* Stream is broken. */
			return 0;
		}
	} else {
		/* Simple case. Still inside window. */
		--self->cursor;
	}
	--self->consumed;
	return 1; /* Success. */
}

/** @see __input_provider */
static int _undo_string(_input_provider *self)
{
	assert(self->state == _PROV_READY);

	if (self->consumed > 0) {
		--self->consumed;
		--self->cursor;
	} else {
		/* Undo history exhausted. */
		return 0;
	}
	return 1; /* Success. */
}

/** @see __input_provider */
static const char *_borrow_cursor_universal(_input_provider *self)
{
	assert(self->state == _PROV_READY);

	self->state = _PROV_CURSOR_LENT;
	return self->cursor;
}

/** @see __input_provider */
static void _return_cursor_stream(_input_provider *self, const char *cursor)
{
	assert(self->state == _PROV_CURSOR_LENT);

	/* Check how much of the window did external entity consumed. */
	self->consumed += cursor - self->cursor;
	self->cursor = cursor;
	if (*self->cursor == '\0') {
		/* Window was completely consumed, fetch new data. */
		ssize_t fetched = getline(&self->window,
		    &self->window_size, self->source.stream);
		if (fetched != -1) {
			self->fetched += fetched;
			self->cursor = self->window;
		} else {
			/* EOF encountered. */
			self->cursor = NULL;
		}
	}
	self->state = _PROV_READY;
}

/** @see __input_provider */
static void _return_cursor_string(_input_provider *self, const char *cursor)
{
	assert(self->state == _PROV_CURSOR_LENT);

	/* Check how much of the window did external entity consumed. */
	self->consumed += cursor - self->cursor;
	self->cursor = cursor;
	self->state = _PROV_READY;
}

/** @see __input_provider */
static void _release_stream(_input_provider *self)
{
	assert(self->state == _PROV_READY);
	assert(self->consumed >= self->fetched);

	/* Try to correct the difference between the stream position and what was
	 * actually consumed. If it is not possible, continue anyway. */
	fseek(self->source.stream, self->consumed - self->fetched, SEEK_CUR);

	/* Destruct internal structures. */
	self->fetched = 0;
	self->cursor = NULL;
	if (self->window) {
		free(self->window);
		self->window = NULL;
	}
	self->window_size = 0;
	self->state = _PROV_CONSTRUCTED;
}

/** @see __input_provider */
static void _release_string(_input_provider *self)
{
	assert(self->state == _PROV_READY);

	/* Destruct internal structures. */
	self->fetched = 0;
	self->cursor = NULL;
	self->window = NULL;
	self->window_size = 0;
	self->state = _PROV_CONSTRUCTED;
}

/** Length modifier values. */
enum {
	LMOD_NONE,
	LMOD_hh,
	LMOD_h,
	LMOD_l,
	LMOD_ll,
	LMOD_j,
	LMOD_z,
	LMOD_t,
	LMOD_L,
	LMOD_p, /* Reserved for %p conversion. */
};

/**
 * Decides whether provided characters specify length modifier. If so, the
 * recognized modifier is stored through provider pointer.
 *
 * @param c Candidate on the length modifier.
 * @param _c Next character (might be NUL).
 * @param modifier Pointer to the modifier value.
 * @return Whether the modifier was recognized or not.
 */
static inline int is_length_mod(int c, int _c, int *modifier)
{
	assert(modifier);

	switch (c) {
	case 'h':
		/* Check whether the modifier was not already recognized. */
		if (*modifier == LMOD_NONE) {
			*modifier = _c == 'h' ? LMOD_hh : LMOD_h;
		} else {
			/* Format string is invalid. Notify the caller. */
			*modifier = LMOD_NONE;
		}
		return 1;
	case 'l':
		if (*modifier == LMOD_NONE) {
			*modifier = _c == 'l' ? LMOD_ll : LMOD_l;
		} else {
			*modifier = LMOD_NONE;
		}
		return 1;
	case 'j':
		*modifier = *modifier == LMOD_NONE ? LMOD_j : LMOD_NONE;
		return 1;
	case 'z':
		*modifier = *modifier == LMOD_NONE ? LMOD_z : LMOD_NONE;
		return 1;
	case 't':
		*modifier = *modifier == LMOD_NONE ? LMOD_t : LMOD_NONE;
		return 1;
	case 'L':
		*modifier = *modifier == LMOD_NONE ? LMOD_L : LMOD_NONE;
		return 1;
	default:
		return 0;
	}
}

/**
 * Decides whether provided character specifies integer conversion. If so, the
 * semantics of the conversion is stored through provided pointers..
 * 
 * @param c Candidate on the integer conversion.
 * @param is_unsigned Pointer to store whether the conversion is signed or not.
 * @param base Pointer to store the base of the integer conversion.
 * @return Whether the conversion was recognized or not.
 */
static inline int is_int_conv(int c, bool *is_unsigned, int *base)
{
	assert(is_unsigned && base);

	switch (c) {
	case 'd':
		*is_unsigned = false;
		*base = 10;
		return 1;
	case 'i':
		*is_unsigned = false;
		*base = 0;
		return 1;
	case 'o':
		*is_unsigned = true;
		*base = 8;
		return 1;
	case 'u':
		*is_unsigned = true;
		*base = 10;
		return 1;
	case 'p': /* According to POSIX, %p modifier is implementation defined but
			   * must correspond to its printf counterpart. */
	case 'x':
	case 'X':
		*is_unsigned = true;
		*base = 16;
		return 1;
		return 1;
	default:
		return 0;
	}
}

/**
 * Decides whether provided character specifies conversion of the floating
 * point number.
 *
 * @param c Candidate on the floating point conversion.
 * @return Whether the conversion was recognized or not.
 */
static inline int is_float_conv(int c)
{
	switch (c) {
	case 'a':
	case 'A':
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
		return 1;
	default:
		return 0;
	}
}

/**
 * Decides whether provided character specifies conversion of the character
 * sequence.
 *
 * @param c Candidate on the character sequence conversion.
 * @param modifier Pointer to store length modifier for wide chars.
 * @return Whether the conversion was recognized or not.
 */
static inline int is_seq_conv(int c, int *modifier)
{
	assert(modifier);
	
	switch (c) {
	case 'S':
		*modifier = LMOD_l;
		/* Fallthrough */
	case 's':
		return 1;
	case 'C':
		*modifier = LMOD_l;
		/* Fallthrough */
	case 'c':
		return 1;
	case '[':
		return 1;
	default:
		return 0;
	}
}

/**
 * Backend for the whole family of scanf functions. Uses input provider
 * to abstract over differences between strings and streams. Should be
 * POSIX compliant (apart from the not supported stuff).
 *
 * NOT SUPPORTED: locale (see strtold), wide chars, numbered output arguments
 * 
 * @param in Input provider.
 * @param fmt Format description.
 * @param arg Output arguments.
 * @return The number of converted output items or EOF on failure.
 */
static inline int _internal_scanf(
    _input_provider *in, const char *restrict fmt, va_list arg)
{
	int c = -1;
	int converted_cnt = 0;
	bool converting = false;
	bool matching_failure = false;

	bool assign_supress = false;
	bool assign_alloc = false;
	long width = -1;
	int length_mod = LMOD_NONE;
	bool int_conv_unsigned = false;
	int int_conv_base = 0;

	/* Buffers allocated by scanf for optional 'm' specifier must be remembered
	 * to deallocaate them in case of an error. Because each of those buffers
	 * corresponds to one of the argument from va_list, there is an upper bound
	 * on the number of those arguments. In case of C99, this uppper bound is
	 * 127 arguments. */
	char *buffers[127];
	for (int i = 0; i < 127; ++i) {
		buffers[i] = NULL;
	}
	int next_unused_buffer_idx = 0;

	in->capture(in);

	/* Interpret format string. Control shall prematurely jump from the cycle
	 * on input failure, matching failure or illegal format string. In order
	 * to keep error reporting simple enough and to keep input consistent,
	 * error condition shall be always manifested as jump from the cycle,
	 * not function return. Format string pointer shall be updated specifically
	 * for each sub-case (i.e. there shall be no loop-wide increment).*/
	while (*fmt) {

		if (converting) {

			/* Processing inside conversion specifier. Either collect optional
			 * parameters or execute the conversion. When the conversion
			 * is successfully completed, increment conversion count and switch
			 * back to normal mode. */
			if (*fmt == '*') {
				/* Assignment-supression (optional). */
				if (assign_supress) {
					/* Already set. Illegal format string. */
					break;
				}
				assign_supress = true;
				++fmt;
			} else if (*fmt == 'm') {
				/* Assignment-allocation (optional). */
				if (assign_alloc) {
					/* Already set. Illegal format string. */
					break;
				}
				assign_alloc = true;
				++fmt;
			} else if (*fmt == '$') {
				/* Reference to numbered output argument. */
				// TODO
				not_implemented();
			} else if (isdigit(*fmt)) {
				/* Maximum field length (optional). */
				if (width != -1) {
					/* Already set. Illegal format string. */
					break;
				}
				char *fmt_new = NULL;
				width = strtol(fmt, &fmt_new, 10);
				if (width != 0) {
					fmt = fmt_new;
				} else {
					/* Since POSIX requires width to be non-zero, it is
					 * sufficient to interpret zero width as error without
					 * referring to errno. */
					break;
				}
			} else if (is_length_mod(*fmt, *(fmt + 1), &length_mod)) {
				/* Length modifier (optional). */
				if (length_mod == LMOD_NONE) {
					/* Already set. Illegal format string. The actual detection
					 * is carried out in the is_length_mod(). */
					break;
				}
				if (length_mod == LMOD_hh || length_mod == LMOD_ll) {
					/* Modifier was two characters long. */
					++fmt;
				}
				++fmt;
			} else if (is_int_conv(*fmt, &int_conv_unsigned, &int_conv_base)) {
				/* Integer conversion. */

				/* Check sanity of optional parts of conversion specifier. */
				if (assign_alloc || length_mod == LMOD_L) {
					/* Illegal format string. */
					break;
				}

				/* Conversion of the integer with %p specifier needs special
				 * handling, because it is not allowed to have arbitrary
				 * length modifier.  */
				if (*fmt == 'p') {
					if (length_mod == LMOD_NONE) {
						length_mod = LMOD_p;
					} else {
						/* Already set. Illegal format string. */
						break;
					}
				}

				/* First consume any white spaces, so we can borrow cursor
				 * from the input provider. This way, the cursor will either
				 * point to the non-white space while the input will be
				 * prefetched up to the newline (which is suitable for strtol),
				 * or the input will be at EOF. */
				do {
					c = in->pop(in);
				} while (isspace(c));

				/* After skipping the white spaces, can we actually continue? */
				if (c == -1) {
					/* Input failure. */
					break;
				} else {
					/* Everything is OK, just undo the last pop, so the cursor
					 * can be borrowed. */
					in->undo(in);
				}

				const char *cur_borrowed = NULL;
				char *cur_duplicated = NULL;
				const char *cur_limited = NULL;
				const char *cur_updated = NULL;

				/* Borrow the cursor. Until it is returned to the provider
				 * we cannot jump from the cycle, because it would leave
				 * the input inconsistent. */
				cur_borrowed = in->borrow_cursor(in);

				/* If the width is limited, the cursor horizont must be
				 * decreased accordingly. Otherwise the strtol could read more
				 * than allowed by width. */
				if (width != -1) {
					cur_duplicated = strndup(cur_borrowed, width);
					cur_limited = cur_duplicated;
				} else {
					cur_limited = cur_borrowed;
				}
				cur_updated = cur_limited;

				long long sres = 0;
				unsigned long long ures = 0;
				errno = 0; /* Reset errno to recognize error later. */
				/* Try to convert the integer. */
				if (int_conv_unsigned) {
					ures = strtoull(cur_limited, (char **) &cur_updated, int_conv_base);
				} else {
					sres = strtoll(cur_limited, (char **) &cur_updated, int_conv_base);
				}

				/* Update the cursor so it can be returned to the provider. */
				cur_borrowed += cur_updated - cur_limited;
				if (cur_duplicated != NULL) {
					/* Deallocate duplicated part of the cursor view. */
					free(cur_duplicated);
				}
				cur_limited = NULL;
				cur_updated = NULL;
				cur_duplicated = NULL;
				/* Return the cursor to the provider. Input consistency is again
				 * the job of the provider, so we can report errors from
				 * now on. */
				in->return_cursor(in, cur_borrowed);
				cur_borrowed = NULL;

				/* Check whether the conversion was successful. */
				if (errno != EOK) {
					matching_failure = true;
					break;
				}

				/* If not supressed, assign the converted integer into
				 * the next output argument. */
				if (!assign_supress) {
					if (int_conv_unsigned) {
						switch (length_mod) {
						case LMOD_hh: ; /* Label cannot be part of declaration. */
							unsigned char *phh = va_arg(arg, unsigned char *);
							*phh = (unsigned char) ures;
							break;
						case LMOD_h: ;
							unsigned short *ph = va_arg(arg, unsigned short *);
							*ph = (unsigned short) ures;
							break;
						case LMOD_NONE: ;
							unsigned *pdef = va_arg(arg, unsigned *);
							*pdef = (unsigned) ures;
							break;
						case LMOD_l: ;
							unsigned long *pl = va_arg(arg, unsigned long *);
							*pl = (unsigned long) ures;
							break;
						case LMOD_ll: ;
							unsigned long long *pll = va_arg(arg, unsigned long long *);
							*pll = (unsigned long long) ures;
							break;
						case LMOD_j: ;
							uintmax_t *pj = va_arg(arg, uintmax_t *);
							*pj = (uintmax_t) ures;
							break;
						case LMOD_z: ;
							size_t *pz = va_arg(arg, size_t *);
							*pz = (size_t) ures;
							break;
						case LMOD_t: ;
							// XXX: What is unsigned counterpart of the ptrdiff_t?
							size_t *pt = va_arg(arg, size_t *);
							*pt = (size_t) ures;
							break;
						case LMOD_p: ;
							void **pp = va_arg(arg, void **);
							*pp = (void *) (uintptr_t) ures;
							break;
						default:
							assert(false);
						}
					} else {
						switch (length_mod) {
						case LMOD_hh: ; /* Label cannot be part of declaration. */
							signed char *phh = va_arg(arg, signed char *);
							*phh = (signed char) sres;
							break;
						case LMOD_h: ;
							short *ph = va_arg(arg, short *);
							*ph = (short) sres;
							break;
						case LMOD_NONE: ;
							int *pdef = va_arg(arg, int *);
							*pdef = (int) sres;
							break;
						case LMOD_l: ;
							long *pl = va_arg(arg, long *);
							*pl = (long) sres;
							break;
						case LMOD_ll: ;
							long long *pll = va_arg(arg, long long *);
							*pll = (long long) sres;
							break;
						case LMOD_j: ;
							intmax_t *pj = va_arg(arg, intmax_t *);
							*pj = (intmax_t) sres;
							break;
						case LMOD_z: ;
							ssize_t *pz = va_arg(arg, ssize_t *);
							*pz = (ssize_t) sres;
							break;
						case LMOD_t: ;
							ptrdiff_t *pt = va_arg(arg, ptrdiff_t *);
							*pt = (ptrdiff_t) sres;
							break;
						default:
							assert(false);
						}
					}
					++converted_cnt;
				}

				converting = false;
				++fmt;
			} else if (is_float_conv(*fmt)) {
				/* Floating point number conversion. */

				/* Check sanity of optional parts of conversion specifier. */
				if (assign_alloc) {
					/* Illegal format string. */
					break;
				}
				if (length_mod != LMOD_NONE &&
				    length_mod != LMOD_l &&
				    length_mod != LMOD_L) {
					/* Illegal format string. */
					break;
				}

				/* First consume any white spaces, so we can borrow cursor
				 * from the input provider. This way, the cursor will either
				 * point to the non-white space while the input will be
				 * prefetched up to the newline (which is suitable for strtof),
				 * or the input will be at EOF. */
				do {
					c = in->pop(in);
				} while (isspace(c));

				/* After skipping the white spaces, can we actually continue? */
				if (c == -1) {
					/* Input failure. */
					break;
				} else {
					/* Everything is OK, just undo the last pop, so the cursor
					 * can be borrowed. */
					in->undo(in);
				}

				const char *cur_borrowed = NULL;
				const char *cur_limited = NULL;
				char *cur_duplicated = NULL;
				const char *cur_updated = NULL;

				/* Borrow the cursor. Until it is returned to the provider
				 * we cannot jump from the cycle, because it would leave
				 * the input inconsistent. */
				cur_borrowed = in->borrow_cursor(in);

				/* If the width is limited, the cursor horizont must be
				 * decreased accordingly. Otherwise the strtof could read more
				 * than allowed by width. */
				if (width != -1) {
					cur_duplicated = strndup(cur_borrowed, width);
					cur_limited = cur_duplicated;
				} else {
					cur_limited = cur_borrowed;
				}
				cur_updated = cur_limited;

				float fres = 0.0;
				double dres = 0.0;
				long double ldres = 0.0;
				errno = 0; /* Reset errno to recognize error later. */
				/* Try to convert the floating point nubmer. */
				switch (length_mod) {
				case LMOD_NONE:
					fres = strtof(cur_limited, (char **) &cur_updated);
					break;
				case LMOD_l:
					dres = strtod(cur_limited, (char **) &cur_updated);
					break;
				case LMOD_L:
					ldres = strtold(cur_limited, (char **) &cur_updated);
					break;
				default:
					assert(false);
				}

				/* Update the cursor so it can be returned to the provider. */
				cur_borrowed += cur_updated - cur_limited;
				if (cur_duplicated != NULL) {
					/* Deallocate duplicated part of the cursor view. */
					free(cur_duplicated);
				}
				cur_limited = NULL;
				cur_updated = NULL;
				/* Return the cursor to the provider. Input consistency is again
				 * the job of the provider, so we can report errors from
				 * now on. */
				in->return_cursor(in, cur_borrowed);
				cur_borrowed = NULL;

				/* Check whether the conversion was successful. */
				if (errno != EOK) {
					matching_failure = true;
					break;
				}

				/* If nto supressed, assign the converted floating point number
				 * into the next output argument. */
				if (!assign_supress) {
					switch (length_mod) {
					case LMOD_NONE: ; /* Label cannot be part of declaration. */
						float *pf = va_arg(arg, float *);
						*pf = fres;
						break;
					case LMOD_l: ;
						double *pd = va_arg(arg, double *);
						*pd = dres;
						break;
					case LMOD_L: ;
						long double *pld = va_arg(arg, long double *);
						*pld = ldres;
						break;
					default:
						assert(false);
					}
					++converted_cnt;
				}

				converting = false;
				++fmt;
			} else if (is_seq_conv(*fmt, &length_mod)) {
				/* Character sequence conversion. */
				
				/* Check sanity of optional parts of conversion specifier. */
				if (length_mod != LMOD_NONE &&
				    length_mod != LMOD_l) {
					/* Illegal format string. */
					break;
				}

				if (length_mod == LMOD_l) {
					/* Wide chars not supported. */
					// TODO
					not_implemented();
				}

				int term_size = 1; /* Size of the terminator (0 or 1)). */
				if (*fmt == 'c') {
					term_size = 0;
					width = width == -1 ? 1 : width;
				}

				if (*fmt == 's') {
					/* Skip white spaces. */
					do {
						c = in->pop(in);
					} while (isspace(c));
				} else {
					/* Fetch a single character. */
					c = in->pop(in);
				}

				/* Check whether there is still input to read. */
				if (c == -1) {
					/* Input failure. */
					break;
				}

				/* Prepare scanset. */
				char terminate_on[256];
				for (int i = 0; i < 256; ++i) {
					terminate_on[i] = 0;
				}
				if (*fmt == 'c') {
					++fmt;
				} else if (*fmt == 's') {
					terminate_on[' '] = 1;
					terminate_on['\n'] = 1;
					terminate_on['\t'] = 1;
					terminate_on['\f'] = 1;
					terminate_on['\r'] = 1;
					terminate_on['\v'] = 1;
					++fmt;
				} else {
					assert(*fmt == '[');
					bool not = false;
					bool dash = false;
					++fmt;
					/* Check for negation. */
					if (*fmt == '^') {
						not = true;
						++fmt;
					}
					/* Check for escape sequences. */
					if (*fmt == '-' || *fmt == ']') {
						terminate_on[(int) *fmt] = 1;
						++fmt;
					}
					/* Check for ordinary characters and ranges. */
					while (*fmt != '\0' && *fmt != ']') {
						if (dash) {
							for (char chr = *(fmt - 2); chr <= *fmt; ++chr) {
								terminate_on[(int) chr] = 1;
							}
							dash = false;
						} else if (*fmt == '-') {
							dash = true;
						} else {
							terminate_on[(int) *fmt] = 1;
						}
						++fmt;
					}
					/* Check for escape sequence. */
					if (dash == true) {
						terminate_on['-'] = 1;
					}
					/* Check whether the specifier was correctly terminated.*/
					if (*fmt == '\0') {
						/* Illegal format string. */
						break;
					} else {
						++fmt;
					}
					/* Inverse the scanset if necessary. */
					if (not == false) {
						for (int i = 0; i < 256; ++i) {
							terminate_on[i] = terminate_on[i] ? 0 : 1;
						}
					}
				}

				char * buf = NULL;
				size_t buf_size = 0;
				char * cur = NULL;
				size_t alloc_step = 80; /* Buffer size gain during reallocation. */
				int my_buffer_idx = 0;

				/* Retrieve the buffer into which popped characters
				 * will be stored. */
				if (!assign_supress) {
					if (assign_alloc) {
						/* We must allocate our own buffer. */
						buf_size =
						    width == -1 ? alloc_step : (size_t) width + term_size;
						buf = malloc(buf_size);
						if (!buf) {
							/* No memory. */
							break;
						}
						my_buffer_idx = next_unused_buffer_idx;
						++next_unused_buffer_idx;
						buffers[my_buffer_idx] = buf;
						cur = buf;
					} else {
						/* Caller provided its buffer. */
						buf = va_arg(arg, char *);
						cur = buf;
						buf_size =
						    width == -1 ? SIZE_MAX : (size_t) width + term_size;
					}
				}

				/* Match the string. The next character is already popped. */
				while ((width == -1 || width > 0) && c != -1 && !terminate_on[c]) {

					/* Check whether the buffer is still sufficiently large. */
					if (!assign_supress) {
						/* Always reserve space for the null terminator. */
						if (cur == buf + buf_size - term_size) {
							/* Buffer size must be increased. */
							buf = realloc(buf, buf_size + alloc_step);
							if (buf) {
								buffers[my_buffer_idx] = buf;
								cur = buf + buf_size - term_size;
								buf_size += alloc_step;
							} else {
								/* Break just from this tight loop. Errno will
								 * be checked after it. */
								break;
							}
						}
						/* Store the input character. */
						*cur = c;
					}

					width = width == -1 ? -1 : width - 1;
					++cur;
					c = in->pop(in);
				}
				if (errno == ENOMEM) {
					/* No memory. */
					break;
				}
				if (c != -1) {
					/* There is still more input, so undo the last pop. */
					in->undo(in);
				}

				/* Check for failures. */
				if (cur == buf) {
					/* Matching failure. Input failure was already checked
					 * earlier. */
					matching_failure = true;
					if (!assign_supress && assign_alloc) {
						/* Roll back. */
						free(buf);
						buffers[my_buffer_idx] = NULL;
						--next_unused_buffer_idx;
					}
					break;
				}

				/* Store the terminator. */
				if (!assign_supress && term_size > 0) {
					/* Space for the terminator was reserved. */
					*cur = '\0';
				}

				/* Store the result if not already stored. */
				if (!assign_supress) {
					if (assign_alloc) {
						char **pbuf = va_arg(arg, char **);
						*pbuf = buf;
					}
					++converted_cnt;
				}
				
				converting = false;
				/* Format string pointer already incremented. */
			} else if (*fmt == 'n') {
				/* Report the number of consumed bytes so far. */

				/* Sanity check. */
				bool sane =
				    width == -1 &&
				    length_mod == LMOD_NONE &&
				    assign_alloc == false &&
				    assign_supress == false;

				if (sane) {
					int *pi = va_arg(arg, int *);
					*pi = in->consumed;
				} else {
					/* Illegal format string. */
					break;
				}

				/* This shall not be counted as conversion. */
				converting = false;
				++fmt;
			} else {
				/* Illegal format string. */
				break;
			}
			
		} else {

			/* Processing outside conversion specifier. Either skip white
			 * spaces or match characters one by one. If conversion specifier
			 * is detected, switch to coversion mode. */
			if (isspace(*fmt)) {
				/* Skip white spaces in the format string. */
				while (isspace(*fmt)) {
					++fmt;
				}
				/* Skip white spaces in the input. */
				do {
					c = in->pop(in);
				} while (isspace(c));
				if (c != -1) {
					/* Input is not at EOF, so undo the last pop operation. */
					in->undo(in);
				}
			} else if (*fmt == '%' && *(fmt + 1) != '%') {
				/* Conversion specifier detected. Switch modes. */
				converting = true;
				/* Reset the conversion context. */
				assign_supress = false;
				assign_alloc = false;
				width = -1;
				length_mod = LMOD_NONE;
				int_conv_unsigned = false;
				int_conv_base = 0;
				++fmt;
			} else {
				/* One by one matching. */
				if (*fmt == '%') {
					/* Escape sequence detected. */
					++fmt;
					assert(*fmt == '%');
				}
				c = in->pop(in);
				if (c == -1) {
					/* Input failure. */
					break;
				} else if (c != *fmt) {
					/* Matching failure. */
					in->undo(in);
					matching_failure = true;
					break;
				} else {
					++fmt;
				}
			}
			
		}

	}

	in->release(in);

	/* This somewhat complicated return value decision is required by POSIX. */
	int rc;
	if (matching_failure) {
		rc = converted_cnt;
	} else {
		if (errno == EOK) {
			rc = converted_cnt > 0 ? converted_cnt : EOF;
		} else {
			rc = EOF;
		}
	}
	if (rc == EOF) {
		/* Caller will not know how many arguments were successfully converted,
		 * so the deallocation of buffers is our responsibility. */
		for (int i = 0; i < next_unused_buffer_idx; ++i) {
			free(buffers[i]);
			buffers[i] = NULL;
		}
		next_unused_buffer_idx = 0;
	}
	return rc;
}

/**
 * Convert formatted input from the stream.
 *
 * @param stream Input stream.
 * @param format Format description.
 * @param arg Output items.
 * @return The number of converted output items or EOF on failure.
 */
int vfscanf(
    FILE *restrict stream, const char *restrict format, va_list arg)
{
	_input_provider provider = {
	    { 0 }, 0, 0, NULL, 0, NULL, _PROV_CONSTRUCTED,
	    _capture_stream, _pop_stream, _undo_stream,
	    _borrow_cursor_universal, _return_cursor_stream, _release_stream
	};
	provider.source.stream = stream;
	return _internal_scanf(&provider, format, arg);
}

/**
 * Convert formatted input from the string.
 *
 * @param s Input string.
 * @param format Format description.
 * @param arg Output items.
 * @return The number of converted output items or EOF on failure.
 */
int vsscanf(
    const char *restrict s, const char *restrict format, va_list arg)
{
	_input_provider provider = {
	    { 0 }, 0, 0, NULL, 0, NULL, _PROV_CONSTRUCTED,
	    _capture_string, _pop_string, _undo_string,
	    _borrow_cursor_universal, _return_cursor_string, _release_string
	};
	provider.source.string = s;
	return _internal_scanf(&provider, format, arg);
}

/** @}
 */
