/**************************************************************************************************
 *
 * Copyright (c) 2024 author. All Rights Reserved.
 *
 * This source file is the property of the author
 * and may not be copied, distributed, or modified
 * without prior written permission.
 *
 **************************************************************************************************/

#ifndef __SIMPLELOG_H__
#define __SIMPLELOG_H__
#include "log_config_type.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#if MACRO_LOG_UPTIME == LOG_ENABLE
#include <sys/sysinfo.h>
static inline APP_LOG_U64 Getuptime() {
    struct sysinfo sinfo;
	sysinfo(&sinfo);
    return sinfo.uptime;
}
#endif //MACRO_LOG_TIME
#if MACRO_LOG_DATE  == LOG_ENABLE || \
    MACRO_LOG_TIME  == LOG_ENABLE
#include <time.h>
static inline char* Gettimestring() {
    static __thread char buffer[32];
    struct tm tm_info;
    time_t now = time(NULL);
    localtime_r(&now, &tm_info);
#if MACRO_LOG_DATE == LOG_ENABLE && \
    MACRO_LOG_TIME == LOG_ENABLE
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);
#elif MACRO_LOG_DATE == LOG_ENABLE
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm_info);
#elif MACRO_LOG_TIME == LOG_ENABLE
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm_info);
#endif
    return buffer;
}
#endif

typedef enum { DEBUG_LOGEMERG, DEBUG_LOGALERT, DEBUG_LOGCRIT,  DEBUG_LOGERROR, DEBUG_LOGWARN, \
               DEBUG_LOGNOTICE,  DEBUG_LOGINFO,  DEBUG_LOGDEBUG, DEBUG_LOGDATA,  DEBUG_LOGBUTT
} DEBUG_LogLevel;

extern __attribute__((weak)) volatile DEBUG_LogLevel DEBUG_debuglevel;

static inline const char* GetlevelString(int nLv) {
    return (nLv >= DEBUG_LOGALERT && nLv < DEBUG_LOGBUTT) ? LEVEL_STR[nLv] : LEVEL_STR[DEBUG_LOGBUTT];
}

static inline volatile DEBUG_LogLevel* DEBUG_LogGetLevelPtr() {
    static volatile DEBUG_LogLevel DEBUG_defaultlevel = DEBUG_LOGINFO;
    return &DEBUG_debuglevel ? &DEBUG_debuglevel : &DEBUG_defaultlevel;
}

static inline DEBUG_LogLevel DEBUG_LogGetLevel() {
    return *DEBUG_LogGetLevelPtr();
}

static inline void DEBUG_LogSetLevel(DEBUG_LogLevel lvl) {
    volatile DEBUG_LogLevel* pCurrentlevel = DEBUG_LogGetLevelPtr();
    *pCurrentlevel = (lvl > DEBUG_LOGBUTT) ? DEBUG_LOGBUTT : lvl;
	fprintf(stderr,"DEBUG LOG LEVEL IS SET TO %d : %s\n", \
        *pCurrentlevel, GetlevelString(*pCurrentlevel));
}

#if (MACRO_LOG_DATE == LOG_ENABLE || \
     MACRO_LOG_TIME == LOG_ENABLE) && \
     MACRO_LOG_UPTIME == LOG_ENABLE
#define LOG_FMT_PREFIX "[%s %12lld] "
#define LOG_ARGS_PREFIX , Gettimestring(), Getuptime()
#elif MACRO_LOG_DATE == LOG_ENABLE || \
      MACRO_LOG_TIME == LOG_ENABLE
#define LOG_FMT_PREFIX "[%s] "
#define LOG_ARGS_PREFIX , Gettimestring()
#elif MACRO_LOG_UPTIME == LOG_ENABLE
#define LOG_FMT_PREFIX "[%12lld] "
#define LOG_ARGS_PREFIX , Getuptime()
#else
#define LOG_FMT_PREFIX ""
#define LOG_ARGS_PREFIX
#endif

#if MACRO_LOG_OPEN == LOG_ENABLE
#define SLOG(Severity, format, ...) do { \
    if (Severity <= DEBUG_LogGetLevel()) { \
        printf(LOG_FMT_PREFIX "[%s] <" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " format "\n" \
        LOG_ARGS_PREFIX, GetlevelString(Severity) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__); \
    } \
} while(0)
#else
#define SLOG(Severity, format, ...)
#endif

#define SLOGA(...) SLOG(DEBUG_LOGALERT, __VA_ARGS__)
#define SLOGC(...) SLOG(DEBUG_LOGCRIT, __VA_ARGS__)
#define SLOGE(...) SLOG(DEBUG_LOGERROR, __VA_ARGS__)
#define SLOGW(...) SLOG(DEBUG_LOGWARN, __VA_ARGS__)
#define SLOGI(...) SLOG(DEBUG_LOGINFO, __VA_ARGS__)
#define SLOGD(...) SLOG(DEBUG_LOGDEBUG, __VA_ARGS__)
#define SLOGN(...) SLOG(DEBUG_LOGNOTICE, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
