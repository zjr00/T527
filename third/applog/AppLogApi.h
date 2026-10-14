/**************************************************************************************************
 *
 * Copyright (c) 2024 author. All Rights Reserved.
 *
 * This source file is the property of the author
 * and may not be copied, distributed, or modified
 * without prior written permission.
 *
 **************************************************************************************************/

#ifndef __APP_LOG_API_H_
#define __APP_LOG_API_H_

#include <stdio.h>
#include <stdarg.h>
#include "log_base_type.h"
#include "log_config_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MACRO_LOG_COLOR == LOG_ENABLE
#define MACRO_LOG_BLACK "\033[1;30;30m"
#define MACRO_LOG_RED "\033[1;30;31m"
#define MACRO_LOG_GREEN "\033[1;30;32m"
#define MACRO_LOG_YELLOW "\033[1;30;33m"
#define MACRO_LOG_BLUE "\033[1;30;34m"
#define MACRO_LOG_PURPLE "\033[1;30;35m"
#define MACRO_LOG_WHITE "\033[1;30;37m"
#define MACRO_LOG_END "\033[0m\n"
#else
#define MACRO_LOG_BLACK
#define MACRO_LOG_RED
#define MACRO_LOG_GREEN
#define MACRO_LOG_YELLOW
#define MACRO_LOG_BLUE
#define MACRO_LOG_PURPLE
#define MACRO_LOG_WHITE
#define MACRO_LOG_END "\n"
#endif

typedef enum {
    APP_LOG_SYNC_SEND = 0,
    APP_LOG_SYNC_RECV = 1,
    APP_LOG_ASYN_SEND = 2,
    APP_LOG_ASYN_RECV = 3,
    APP_LOG_FLAG_BUTT
} APP_LOG_FLAG_E;

typedef enum {
    APP_LOG_TARGET_NULL = 0,
    APP_LOG_TARGET_SYSLOG = 1,
    APP_LOG_TARGET_APPLOG = 2,
    APP_LOG_TARGET_STDOUT = 4,
    APP_LOG_TARGET_BUTT
} APP_LOG_TARGET_E;

typedef enum {
    APP_LOG_EMERG = 0,
    APP_LOG_ALERT = 1,
    APP_LOG_CRITICAL = 2,
    APP_LOG_ERROR = 3,
    APP_LOG_WARN = 4,
    APP_LOG_NOTICE = 5,
    APP_LOG_INFO = 6,
    APP_LOG_DEBUG = 7,
    APP_LOG_DATA = 8,
    APP_LOG_BUTT
} APP_LOG_LEVEL_E;

#define MAX_APP_NAME_LEN (32)
typedef struct {
    APP_LOG_U32 nTarget;
    APP_LOG_S32 nLv;
    APP_LOG_CHAR szAppName[MAX_APP_NAME_LEN];
} APP_LOG_ATTR_T;

/* APIs */
APP_LOG_S32 LOG_APP_Log_Init(const APP_LOG_ATTR_T *pstAttr);
APP_LOG_VOID LOG_APP_Log_DeInit(APP_LOG_VOID);
APP_LOG_S32 LOG_APP_GetLogLevel(APP_LOG_VOID);
APP_LOG_VOID LOG_APP_SetLogLevel(APP_LOG_S32 nLv);
APP_LOG_VOID LOG_APP_Log_SetOutput(FILE *file);
APP_LOG_VOID LOG_APP_Log_SetSysModuleInited(APP_LOG_BOOL bInited);
const APP_LOG_CHAR *LOG_APP_LogLevelToString(APP_LOG_S32 nLv);
APP_LOG_VOID LOG_APP_LogFmtStr(APP_LOG_S32 nLv, const APP_LOG_CHAR *pFmt, ...);
APP_LOG_VOID LOG_APP_LogBufData(APP_LOG_S32 nLv, const APP_LOG_VOID *pBuf, APP_LOG_U32 nBufSize, APP_LOG_U32 nFlag);
APP_LOG_VOID LOG_APP_LogHexData(APP_LOG_S32 nLv, const APP_LOG_VOID *pBuf, APP_LOG_U32 nBufSize);
APP_LOG_VOID LOG_APP_LogHexString(APP_LOG_S32 nLv, const APP_LOG_VOID *pBuf, APP_LOG_U32 nBufSize);
APP_LOG_VOID LOG_APP_LogProgressBar(APP_LOG_S32 nLv, APP_LOG_F64 elapsed, APP_LOG_F64 total);

#if MACRO_LOG_OPEN == LOG_ENABLE
#define LOGSTR LOG_APP_LogFmtStr
#define LOGBUF LOG_APP_LogBufData
#else
#define LOGSTR
#define LOGBUF
#endif
#define LOG LOG_W
#define LOG_M LOG_M_W
#define LOG_MM LOG_MM_W

#define LOG_A(fmt, ...) LOGSTR(APP_LOG_ALERT, MACRO_LOG_YELLOW "A: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_C(fmt, ...) LOGSTR(APP_LOG_CRITICAL, MACRO_LOG_YELLOW "K: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_E(fmt, ...) LOGSTR(APP_LOG_ERROR, MACRO_LOG_RED "E: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_W(fmt, ...) LOGSTR(APP_LOG_WARN, MACRO_LOG_YELLOW "W: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_I(fmt, ...) LOGSTR(APP_LOG_INFO, MACRO_LOG_GREEN "I: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_D(fmt, ...) LOGSTR(APP_LOG_DEBUG, MACRO_LOG_WHITE "D: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_N(fmt, ...) LOGSTR(APP_LOG_NOTICE, MACRO_LOG_PURPLE "N: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_B(buf, size, flag) LOGBUF(APP_LOG_DATA, buf, size, flag)

#define LOG_M_A(tag, fmt, ...) LOGSTR(APP_LOG_ALERT, MACRO_LOG_YELLOW "A %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_C(tag, fmt, ...) LOGSTR(APP_LOG_CRITICAL, MACRO_LOG_YELLOW "K %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_E(tag, fmt, ...) LOGSTR(APP_LOG_ERROR, MACRO_LOG_RED "E %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_W(tag, fmt, ...) LOGSTR(APP_LOG_WARN, MACRO_LOG_YELLOW "W %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_I(tag, fmt, ...) LOGSTR(APP_LOG_INFO, MACRO_LOG_GREEN "I %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_D(tag, fmt, ...) LOGSTR(APP_LOG_DEBUG, MACRO_LOG_WHITE "D %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_N(tag, fmt, ...) LOGSTR(APP_LOG_NOTICE, MACRO_LOG_PURPLE "N %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)

#define LOG_MM_A(tag, fmt, ...) LOGSTR(APP_LOG_ALERT, MACRO_LOG_YELLOW "A %s <%s>: " fmt MACRO_LOG_END, tag, __func__, ##__VA_ARGS__)
#define LOG_MM_C(tag, fmt, ...) LOGSTR(APP_LOG_CRITICAL, MACRO_LOG_YELLOW "K %s <%s>: " fmt MACRO_LOG_END, tag, __func__, ##__VA_ARGS__)
#define LOG_MM_E(tag, fmt, ...) \
    LOGSTR(APP_LOG_ERROR, MACRO_LOG_RED "E %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_W(tag, fmt, ...) \
    LOGSTR(APP_LOG_WARN, MACRO_LOG_YELLOW "W %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_I(tag, fmt, ...) \
    LOGSTR(APP_LOG_INFO, MACRO_LOG_GREEN "I %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_D(tag, fmt, ...) LOGSTR(APP_LOG_DEBUG, MACRO_LOG_WHITE "D %s <%s>: " fmt MACRO_LOG_END, tag, __func__, ##__VA_ARGS__)
#define LOG_MM_N(tag, fmt, ...) LOGSTR(APP_LOG_NOTICE, MACRO_LOG_PURPLE "N %s <%s>: " fmt MACRO_LOG_END, tag, __func__, ##__VA_ARGS__)

#define LOGA(fmt, ...) LOGSTR(APP_LOG_ALERT, MACRO_LOG_YELLOW LOG_TYPE_PART "<" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " fmt MACRO_LOG_END LOG_TYPE_ARG(APP_LOG_ALERT) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__)
#define LOGC(fmt, ...) LOGSTR(APP_LOG_CRITICAL, MACRO_LOG_YELLOW LOG_TYPE_PART "<" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " fmt MACRO_LOG_END LOG_TYPE_ARG(APP_LOG_CRITICAL) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOGSTR(APP_LOG_ERROR, MACRO_LOG_RED LOG_TYPE_PART "<" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " fmt MACRO_LOG_END LOG_TYPE_ARG(APP_LOG_ERROR) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOGSTR(APP_LOG_WARN, MACRO_LOG_YELLOW LOG_TYPE_PART "<" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " fmt MACRO_LOG_END LOG_TYPE_ARG(APP_LOG_WARN) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOGSTR(APP_LOG_INFO, MACRO_LOG_GREEN LOG_TYPE_PART "<" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " fmt MACRO_LOG_END LOG_TYPE_ARG(APP_LOG_INFO) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__)
#define LOGD(fmt, ...) LOGSTR(APP_LOG_DEBUG, MACRO_LOG_WHITE LOG_TYPE_PART "<" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " fmt MACRO_LOG_END LOG_TYPE_ARG(APP_LOG_DEBUG) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__)
#define LOGN(fmt, ...) LOGSTR(APP_LOG_NOTICE, MACRO_LOG_PURPLE LOG_TYPE_PART "<" LOG_FILE_PART LOG_FUNC_PART LOG_LINE_PART "> " fmt MACRO_LOG_END LOG_TYPE_ARG(APP_LOG_NOTICE) LOG_FILE_ARG LOG_FUNC_ARG LOG_LINE_ARG, ##__VA_ARGS__)

#ifdef __cplusplus
}

#endif
#endif /* __APP_LOG_API_H_ */