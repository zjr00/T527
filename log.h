/*
 *  Copyright (C) 2024
 *
 *  This file is part of log debug.
 *
 */

#ifndef __DEBUG_LOG_H__
#define __DEBUG_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Enable this to get full debugging output */
#define _DEBUG
/* Enable this to add time debugging output hour:min:sec */
#define _DEBUG_TIME
/* Enable this to add date debugging output year-mon-day hour:min:sec */
#define _DEBUG_DATE
/* Enable this to add detail debugging output */
#define _DEBUG_DETAIL

#ifdef _DEBUG
#undef NODEBUG
#endif

#ifdef _DEBUG_TIME
#include <time.h>
#include <sys/sysinfo.h>
#endif

#ifdef _DEBUG_DETAIL
#define DEBUG_LOG(level, format, ...) \
    DEBUG_Log(level, "<%s/%s/line:%d> " format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_LOG(level, format,...) \
    DEBUG_Log(level, format, ##__VA_ARGS__)
#endif

typedef enum
{ DEBUG_LOGCRIT=0, DEBUG_LOGERROR, DEBUG_LOGWARNING, DEBUG_LOGINFO,
  DEBUG_LOGDEBUG, DEBUG_LOGDEBUG2, DEBUG_LOGALL
} DEBUG_LogLevel;

extern DEBUG_LogLevel DEBUG_debuglevel;

typedef void (DEBUG_LogCallback)(int level, const char *fmt, va_list);
void DEBUG_LogSetCallback(DEBUG_LogCallback *cb);
void DEBUG_LogSetOutput(FILE *file);
#ifdef __GNUC__
void DEBUG_LogPrintf(const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void DEBUG_LogStatus(const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void DEBUG_Log(int level, const char *format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
#else
void DEBUG_LogPrintf(const char *format, ...);
void DEBUG_LogStatus(const char *format, ...);
void DEBUG_Log(int level, const char *format, ...);
#endif
void DEBUG_LogHex(int level, const uint8_t *data, unsigned long len);
void DEBUG_LogHexString(int level, const uint8_t *data, unsigned long len);
void DEBUG_LogSetLevel(DEBUG_LogLevel lvl);
DEBUG_LogLevel DEBUG_LogGetLevel(void);

static const char* level_to_string(int level) {
    switch (level) {
        case DEBUG_LOGCRIT: return "CRIT";
        case DEBUG_LOGERROR: return "ERROR";
        case DEBUG_LOGWARNING: return "WARNING";
        case DEBUG_LOGINFO: return "INFO";
        case DEBUG_LOGDEBUG: return "DEBUG";
        case DEBUG_LOGDEBUG2: return "DEBUG2";
        case DEBUG_LOGALL: return "BEBUGALL";
        default: return "UNKNOWN";
    }
}

#define LOG(Severity, ...) DEBUG_LOG(Severity, ##__VA_ARGS__)
#define LOGD(...) LOG(DEBUG_LOGDEBUG, ##__VA_ARGS__)
#define LOGI(...) LOG(DEBUG_LOGINFO, ##__VA_ARGS__)
#define LOGW(...) LOG(DEBUG_LOGWARNING, ##__VA_ARGS__)
#define LOGE(...) LOG(DEBUG_LOGERROR, ##__VA_ARGS__)
#define LOGC(...) LOG(DEBUG_LOGCRIT, ##__VA_ARGS__) abort();

#ifdef __cplusplus
}
#endif

#endif
