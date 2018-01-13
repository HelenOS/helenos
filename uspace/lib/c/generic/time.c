/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <libarch/barrier.h>
#include <macros.h>
#include <errno.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <libc.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <loc.h>
#include <device/clock_dev.h>
#include <thread.h>

#define ASCTIME_BUF_LEN  26

#define HOURS_PER_DAY  24
#define MINS_PER_HOUR  60
#define SECS_PER_MIN   60
#define USECS_PER_SEC  1000000
#define MINS_PER_DAY   (MINS_PER_HOUR * HOURS_PER_DAY)
#define SECS_PER_HOUR  (SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY   (SECS_PER_HOUR * HOURS_PER_DAY)

/** Pointer to kernel shared variables with time */
struct {
	volatile sysarg_t seconds1;
	volatile sysarg_t useconds;
	volatile sysarg_t seconds2;
} *ktime = NULL;

static async_sess_t *clock_conn = NULL;

/** Check whether the year is a leap year.
 *
 * @param year Year since 1900 (e.g. for 1970, the value is 70).
 *
 * @return true if year is a leap year, false otherwise
 *
 */
static bool is_leap_year(time_t year)
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

/** How many days there are in the given month
 *
 * Return how many days there are in the given month of the given year.
 * Note that year is only taken into account if month is February.
 *
 * @param year Year since 1900 (can be negative).
 * @param mon  Month of the year. 0 for January, 11 for December.
 *
 * @return Number of days in the specified month.
 *
 */
static int days_in_month(time_t year, time_t mon)
{
	assert(mon >= 0);
	assert(mon <= 11);
	
	static int month_days[] = {
		31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	
	if (mon == 1) {
		/* February */
		year += 1900;
		return is_leap_year(year) ? 29 : 28;
	}
	
	return month_days[mon];
}

/** Which day of that year it is.
 *
 * For specified year, month and day of month, return which day of that year
 * it is.
 *
 * For example, given date 2011-01-03, the corresponding expression is:
 * day_of_year(111, 0, 3) == 2
 *
 * @param year Year (year 1900 = 0, can be negative).
 * @param mon  Month (January = 0).
 * @param mday Day of month (First day is 1).
 *
 * @return Day of year (First day is 0).
 *
 */
static int day_of_year(time_t year, time_t mon, time_t mday)
{
	static int mdays[] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};
	
	static int leap_mdays[] = {
		0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
	};
	
	return (is_leap_year(year) ? leap_mdays[mon] : mdays[mon]) + mday - 1;
}

/** Integer division that rounds to negative infinity.
 *
 * Used by some functions in this module.
 *
 * @param op1 Dividend.
 * @param op2 Divisor.
 *
 * @return Rounded quotient.
 *
 */
static time_t floor_div(time_t op1, time_t op2)
{
	if ((op1 >= 0) || (op1 % op2 == 0))
		return op1 / op2;
	
	return op1 / op2 - 1;
}

/** Modulo that rounds to negative infinity.
 *
 * Used by some functions in this module.
 *
 * @param op1 Dividend.
 * @param op2 Divisor.
 *
 * @return Remainder.
 *
 */
static time_t floor_mod(time_t op1, time_t op2)
{
	time_t div = floor_div(op1, op2);
	
	/*
	 * (a / b) * b + a % b == a
	 * Thus: a % b == a - (a / b) * b
	 */
	
	time_t result = op1 - div * op2;
	
	/* Some paranoid checking to ensure there is mistake here. */
	assert(result >= 0);
	assert(result < op2);
	assert(div * op2 + result == op1);
	
	return result;
}

/** Number of days since the Epoch.
 *
 * Epoch is 1970-01-01, which is also equal to day 0.
 *
 * @param year Year (year 1900 = 0, may be negative).
 * @param mon  Month (January = 0).
 * @param mday Day of month (first day = 1).
 *
 * @return Number of days since the Epoch.
 *
 */
static time_t days_since_epoch(time_t year, time_t mon, time_t mday)
{
	return (year - 70) * 365 + floor_div(year - 69, 4) -
	    floor_div(year - 1, 100) + floor_div(year + 299, 400) +
	    day_of_year(year, mon, mday);
}

/** Seconds since the Epoch.
 *
 * See also days_since_epoch().
 *
 * @param tm Normalized broken-down time.
 *
 * @return Number of seconds since the epoch, not counting leap seconds.
 *
 */
static time_t secs_since_epoch(const struct tm *tm)
{
	return days_since_epoch(tm->tm_year, tm->tm_mon, tm->tm_mday) *
	    SECS_PER_DAY + tm->tm_hour * SECS_PER_HOUR +
	    tm->tm_min * SECS_PER_MIN + tm->tm_sec;
}

/** Which day of week the specified date is.
 *
 * @param year Year (year 1900 = 0).
 * @param mon  Month (January = 0).
 * @param mday Day of month (first = 1).
 *
 * @return Day of week (Sunday = 0).
 *
 */
static time_t day_of_week(time_t year, time_t mon, time_t mday)
{
	/* 1970-01-01 is Thursday */
	return floor_mod(days_since_epoch(year, mon, mday) + 4, 7);
}

/** Normalize the broken-down time.
 *
 * Optionally add specified amount of seconds.
 *
 * @param tm Broken-down time to normalize.
 * @param tv Timeval to add.
 *
 * @return 0 on success, -1 on overflow
 *
 */
static int normalize_tm_tv(struct tm *tm, const struct timeval *tv)
{
	// TODO: DST correction
	
	/* Set initial values. */
	time_t usec = tm->tm_usec + tv->tv_usec;
	time_t sec = tm->tm_sec + tv->tv_sec;
	time_t min = tm->tm_min;
	time_t hour = tm->tm_hour;
	time_t day = tm->tm_mday - 1;
	time_t mon = tm->tm_mon;
	time_t year = tm->tm_year;
	
	/* Adjust time. */
	sec += floor_div(usec, USECS_PER_SEC);
	usec = floor_mod(usec, USECS_PER_SEC);
	min += floor_div(sec, SECS_PER_MIN);
	sec = floor_mod(sec, SECS_PER_MIN);
	hour += floor_div(min, MINS_PER_HOUR);
	min = floor_mod(min, MINS_PER_HOUR);
	day += floor_div(hour, HOURS_PER_DAY);
	hour = floor_mod(hour, HOURS_PER_DAY);
	
	/* Adjust month. */
	year += floor_div(mon, 12);
	mon = floor_mod(mon, 12);
	
	/* Now the difficult part - days of month. */
	
	/* First, deal with whole cycles of 400 years = 146097 days. */
	year += floor_div(day, 146097) * 400;
	day = floor_mod(day, 146097);
	
	/* Then, go in one year steps. */
	if (mon <= 1) {
		/* January and February. */
		while (day > 365) {
			day -= is_leap_year(year) ? 366 : 365;
			year++;
		}
	} else {
		/* Rest of the year. */
		while (day > 365) {
			day -= is_leap_year(year + 1) ? 366 : 365;
			year++;
		}
	}
	
	/* Finally, finish it off month per month. */
	while (day >= days_in_month(year, mon)) {
		day -= days_in_month(year, mon);
		mon++;
		
		if (mon >= 12) {
			mon -= 12;
			year++;
		}
	}
	
	/* Calculate the remaining two fields. */
	tm->tm_yday = day_of_year(year, mon, day + 1);
	tm->tm_wday = day_of_week(year, mon, day + 1);
	
	/* And put the values back to the struct. */
	tm->tm_usec = (int) usec;
	tm->tm_sec = (int) sec;
	tm->tm_min = (int) min;
	tm->tm_hour = (int) hour;
	tm->tm_mday = (int) day + 1;
	tm->tm_mon = (int) mon;
	
	/* Casts to work around POSIX brain-damage. */
	if (year > ((int) INT_MAX) || year < ((int) INT_MIN)) {
		tm->tm_year = (year < 0) ? ((int) INT_MIN) : ((int) INT_MAX);
		return -1;
	}
	
	tm->tm_year = (int) year;
	return 0;
}

static int normalize_tm_time(struct tm *tm, time_t time)
{
	struct timeval tv = {
		.tv_sec = time,
		.tv_usec = 0
	};

	return normalize_tm_tv(tm, &tv);
}


/** Which day the week-based year starts on.
 *
 * Relative to the first calendar day. E.g. if the year starts
 * on December 31st, the return value is -1.
 *
 * @param Year since 1900.
 *
 * @return Offset of week-based year relative to calendar year.
 *
 */
static int wbyear_offset(int year)
{
	int start_wday = day_of_week(year, 0, 1);
	
	return floor_mod(4 - start_wday, 7) - 3;
}

/** Week-based year of the specified time.
 *
 * @param tm Normalized broken-down time.
 *
 * @return Week-based year.
 *
 */
static int wbyear(const struct tm *tm)
{
	int day = tm->tm_yday - wbyear_offset(tm->tm_year);
	
	if (day < 0) {
		/* Last week of previous year. */
		return tm->tm_year - 1;
	}
	
	if (day > 364 + is_leap_year(tm->tm_year)) {
		/* First week of next year. */
		return tm->tm_year + 1;
	}
	
	/* All the other days are in the calendar year. */
	return tm->tm_year;
}

/** Week number of the year (assuming weeks start on Sunday).
 *
 * The first Sunday of January is the first day of week 1;
 * days in the new year before this are in week 0.
 *
 * @param tm Normalized broken-down time.
 *
 * @return The week number (0 - 53).
 *
 */
static int sun_week_number(const struct tm *tm)
{
	int first_day = (7 - day_of_week(tm->tm_year, 0, 1)) % 7;
	
	return (tm->tm_yday - first_day + 7) / 7;
}

/** Week number of the year (assuming weeks start on Monday).
 *
 * If the week containing January 1st has four or more days
 * in the new year, then it is considered week 1. Otherwise,
 * it is the last week of the previous year, and the next week
 * is week 1. Both January 4th and the first Thursday
 * of January are always in week 1.
 *
 * @param tm Normalized broken-down time.
 *
 * @return The week number (1 - 53).
 *
 */
static int iso_week_number(const struct tm *tm)
{
	int day = tm->tm_yday - wbyear_offset(tm->tm_year);
	
	if (day < 0) {
		/* Last week of previous year. */
		return 53;
	}
	
	if (day > 364 + is_leap_year(tm->tm_year)) {
		/* First week of next year. */
		return 1;
	}
	
	/* All the other days give correct answer. */
	return (day / 7 + 1);
}

/** Week number of the year (assuming weeks start on Monday).
 *
 * The first Monday of January is the first day of week 1;
 * days in the new year before this are in week 0.
 *
 * @param tm Normalized broken-down time.
 *
 * @return The week number (0 - 53).
 *
 */
static int mon_week_number(const struct tm *tm)
{
	int first_day = (1 - day_of_week(tm->tm_year, 0, 1)) % 7;
	
	return (tm->tm_yday - first_day + 7) / 7;
}

static void tv_normalize(struct timeval *tv)
{
	while (tv->tv_usec > USECS_PER_SEC) {
		tv->tv_sec++;
		tv->tv_usec -= USECS_PER_SEC;
	}
	while (tv->tv_usec < 0) {
		tv->tv_sec--;
		tv->tv_usec += USECS_PER_SEC;
	}
}

/** Add microseconds to given timeval.
 *
 * @param tv    Destination timeval.
 * @param usecs Number of microseconds to add.
 *
 */
void tv_add_diff(struct timeval *tv, suseconds_t usecs)
{
	tv->tv_sec += usecs / USECS_PER_SEC;
	tv->tv_usec += usecs % USECS_PER_SEC;
	tv_normalize(tv);
}

/** Add two timevals.
 *
 * @param tv1 First timeval.
 * @param tv2 Second timeval.
 */
void tv_add(struct timeval *tv1, struct timeval *tv2)
{
	tv1->tv_sec += tv2->tv_sec;
	tv1->tv_usec += tv2->tv_usec;
	tv_normalize(tv1);
}

/** Subtract two timevals.
 *
 * @param tv1 First timeval.
 * @param tv2 Second timeval.
 *
 * @return Difference between tv1 and tv2 (tv1 - tv2) in
 *         microseconds.
 *
 */
suseconds_t tv_sub_diff(struct timeval *tv1, struct timeval *tv2)
{
	return (tv1->tv_usec - tv2->tv_usec) +
	    ((tv1->tv_sec - tv2->tv_sec) * USECS_PER_SEC);
}

/** Subtract two timevals.
 *
 * @param tv1 First timeval.
 * @param tv2 Second timeval.
 *
 */
void tv_sub(struct timeval *tv1, struct timeval *tv2)
{
	tv1->tv_sec -= tv2->tv_sec;
	tv1->tv_usec -= tv2->tv_usec;
	tv_normalize(tv1);
}

/** Decide if one timeval is greater than the other.
 *
 * @param t1 First timeval.
 * @param t2 Second timeval.
 *
 * @return True if tv1 is greater than tv2.
 * @return False otherwise.
 *
 */
int tv_gt(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return true;
	
	if ((tv1->tv_sec == tv2->tv_sec) && (tv1->tv_usec > tv2->tv_usec))
		return true;
	
	return false;
}

/** Decide if one timeval is greater than or equal to the other.
 *
 * @param tv1 First timeval.
 * @param tv2 Second timeval.
 *
 * @return True if tv1 is greater than or equal to tv2.
 * @return False otherwise.
 *
 */
int tv_gteq(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return true;
	
	if ((tv1->tv_sec == tv2->tv_sec) && (tv1->tv_usec >= tv2->tv_usec))
		return true;
	
	return false;
}

/** Get time of day.
 *
 * The time variables are memory mapped (read-only) from kernel which
 * updates them periodically.
 *
 * As it is impossible to read 2 values atomically, we use a trick:
 * First we read the seconds, then we read the microseconds, then we
 * read the seconds again. If a second elapsed in the meantime, set
 * the microseconds to zero.
 *
 * This assures that the values returned by two subsequent calls
 * to gettimeofday() are monotonous.
 *
 */
void gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (tz) {
		tz->tz_minuteswest = 0;
		tz->tz_dsttime = DST_NONE;
	}
	
	if (clock_conn == NULL) {
		category_id_t cat_id;
		errno_t rc = loc_category_get_id("clock", &cat_id, IPC_FLAG_BLOCKING);
		if (rc != EOK)
			goto fallback;
		
		service_id_t *svc_ids;
		size_t svc_cnt;
		rc = loc_category_get_svcs(cat_id, &svc_ids, &svc_cnt);
		if (rc != EOK)
			goto fallback;
		
		if (svc_cnt == 0)
			goto fallback;
		
		char *svc_name;
		rc = loc_service_get_name(svc_ids[0], &svc_name);
		free(svc_ids);
		if (rc != EOK)
			goto fallback;
		
		service_id_t svc_id;
		rc = loc_service_get_id(svc_name, &svc_id, 0);
		free(svc_name);
		if (rc != EOK)
			goto fallback;
		
		clock_conn = loc_service_connect(svc_id, INTERFACE_DDF,
		    IPC_FLAG_BLOCKING);
		if (!clock_conn)
			goto fallback;
	}
	
	struct tm time;
	errno_t rc = clock_dev_time_get(clock_conn, &time);
	if (rc != EOK)
		goto fallback;
	
	tv->tv_usec = time.tm_usec;
	tv->tv_sec = mktime(&time);
	
	return;
	
fallback:
	getuptime(tv);
}

void getuptime(struct timeval *tv)
{
	if (ktime == NULL) {
		uintptr_t faddr;
		errno_t rc = sysinfo_get_value("clock.faddr", &faddr);
		if (rc != EOK) {
			errno = rc;
			goto fallback;
		}
		
		void *addr = AS_AREA_ANY;
		rc = physmem_map(faddr, 1, AS_AREA_READ | AS_AREA_CACHEABLE,
		    &addr);
		if (rc != EOK) {
			as_area_destroy(addr);
			errno = rc;
			goto fallback;
		}
		
		ktime = addr;
	}
	
	sysarg_t s2 = ktime->seconds2;
	
	read_barrier();
	tv->tv_usec = ktime->useconds;
	
	read_barrier();
	sysarg_t s1 = ktime->seconds1;
	
	if (s1 != s2) {
		tv->tv_sec = max(s1, s2);
		tv->tv_usec = 0;
	} else
		tv->tv_sec = s1;
	
	return;
	
fallback:
	tv->tv_sec = 0;
	tv->tv_usec = 0;
}

time_t time(time_t *tloc)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	if (tloc)
		*tloc = tv.tv_sec;
	
	return tv.tv_sec;
}

void udelay(useconds_t time)
{
	(void) __SYSCALL1(SYS_THREAD_UDELAY, (sysarg_t) time);
}

/** Get time from broken-down time.
 *
 * First normalize the provided broken-down time
 * (moves all values to their proper bounds) and
 * then try to calculate the appropriate time_t
 * representation.
 *
 * @param tm Broken-down time.
 *
 * @return time_t representation of the time.
 * @return Undefined value on overflow.
 *
 */
time_t mktime(struct tm *tm)
{
	// TODO: take DST flag into account
	// TODO: detect overflow
	
	normalize_tm_time(tm, 0);
	return secs_since_epoch(tm);
}

/*
 * FIXME: This requires POSIX-correct snprintf.
 *        Otherwise it won't work with non-ASCII chars.
 */
#define APPEND(...) \
	{ \
		consumed = snprintf(ptr, remaining, __VA_ARGS__); \
		if (consumed >= remaining) \
			return 0; \
		\
		ptr += consumed; \
		remaining -= consumed; \
	}

#define RECURSE(fmt) \
	{ \
		consumed = strftime(ptr, remaining, fmt, tm); \
		if (consumed == 0) \
			return 0; \
		\
		ptr += consumed; \
		remaining -= consumed; \
	}

#define TO_12H(hour) \
	(((hour) > 12) ? ((hour) - 12) : \
	    (((hour) == 0) ? 12 : (hour)))

/** Convert time and date to a string.
 *
 * @param s       Buffer to write string to.
 * @param maxsize Size of the buffer.
 * @param format  Format of the output.
 * @param tm      Broken-down time to format.
 *
 * @return Number of bytes written.
 *
 */
size_t strftime(char *restrict s, size_t maxsize,
    const char *restrict format, const struct tm *restrict tm)
{
	assert(s != NULL);
	assert(format != NULL);
	assert(tm != NULL);
	
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
	
	if (maxsize < 1)
		return 0;
	
	char *ptr = s;
	size_t consumed;
	size_t remaining = maxsize;
	
	while (*format != '\0') {
		if (*format != '%') {
			APPEND("%c", *format);
			format++;
			continue;
		}
		
		format++;
		if ((*format == '0') || (*format == '+')) {
			// TODO: padding
			format++;
		}
		
		while (isdigit(*format)) {
			// TODO: padding
			format++;
		}
		
		if ((*format == 'O') || (*format == 'E')) {
			// TODO: locale's alternative format
			format++;
		}
		
		switch (*format) {
		case 'a':
			APPEND("%s", wday_abbr[tm->tm_wday]);
			break;
		case 'A':
			APPEND("%s", wday[tm->tm_wday]);
			break;
		case 'b':
			APPEND("%s", mon_abbr[tm->tm_mon]);
			break;
		case 'B':
			APPEND("%s", mon[tm->tm_mon]);
			break;
		case 'c':
			// TODO: locale-specific datetime format
			RECURSE("%Y-%m-%d %H:%M:%S");
			break;
		case 'C':
			APPEND("%02d", (1900 + tm->tm_year) / 100);
			break;
		case 'd':
			APPEND("%02d", tm->tm_mday);
			break;
		case 'D':
			RECURSE("%m/%d/%y");
			break;
		case 'e':
			APPEND("%2d", tm->tm_mday);
			break;
		case 'F':
			RECURSE("%+4Y-%m-%d");
			break;
		case 'g':
			APPEND("%02d", wbyear(tm) % 100);
			break;
		case 'G':
			APPEND("%d", wbyear(tm));
			break;
		case 'h':
			RECURSE("%b");
			break;
		case 'H':
			APPEND("%02d", tm->tm_hour);
			break;
		case 'I':
			APPEND("%02d", TO_12H(tm->tm_hour));
			break;
		case 'j':
			APPEND("%03d", tm->tm_yday);
			break;
		case 'k':
			APPEND("%2d", tm->tm_hour);
			break;
		case 'l':
			APPEND("%2d", TO_12H(tm->tm_hour));
			break;
		case 'm':
			APPEND("%02d", tm->tm_mon);
			break;
		case 'M':
			APPEND("%02d", tm->tm_min);
			break;
		case 'n':
			APPEND("\n");
			break;
		case 'p':
			APPEND("%s", tm->tm_hour < 12 ? "AM" : "PM");
			break;
		case 'P':
			APPEND("%s", tm->tm_hour < 12 ? "am" : "PM");
			break;
		case 'r':
			RECURSE("%I:%M:%S %p");
			break;
		case 'R':
			RECURSE("%H:%M");
			break;
		case 's':
			APPEND("%ld", secs_since_epoch(tm));
			break;
		case 'S':
			APPEND("%02d", tm->tm_sec);
			break;
		case 't':
			APPEND("\t");
			break;
		case 'T':
			RECURSE("%H:%M:%S");
			break;
		case 'u':
			APPEND("%d", (tm->tm_wday == 0) ? 7 : tm->tm_wday);
			break;
		case 'U':
			APPEND("%02d", sun_week_number(tm));
			break;
		case 'V':
			APPEND("%02d", iso_week_number(tm));
			break;
		case 'w':
			APPEND("%d", tm->tm_wday);
			break;
		case 'W':
			APPEND("%02d", mon_week_number(tm));
			break;
		case 'x':
			// TODO: locale-specific date format
			RECURSE("%Y-%m-%d");
			break;
		case 'X':
			// TODO: locale-specific time format
			RECURSE("%H:%M:%S");
			break;
		case 'y':
			APPEND("%02d", tm->tm_year % 100);
			break;
		case 'Y':
			APPEND("%d", 1900 + tm->tm_year);
			break;
		case 'z':
			// TODO: timezone
			break;
		case 'Z':
			// TODO: timezone
			break;
		case '%':
			APPEND("%%");
			break;
		default:
			/* Invalid specifier, print verbatim. */
			while (*format != '%')
				format--;
			
			APPEND("%%");
			break;
		}
		
		format++;
	}
	
	return maxsize - remaining;
}

/** Convert a time value to a broken-down UTC time/
 *
 * @param time   Time to convert
 * @param result Structure to store the result to
 *
 * @return EOK or an error code
 *
 */
errno_t time_utc2tm(const time_t time, struct tm *restrict result)
{
	assert(result != NULL);
	
	/* Set result to epoch. */
	result->tm_usec = 0;
	result->tm_sec = 0;
	result->tm_min = 0;
	result->tm_hour = 0;
	result->tm_mday = 1;
	result->tm_mon = 0;
	result->tm_year = 70; /* 1970 */
	
	if (normalize_tm_time(result, time) == -1)
		return EOVERFLOW;
	
	return EOK;
}

/** Convert a time value to a NULL-terminated string.
 *
 * The format is "Wed Jun 30 21:49:08 1993\n" expressed in UTC.
 *
 * @param time Time to convert.
 * @param buf  Buffer to store the string to, must be at least
 *             ASCTIME_BUF_LEN bytes long.
 *
 * @return EOK or an error code.
 *
 */
errno_t time_utc2str(const time_t time, char *restrict buf)
{
	struct tm tm;
	errno_t ret = time_utc2tm(time, &tm);
	if (ret != EOK)
		return ret;
	
	time_tm2str(&tm, buf);
	return EOK;
}

/** Convert broken-down time to a NULL-terminated string.
 *
 * The format is "Sun Jan 1 00:00:00 1970\n". (Obsolete)
 *
 * @param timeptr Broken-down time structure.
 * @param buf     Buffer to store string to, must be at least
 *                ASCTIME_BUF_LEN bytes long.
 *
 */
void time_tm2str(const struct tm *restrict timeptr, char *restrict buf)
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
}

/** Converts a time value to a broken-down local time.
 *
 * Time is expressed relative to the user's specified timezone.
 *
 * @param tv     Timeval to convert.
 * @param result Structure to store the result to.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t time_tv2tm(const struct timeval *tv, struct tm *restrict result)
{
	// TODO: Deal with timezones.
	//       Currently assumes system and all times are in UTC
	
	/* Set result to epoch. */
	result->tm_usec = 0;
	result->tm_sec = 0;
	result->tm_min = 0;
	result->tm_hour = 0;
	result->tm_mday = 1;
	result->tm_mon = 0;
	result->tm_year = 70; /* 1970 */
	
	if (normalize_tm_tv(result, tv) == -1)
		return EOVERFLOW;
	
	return EOK;
}

/** Converts a time value to a broken-down local time.
 *
 * Time is expressed relative to the user's specified timezone.
 *
 * @param timer  Time to convert.
 * @param result Structure to store the result to.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t time_local2tm(const time_t time, struct tm *restrict result)
{
	struct timeval tv = {
		.tv_sec = time,
		.tv_usec = 0
	};

	return time_tv2tm(&tv, result);
}

/** Convert the calendar time to a NULL-terminated string.
 *
 * The format is "Wed Jun 30 21:49:08 1993\n" expressed relative to the
 * user's specified timezone.
 *
 * @param timer Time to convert.
 * @param buf   Buffer to store the string to. Must be at least
 *              ASCTIME_BUF_LEN bytes long.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t time_local2str(const time_t time, char *buf)
{
	struct tm loctime;
	errno_t ret = time_local2tm(time, &loctime);
	if (ret != EOK)
		return ret;
	
	time_tm2str(&loctime, buf);
	return EOK;
}

/** Calculate the difference between two times, in seconds.
 *
 * @param time1 First time.
 * @param time0 Second time.
 *
 * @return Time difference in seconds.
 *
 */
double difftime(time_t time1, time_t time0)
{
	return (double) (time1 - time0);
}

/** @}
 */
