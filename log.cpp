/*
 *  Copyright (C) 2024
 *
 *  This file is part of log debug.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "log.h"

#define MAX_PRINT_LEN	2048

DEBUG_LogLevel DEBUG_debuglevel = DEBUG_LOGDEBUG2;

static int neednl;

static FILE *fmsg;

static DEBUG_LogCallback debug_log_default, *cb = debug_log_default;

static const char *levels[] = {
  "CRIT", "ERROR", "WARNING", "INFO",
  "DEBUG", "DEBUG2"
};

static void debug_log_default(int level, const char *format, va_list vl)
{
	char str[MAX_PRINT_LEN]="";

	vsnprintf(str, MAX_PRINT_LEN-1, format, vl);

	/* Filter out 'no-name' */
	if ( DEBUG_debuglevel<DEBUG_LOGALL && strstr(str, "no-name" ) != NULL )
		return;

	if ( !fmsg ) fmsg = stderr;

	if ( level <= (int)DEBUG_debuglevel ) {
		if (neednl) {
			putc('\n', fmsg);
			neednl = 0;
		}
#ifdef _DEBUG_DATE
		struct sysinfo sinfo;
		time_t now = time(NULL);
    	struct tm *t = localtime(&now);
    	char time_str[0x20];
		sysinfo(&sinfo);
    	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
		fprintf(fmsg, "[%s %12ld] %s: %s\n", time_str, sinfo.uptime, levels[level], str);
#elif defined(_DEBUG_TIME)
		fprintf(fmsg, "[%s] %s: %s\n", __TIME__, levels[level], str);
#else
		fprintf(fmsg, "%s: %s\n", levels[level], str);
#endif
#ifdef _DEBUG
		fflush(fmsg);
#endif
	}
}

void DEBUG_LogSetOutput(FILE *file)
{
	fmsg = file;
}

void DEBUG_LogSetLevel(DEBUG_LogLevel level)
{
	DEBUG_debuglevel = level;
}

void DEBUG_LogSetCallback(DEBUG_LogCallback *cbp)
{
	cb = cbp;
}

DEBUG_LogLevel DEBUG_LogGetLevel()
{
	return DEBUG_debuglevel;
}

void DEBUG_Log(int level, const char *format, ...)
{
	va_list args;

	if ( level > (int)DEBUG_debuglevel )
		return;

	va_start(args, format);
	cb(level, format, args);
	va_end(args);
}

static const char hexdig[] = "0123456789abcdef";

void DEBUG_LogHex(int level, const uint8_t *data, unsigned long len)
{
	unsigned long i;
	char line[50], *ptr;

	if ( level > (int)DEBUG_debuglevel )
		return;

	ptr = line;

	for(i=0; i<len; i++) {
		*ptr++ = hexdig[0x0f & (data[i] >> 4)];
		*ptr++ = hexdig[0x0f & data[i]];
		if ((i & 0x0f) == 0x0f) {
			*ptr = '\0';
			ptr = line;
			DEBUG_Log(level, "%s", line);
		} else {
			*ptr++ = ' ';
		}
	}
	if (i & 0x0f) {
		*ptr = '\0';
		DEBUG_Log(level, "%s", line);
	}
}

void DEBUG_LogHexString(int level, const uint8_t *data, unsigned long len)
{
#define BP_OFFSET 9
#define BP_GRAPH 60
#define BP_LEN	80
	char	line[BP_LEN];
	unsigned long i;

	if ( !data || level > (int)DEBUG_debuglevel )
		return;

	/* in case len is zero */
	line[0] = '\0';

	for ( i = 0 ; i < len ; i++ ) {
		int n = i % 16;
		unsigned off;

		if( !n ) {
			if( i ) DEBUG_Log( level, "%s", line );
			memset( line, ' ', sizeof(line)-2 );
			line[sizeof(line)-2] = '\0';

			off = i % 0x0ffffU;

			line[2] = hexdig[0x0f & (off >> 12)];
			line[3] = hexdig[0x0f & (off >>  8)];
			line[4] = hexdig[0x0f & (off >>  4)];
			line[5] = hexdig[0x0f & off];
			line[6] = ':';
		}

		off = BP_OFFSET + n*3 + ((n >= 8)?1:0);
		line[off] = hexdig[0x0f & ( data[i] >> 4 )];
		line[off+1] = hexdig[0x0f & data[i]];

		off = BP_GRAPH + n + ((n >= 8)?1:0);

		if ( isprint( data[i] )) {
			line[BP_GRAPH + n] = data[i];
		} else {
			line[BP_GRAPH + n] = '.';
		}
	}

	DEBUG_Log( level, "%s", line );
}

/* These should only be used by apps, never by the library itself */
void DEBUG_LogPrintf(const char *format, ...)
{
	char str[MAX_PRINT_LEN]="";
	int len;
	va_list args;
	va_start(args, format);
	len = vsnprintf(str, MAX_PRINT_LEN-1, format, args);
	va_end(args);

	if ( DEBUG_debuglevel==DEBUG_LOGCRIT )
		return;

	if ( !fmsg ) fmsg = stderr;

	if (neednl) {
		putc('\n', fmsg);
		neednl = 0;
	}

    if (len > MAX_PRINT_LEN-1)
          len = MAX_PRINT_LEN-1;
	fprintf(fmsg, "%s", str);
    if (str[len-1] == '\n')
		fflush(fmsg);
}

void DEBUG_LogStatus(const char *format, ...)
{
	char str[MAX_PRINT_LEN]="";
	va_list args;
	va_start(args, format);
	vsnprintf(str, MAX_PRINT_LEN-1, format, args);
	va_end(args);

	if ( DEBUG_debuglevel==DEBUG_LOGCRIT )
		return;

	if ( !fmsg ) fmsg = stderr;

	fprintf(fmsg, "%s", str);
	fflush(fmsg);
	neednl = 1;
}