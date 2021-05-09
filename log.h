/*
 * Copyright (c) 2000-2004 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
#ifndef _LOG_H_
#define _LOG_H_

#ifdef __GNUC__
#define EV_CHECK_FMT(a,b) __attribute__((format(printf, a, b)))
#else
#define EV_CHECK_FMT(a,b)
#endif

void _event_err(const char *file_name, const char *func_name, const int line_num, int eval, const char *fmt, ...) EV_CHECK_FMT(5,6);
void _event_warn(const char *file_name, const char *func_name, const int line_num, const char *fmt, ...) EV_CHECK_FMT(4,5);
void _event_errx(const char *file_name, const char *func_name, const int line_num, int eval, const char *fmt, ...) EV_CHECK_FMT(5,6);
void _event_warnx(const char *file_name, const char *func_name, const int line_num, const char *fmt, ...) EV_CHECK_FMT(4,5);
void _event_msgx(const char *file_name, const char *func_name, const int line_num, const char *fmt, ...) EV_CHECK_FMT(4,5);
void _internal_event_debugx(const char *file_name, const char *func_name, const int line_num,const char *fmt, ...) EV_CHECK_FMT(4,5);

#define event_err(format, args...)      _event_err(__FILE__, __FUNCTION__, __LINE__, format, ##args)
#define event_warn(format, args...)     _event_warn(__FILE__, __FUNCTION__, __LINE__, format, ##args)
#define event_errx(format, args...)     _event_errx(__FILE__, __FUNCTION__, __LINE__, format, ##args)
#define event_warnx(format, args...)    _event_warnx(__FILE__, __FUNCTION__, __LINE__, format, ##args)
#define event_msgx(format, args...)      _event_msgx(__FILE__, __FUNCTION__, __LINE__, format, ##args)
#define internal_event_debugx(format, args...)       _internal_event_debugx(__FILE__, __FUNCTION__, __LINE__, format, ##args)


#ifdef USE_DEBUG
#define event_debug(x) internal_event_debugx x
#else
#define event_debug(x) do {;} while (0)
#endif

#undef EV_CHECK_FMT

#endif
