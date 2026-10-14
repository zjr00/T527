/**************************************************************************************************
 *
 * Copyright (c) 2024 author. All Rights Reserved.
 *
 * This source file is the property of the author
 * and may not be copied, distributed, or modified
 * without prior written permission.
 *
 **************************************************************************************************/

#ifndef _LOG_CONFIG_TYPE_H_
#define _LOG_CONFIG_TYPE_H_
#include "log_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All the following settings are enabled by LOG_ENABLE. */
/* All the following settings are disabled by LOG_DISABLE. */

/* Enable this to open log */
#define MACRO_LOG_OPEN      (LOG_ENABLE)

/* Enable this to add color to log */
#define MACRO_LOG_COLOR     (LOG_ENABLE)

/* Enable this to add date to log *//*format: yyyy-mm-dd*/
#define MACRO_LOG_DATE      (LOG_ENABLE)

/* Enable this to add time to log *//*format: hh-mm-ss*/
#define MACRO_LOG_TIME      (LOG_ENABLE)

/* Enable this to add system uptime to log */
#define MACRO_LOG_UPTIME    (LOG_ENABLE)

/* Need MACRO_LOG_UPTIME is enabled */
/* Chose SEC OR USEC add to log */
#define MACRO_UPTIME_SEC    (LOG_ENABLE)
#define MACRO_UPTIME_USEC   (!(MACRO_UPTIME_SEC))

/* Enable this to add kernel thread ID to log*/
#define MACRO_LOG_TID       (LOG_ENABLE)

/* Enable this to add detail info to log */
#define MACRO_LOG_DETAIL    (LOG_ENABLE)

/* Enable this to add type detail info to log */
#define MACRO_LOG_TYPE      (LOG_ENABLE)
/* Enable this to add detail file to log */
#define MACRO_LOG_FILE      ((LOG_ENABLE)&(MACRO_LOG_DETAIL))
/* Enable this to add detail func to log */
#define MACRO_LOG_FUNC      ((LOG_ENABLE)&(MACRO_LOG_DETAIL))
/* Enable this to add detail line to log */
#define MACRO_LOG_LINE      ((LOG_ENABLE)&(MACRO_LOG_DETAIL))

static const char* LOG_LEVEL_CHARS[] __attribute__((unused)) = {"E","A", "K", "E", "W", "N", "I", "D", "D","U"};
#define LOG_TYPE_PART "[%s]: "
#if MACRO_LOG_TYPE == LOG_DISABLE
    #define LOG_TYPE_ARG(nLv) , LOG_APP_LogLevelToString(nLv)
#else
    #define LOG_TYPE_ARG(nLv) , \
        ((nLv) >= 1 && (nLv) < sizeof(LOG_LEVEL_CHARS)/sizeof(LOG_LEVEL_CHARS[0]) ? \
        LOG_LEVEL_CHARS[nLv] : LOG_LEVEL_CHARS[9])
#endif //MACRO_LOG_TYPE

#if MACRO_LOG_FILE == LOG_ENABLE
    #define LOG_FILE_PART "%s"
    #define LOG_FILE_ARG , __FILE__
#else
    #define LOG_FILE_PART ""
    #define LOG_FILE_ARG
#endif //MACRO_LOG_FILE

#if MACRO_LOG_FUNC == LOG_ENABLE
    #if MACRO_LOG_FILE == LOG_ENABLE
        #define LOG_FUNC_PART "::%s"
    #else
        #define LOG_FUNC_PART "%s"
    #endif //MACRO_LOG_FILE
    #define LOG_FUNC_ARG , __func__
#else
    #define LOG_FUNC_PART ""
    #define LOG_FUNC_ARG
#endif //MACRO_LOG_FUNC

#if MACRO_LOG_LINE == LOG_ENABLE
    #if MACRO_LOG_FILE == LOG_ENABLE || MACRO_LOG_FUNC == LOG_ENABLE
        #define LOG_LINE_PART ":@%d"
    #else
        #define LOG_LINE_PART "line:%d"
    #endif
    #define LOG_LINE_ARG , __LINE__
#else
    #define LOG_LINE_PART ""
    #define LOG_LINE_ARG
#endif //LOG_LINE_ENABLE

#ifdef __cplusplus
}
#endif

#endif

