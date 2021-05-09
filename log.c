/*	$OpenBSD: err.c,v 1.2 2002/06/25 15:50:15 mickey Exp $	*/

/*
 * log.c
 *
 * Based on err.c, which was adapted from OpenBSD libc *err* *warn* code.
 *
 * Copyright (c) 2005 Nick Mathewson <nickm@freehaven.net>
 *
 * Copyright (c) 2000 Dug Song <dugsong@monkey.org>
 *
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include "misc.h"
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys/_time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>

#include "event.h"

#include "log.h"

static void _warn_helper(const char *file_name, const char *func_name, const int line_num, int severity, int log_errno, const char *fmt,
                         va_list ap);
static void event_log(int severity, const char *msg);

static int
event_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int r;
	if (size == 0)
		return -1;
#ifdef WIN32
	r = _vsnprintf(str, size, format, args);
#else
	r = vsnprintf(str, size, format, args);
#endif
	str[size-1] = '\0';
	if (r < 0 || ((size_t)r) >= size) {
		/* different platforms behave differently on overflow;
		 * handle both kinds. */
		return -1;
	}
	return r;
}

static int
event_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int r;
    va_start(ap, format);
    r = event_vsnprintf(str, size, format, ap);
    va_end(ap);
    return r;
}

void
_event_err(const char *file_name, const char *func_name, const int line_num, int eval, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	_warn_helper(file_name, func_name, line_num, _EVENT_LOG_ERR, errno, fmt, ap);
	va_end(ap);
	exit(eval);
}

void
_event_warn(const char *file_name, const char *func_name, const int line_num, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	_warn_helper(file_name, func_name, line_num, _EVENT_LOG_WARN, errno, fmt, ap);
	va_end(ap);
}

void
_event_errx(const char *file_name, const char *func_name, const int line_num, int eval, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	_warn_helper(file_name, func_name, line_num, _EVENT_LOG_ERR, -1, fmt, ap);
	va_end(ap);
	exit(eval);
}

void
_event_warnx(const char *file_name, const char *func_name, const int line_num, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	_warn_helper(file_name, func_name, line_num, _EVENT_LOG_WARN, -1, fmt, ap);
	va_end(ap);
}

void
_event_msgx(const char *file_name, const char *func_name, const int line_num, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	_warn_helper(file_name, func_name, line_num, _EVENT_LOG_MSG, -1, fmt, ap);
	va_end(ap);
}

void
_internal_event_debugx(const char *file_name, const char *func_name, const int line_num, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	_warn_helper(file_name, func_name, line_num, _EVENT_LOG_DEBUG, -1, fmt, ap);
	va_end(ap);
}

#define MAX_CURR_TIME_STRING_LEN  27  // strlen("2015-10-23 09:15:59.737940") + 1

static inline char * get_curr_time_string(char * time_string_buffer, int buffer_len)
{
    struct timeval tv;
    struct tm tm;
    
    if (buffer_len < MAX_CURR_TIME_STRING_LEN)
    {
        return NULL;
    }
    
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    
    tm.tm_year += 1900;
    tm.tm_mon += 1;
    
    snprintf(time_string_buffer, buffer_len, "%4d-%02d-%02d %02d:%02d:%02d.%06d",
             tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (uint32_t)(tv.tv_usec));
    
    return time_string_buffer;
}

static void
_warn_helper(const char *file_name, const char *func_name, const int line_num, int severity, int log_errno, const char *fmt, va_list ap)
{
	char buf[1024] = {0};
	size_t len;

	char * base_file_name = NULL;
	base_file_name = basename((char *)file_name);
	char curr_time_string[MAX_CURR_TIME_STRING_LEN];

	get_curr_time_string(curr_time_string, MAX_CURR_TIME_STRING_LEN);
    int index = snprintf(buf, sizeof(buf),"[%s][%s:%s():%d]:", curr_time_string, base_file_name, func_name, line_num);

	if (fmt != NULL)
		event_vsnprintf(buf + index, sizeof(buf) - index, fmt, ap);

	if (log_errno >= 0) {
		len = strlen(buf);
		if (len < sizeof(buf) - 3) {
			event_snprintf(buf + len, sizeof(buf) - len, ": %s",
			    strerror(log_errno));
		}
	}

	event_log(severity, buf);
}

static event_log_cb log_fn = NULL;

void
event_set_log_callback(event_log_cb cb)
{
	log_fn = cb;
}

static void
event_log(int severity, const char *msg)
{
	if (log_fn)
		log_fn(severity, msg);
	else {
		const char *severity_str;
		switch (severity) {
		case _EVENT_LOG_DEBUG:
			severity_str = "debug";
			break;
		case _EVENT_LOG_MSG:
			severity_str = "msg";
			break;
		case _EVENT_LOG_WARN:
			severity_str = "warn";
			break;
		case _EVENT_LOG_ERR:
			severity_str = "err";
			break;
		default:
			severity_str = "???";
			break;
		}
		(void)fprintf(stderr, "[%s] %s\n", severity_str, msg);
	}
}
