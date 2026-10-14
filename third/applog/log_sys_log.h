/**************************************************************************************************
 *
 * Copyright (c) 2024 author. All Rights Reserved.
 *
 * This source file is the property of the author
 * and may not be copied, distributed, or modified
 * without prior written permission.
 *
 **************************************************************************************************/

#ifndef _LOG_SYS_LOG_H_
#define _LOG_SYS_LOG_H_
#include "log_config_type.h"
#include <syslog.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <string.h>
#include <sys/klog.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef __cplusplus
extern "C"
{
#endif

/*
    LOG_EMERG     system is unusable
    LOG_ALERT     action must be taken immediately
    LOG_CRIT      critical conditions
    LOG_ERR       error conditions
    LOG_WARNING   warning conditions
    LOG_NOTICE    normal, but significant, condition
    LOG_INFO      informational message
    LOG_DEBUG     debug-level message
*/

typedef enum { System_LOGEMERG, System_LOGALERT, System_LOGCRIT,  System_LOGERROR, System_LOGWARN, \
               System_LOGNOTICE,  System_LOGINFO,  System_LOGDEBUG, System_LOGDATA,  System_LOGBUTT
} System_LogLevel;

static inline APP_LOG_VOID LOG_SYS_LogPrint(APP_LOG_S32 nLv, APP_LOG_CHAR const *tag, APP_LOG_CHAR const *pFormat, ...) {
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
    va_list vlist;
    APP_LOG_CHAR szBuf[1024] = {0};
    va_start(vlist, pFormat);
    APP_LOG_S32 nLen = vsnprintf(szBuf, sizeof(szBuf), pFormat, vlist);
    va_end(vlist);
    if (nLen > 0) {
        APP_LOG_CHAR szLog[1280] = {0};
        snprintf(szLog, sizeof(szLog), "[%s][%s] %s", \
                (nLv >= System_LOGALERT && nLv < System_LOGBUTT) ? LEVEL_STR[nLv] : LEVEL_STR[System_LOGBUTT], tag, szBuf);
        pthread_mutex_lock(&log_mutex);
        int fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
        if (fd >= 0) {
            write(fd, szLog, strlen(szLog));
            close(fd);
        } else {
            if(klogctl(3, szLog, strlen(szLog)) < 0)
                syslog(nLv | LOG_LOCAL0, "%s", szLog);
        }
    }
    pthread_mutex_unlock(&log_mutex);
}

#define LOG_SYS_LogPrint_Ex LOG_SYS_LogPrint

#ifdef __cplusplus
}
#endif

#define MACRO_BLACK         "\033[1;30;30m"
#define MACRO_RED           "\033[1;30;31m"
#define MACRO_GREEN         "\033[1;30;32m"
#define MACRO_YELLOW        "\033[1;30;33m"
#define MACRO_BLUE          "\033[1;30;34m"
#define MACRO_PURPLE        "\033[1;30;35m"
#define MACRO_WHITE         "\033[1;30;37m"
#define MACRO_END           "\033[0m"

#define LOG_MSYS_LOG_TAG  "MSYS"
#define LOG_ISP_LOG_TAG  "ISP"

#define LOG_LOG_EMERG(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_EMERGENCY, tag, "[M][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_LOG_ALERT(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_ALERT, tag, "[A][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_LOG_CRIT(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_CRITICAL, tag, "[C][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_LOG_ERR(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_ERROR, tag, "[E][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_LOG_WARN(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_WARN, tag, "[W][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_LOG_INFO(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_INFO, tag, "[I][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_LOG_DBG(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_DEBUG, tag, "[D][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_LOG_NOTICE(tag,fmt,...) \
    LOG_SYS_LogPrint(SYS_LOG_NOTICE, tag, "[N][%32s][%4d]: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#endif //_LOG_SYS_LOG_H_
