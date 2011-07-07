/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2011 Jiri Zarevucky
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
/** @file
 */

#define LIBPOSIX_INTERNAL

/* Must be first. */
#include "stdbool.h"

#include "internal/common.h"
#include "time.h"

#include "ctype.h"
#include "errno.h"

#include "libc/malloc.h"
#include "libc/task.h"
#include "libc/stats.h"

// TODO: documentation
// TODO: test everything in this file

/* Helper functions ***********************************************************/

#define HOURS_PER_DAY (24)
#define MINS_PER_HOUR (60)
#define SECS_PER_MIN (60)
#define MINS_PER_DAY (MINS_PER_HOUR * HOURS_PER_DAY)
#define SECS_PER_HOUR (SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY (SECS_PER_HOUR * HOURS_PER_DAY)

static bool _is_leap_year(time_t year)
{
	year += 1900;

	if (year % 400 == 0)
		return true;
	if (year % 100 == 0)
		return false;
	if (year % 4 == 0)
		return true;
	return false;
}

static int _days_in_month(time_t year, time_t mon)
{
	assert(mon >= 0 && mon <= 11);
	year += 1900;

	static int month_days[] =
		{ 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if (mon == 1) {
		/* february */
		return _is_leap_year(year) ? 29 : 28;
	} else {
		return month_days[mon];
	}
}

static int _day_of_year(time_t year, time_t mon, time_t mday)
{
	static int mdays[] =
	    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	static int leap_mdays[] =
	    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

	return (_is_leap_year(year) ? leap_mdays[mon] : mdays[mon]) + mday - 1;
}

/* Integer division that rounds to negative infinity.
 */
static time_t _floor_div(time_t op1, time_t op2)
{
	if (op1 >= 0 || op1 % op2 == 0) {
		return op1 / op2;
	} else {
		return op1 / op2 - 1;
	}
}

/* Modulo that rounds to negative infinity.
 */
static time_t _floor_mod(time_t op1, time_t op2)
{
	int div = _floor_div(op1, op2);

	/* (a / b) * b + a % b == a */
	/* thus, a % b == a - (a / b) * b */

	int result = op1 - div * op2;
	
	/* Some paranoid checking to ensure I didn't make a mistake here. */
	assert(result >= 0);
	assert(result < op2);
	assert(div * op2 + result == op1);
	
	return result;
}

static time_t _days_since_epoch(time_t year, time_t mon, time_t mday)
{
	return (year - 70) * 365 + _floor_div(year - 69, 4) -
	    _floor_div(year - 1, 100) + _floor_div(year + 299, 400) +
	    _day_of_year(year, mon, mday);
}

/* Assumes normalized broken-down time. */
static time_t _secs_since_epoch(const struct posix_tm *tm)
{
	return _days_since_epoch(tm->tm_year, tm->tm_mon, tm->tm_mday) *
	    SECS_PER_DAY + tm->tm_hour * SECS_PER_HOUR +
	    tm->tm_min * SECS_PER_MIN + tm->tm_sec;
}

static int _day_of_week(time_t year, time_t mon, time_t mday)
{
	/* 1970-01-01 is Thursday */
	return (_days_since_epoch(year, mon, mday) + 4) % 7;
}

struct _long_tm {
	time_t tm_sec;
	time_t tm_min;
	time_t tm_hour;
	time_t tm_mday;
	time_t tm_mon;
	time_t tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

static void _posix_to_long_tm(struct _long_tm *ltm, struct posix_tm *ptm)
{
	assert(ltm != NULL && ptm != NULL);
	ltm->tm_sec = ptm->tm_sec;
	ltm->tm_min = ptm->tm_min;
	ltm->tm_hour = ptm->tm_hour;
	ltm->tm_mday = ptm->tm_mday;
	ltm->tm_mon = ptm->tm_mon;
	ltm->tm_year = ptm->tm_year;
	ltm->tm_wday = ptm->tm_wday;
	ltm->tm_yday = ptm->tm_yday;
	ltm->tm_isdst = ptm->tm_isdst;
}

static void _long_to_posix_tm(struct posix_tm *ptm, struct _long_tm *ltm)
{
	assert(ltm != NULL && ptm != NULL);
	// FIXME: the cast should be unnecessary, libarch/common.h brain-damage
	assert((ltm->tm_year >= (int) INT_MIN) && (ltm->tm_year <= (int) INT_MAX));

	ptm->tm_sec = ltm->tm_sec;
	ptm->tm_min = ltm->tm_min;
	ptm->tm_hour = ltm->tm_hour;
	ptm->tm_mday = ltm->tm_mday;
	ptm->tm_mon = ltm->tm_mon;
	ptm->tm_year = ltm->tm_year;
	ptm->tm_wday = ltm->tm_wday;
	ptm->tm_yday = ltm->tm_yday;
	ptm->tm_isdst = ltm->tm_isdst;
}

static void _normalize_time(struct _long_tm *tm)
{
	// TODO: DST correction

	/* Adjust time. */
	tm->tm_min += _floor_div(tm->tm_sec, SECS_PER_MIN);
	tm->tm_sec = _floor_mod(tm->tm_sec, SECS_PER_MIN);
	tm->tm_hour += _floor_div(tm->tm_min, MINS_PER_HOUR);
	tm->tm_min = _floor_mod(tm->tm_min, MINS_PER_HOUR);
	tm->tm_mday += _floor_div(tm->tm_hour, HOURS_PER_DAY);
	tm->tm_hour = _floor_mod(tm->tm_hour, HOURS_PER_DAY);

	/* Adjust month. */
	tm->tm_year += _floor_div(tm->tm_mon, 12);
	tm->tm_mon = _floor_mod(tm->tm_mon, 12);

	/* Now the difficult part - days of month. */
	/* Slow, but simple. */
	// TODO: do this faster

	while (tm->tm_mday < 1) {
		tm->tm_mon--;
		if (tm->tm_mon == -1) {
			tm->tm_mon = 11;
			tm->tm_year--;
		}
		
		tm->tm_mday += _days_in_month(tm->tm_year, tm->tm_mon);
	}

	while (tm->tm_mday > _days_in_month(tm->tm_year, tm->tm_mon)) {
		tm->tm_mday -= _days_in_month(tm->tm_year, tm->tm_mon);
		
		tm->tm_mon++;
		if (tm->tm_mon == 12) {
			tm->tm_mon = 0;
			tm->tm_year++;
		}
	}

	/* Calculate the remaining two fields. */
	tm->tm_yday = _day_of_year(tm->tm_year, tm->tm_mon, tm->tm_mday);
	tm->tm_wday = _day_of_week(tm->tm_year, tm->tm_mon, tm->tm_mday);
}

/* Which day the week-based year starts on relative to the first calendar day.
 * E.g. if the year starts on December 31st, the return value is -1.
 */
static int _wbyear_offset(int year)
{
	int start_wday = _day_of_week(year, 0, 1);
	return _floor_mod(4 - start_wday, 7) - 3;
}

/* Returns week-based year of the specified time.
 * Assumes normalized broken-down time.
 */
static int _wbyear(const struct posix_tm *tm)
{
	if (tm->tm_yday < _wbyear_offset(tm->tm_year)) {
		return tm->tm_year - 1;
	}
	if (tm->tm_yday > (364 + _is_leap_year(tm->tm_year) +
	    _wbyear_offset(tm->tm_year + 1))) {
		return tm->tm_year + 1;
	}
	return tm->tm_year;
}

/* Number of week in year, when week starts on sunday.
 */
static int _sun_week_number(const struct posix_tm *tm)
{
	// TODO
	not_implemented();
}

/* Number of week in week-based year.
 */
static int _iso_week_number(const struct posix_tm *tm)
{
	// TODO
	not_implemented();
}

/* Number of week in year, when week starts on monday.
 */
static int _mon_week_number(const struct posix_tm *tm)
{
	// TODO
	not_implemented();
}

/******************************************************************************/

int posix_daylight;
long posix_timezone;
char *posix_tzname[2];

void posix_tzset(void)
{
	// TODO: read environment
	posix_tzname[0] = (char *) "GMT";
	posix_tzname[1] = (char *) "GMT";
	posix_daylight = 0;
	posix_timezone = 0;
}

double posix_difftime(time_t time1, time_t time0)
{
	return (double) (time1 - time0);
}

/** This function first normalizes the provided broken-down time
 *  (moves all values to their proper bounds) and then tries to
 *  calculate the appropriate time_t representation.
 *
 * @param timeptr Broken-down time.
 * @return time_t representation of the time, undefined value on overflow
 */
time_t posix_mktime(struct posix_tm *tm)
{
	// TODO: take DST flag into account
	// TODO: detect overflow

	struct _long_tm ltm;
	_posix_to_long_tm(&ltm, tm);
	_normalize_time(&ltm);
	_long_to_posix_tm(tm, &ltm);

	return _secs_since_epoch(tm);
}

/**
 *
 * @param timep
 * @return
 */
struct posix_tm *posix_localtime(const time_t *timep)
{
	static struct posix_tm result;
	return posix_localtime_r(timep, &result);
}

struct posix_tm *posix_localtime_r(const time_t *restrict timer,
    struct posix_tm *restrict result)
{
	assert(timer != NULL);
	assert(result != NULL);

	// TODO: deal with timezone
	// currently assumes system and all times are in GMT

	/* Set epoch and seconds to _long_tm struct and normalize to get
	 * correct values.
	 */
	struct _long_tm ltm = {
		.tm_sec = *timer,
		.tm_min = 0,
		.tm_hour = 0, /* 00:00:xx */
		.tm_mday = 1,
		.tm_mon = 0, /* January 1st */
		.tm_year = 70, /* 1970 */
	};
	_normalize_time(&ltm);

	if (ltm.tm_year < (int) INT_MIN || ltm.tm_year > (int) INT_MAX) {
		errno = EOVERFLOW;
		return NULL;
	}

	_long_to_posix_tm(result, &ltm);
	return result;
}

/**
 *
 * @param tm
 * @return
 */
char *posix_asctime(const struct posix_tm *timeptr)
{
	static char buf[ASCTIME_BUF_LEN];
	return posix_asctime_r(timeptr, buf);
}

char *posix_asctime_r(const struct posix_tm *restrict timeptr,
    char *restrict buf)
{
	assert(timeptr != NULL);
	assert(buf != NULL);

	static const char *wday[] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char *mon[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	snprintf(buf, ASCTIME_BUF_LEN, "%s %s %2d %02d:%02d:%02d %d\n",
	    wday[timeptr->tm_wday],
	    mon[timeptr->tm_mon],
	    timeptr->tm_mday, timeptr->tm_hour,
	    timeptr->tm_min, timeptr->tm_sec,
	    1900 + timeptr->tm_year);

	return buf;
}

/**
 * 
 * @param timep
 * @return
 */
char *posix_ctime(const time_t *timep)
{
	return posix_asctime(posix_localtime(timep));
}

/**
 * 
 * @param s
 * @param maxsize
 * @param format
 * @param tm
 * @return
 */
size_t posix_strftime(char *s, size_t maxsize,
    const char *format, const struct posix_tm *tm)
{
	// TODO: use locale
	static const char *wday_abbr[] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char *wday[] = {
		"Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday"
	};
	static const char *mon_abbr[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static const char *mon[] = {
		"January", "February", "March", "April", "May", "June", "July",
		"August", "September", "October", "November", "December"
	};
	
	if (maxsize < 1) {
		return 0;
	}
	
	char *ptr = s;
	size_t consumed;
	size_t remaining = maxsize;
	
	#define append(...) { \
		/* FIXME: this requires POSIX-correct snprintf */ \
		/*        otherwise it won't work with non-ascii chars */ \
		consumed = snprintf(ptr, remaining, __VA_ARGS__); \
		if (consumed >= remaining) { \
			return 0; \
		} \
		ptr += consumed; \
		remaining -= consumed; \
	}
	
	#define recurse(fmt) { \
		consumed = posix_strftime(ptr, remaining, fmt, tm); \
		if (consumed == 0) { \
			return 0; \
		} \
		ptr += consumed; \
		remaining -= consumed; \
	}
	
	#define TO_12H(hour) (((hour) > 12) ? ((hour) - 12) : \
	    (((hour) == 0) ? 12 : (hour)))
	
	while (*format != '\0') {
		if (*format != '%') {
			append("%c", *format);
			format++;
			continue;
		}
		
		format++;
		if (*format == '0' || *format == '+') {
			// TODO: padding
			format++;
		}
		while (isdigit(*format)) {
			// TODO: padding
			format++;
		}
		if (*format == 'O' || *format == 'E') {
			// TODO: locale's alternative format
			format++;
		}
		
		switch (*format) {
		case 'a':
			append("%s", wday_abbr[tm->tm_wday]); break;
		case 'A':
			append("%s", wday[tm->tm_wday]); break;
		case 'b':
			append("%s", mon_abbr[tm->tm_mon]); break;
		case 'B':
			append("%s", mon[tm->tm_mon]); break;
		case 'c':
			// TODO: locale-specific datetime format
			recurse("%Y-%m-%d %H:%M:%S"); break;
		case 'C':
			append("%02d", (1900 + tm->tm_year) / 100); break;
		case 'd':
			append("%02d", tm->tm_mday); break;
		case 'D':
			recurse("%m/%d/%y"); break;
		case 'e':
			append("%2d", tm->tm_mday); break;
		case 'F':
			recurse("%+4Y-%m-%d"); break;
		case 'g':
			append("%02d", _wbyear(tm) % 100); break;
		case 'G':
			append("%d", _wbyear(tm)); break;
		case 'h':
			recurse("%b"); break;
		case 'H':
			append("%02d", tm->tm_hour); break;
		case 'I':
			append("%02d", TO_12H(tm->tm_hour)); break;
		case 'j':
			append("%03d", tm->tm_yday); break;
		case 'k':
			append("%2d", tm->tm_hour); break;
		case 'l':
			append("%2d", TO_12H(tm->tm_hour)); break;
		case 'm':
			append("%02d", tm->tm_mon); break;
		case 'M':
			append("%02d", tm->tm_min); break;
		case 'n':
			append("\n"); break;
		case 'p':
			append("%s", tm->tm_hour < 12 ? "AM" : "PM"); break;
		case 'P':
			append("%s", tm->tm_hour < 12 ? "am" : "PM"); break;
		case 'r':
			recurse("%I:%M:%S %p"); break;
		case 'R':
			recurse("%H:%M"); break;
		case 's':
			append("%ld", _secs_since_epoch(tm)); break;
		case 'S':
			append("%02d", tm->tm_sec); break;
		case 't':
			append("\t"); break;
		case 'T':
			recurse("%H:%M:%S"); break;
		case 'u':
			append("%d", (tm->tm_wday == 0) ? 7 : tm->tm_wday); break;
		case 'U':
			append("%02d", _sun_week_number(tm)); break;
		case 'V':
			append("%02d", _iso_week_number(tm)); break;
		case 'w':
			append("%d", tm->tm_wday); break;
		case 'W':
			append("%02d", _mon_week_number(tm)); break;
		case 'x':
			// TODO: locale-specific date format
			recurse("%Y-%m-%d"); break;
		case 'X':
			// TODO: locale-specific time format
			recurse("%H:%M:%S"); break;
		case 'y':
			append("%02d", tm->tm_year % 100); break;
		case 'Y':
			append("%d", 1900 + tm->tm_year); break;
		case 'z':
			// TODO: timezone
			break;
		case 'Z':
			// TODO: timezone
			break;
		case '%':
			append("%%");
			break;
		default:
			/* Invalid specifier, print verbatim. */
			while (*format != '%') {
				format--;
			}
			append("%%");
			break;
		}
		format++;
	}
	
	#undef append
	#undef recurse
	
	return maxsize - remaining;
}

/**
 * Get CPU time used since the process invocation.
 *
 * @return Consumed CPU cycles by this process or -1 if not available.
 */
posix_clock_t posix_clock(void)
{
	posix_clock_t total_cycles = -1;
	stats_task_t *task_stats = stats_get_task(task_get_id());
	if (task_stats) {
		total_cycles = (posix_clock_t) (task_stats->kcycles + task_stats->ucycles);
	}
	free(task_stats);
	task_stats = 0;

	return total_cycles;
}

/** @}
 */
