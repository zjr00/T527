/**************************************************************************************************
 *
 * Copyright (c) 2024 author. All Rights Reserved.
 *
 * This source file is the property of the author
 * and may not be copied, distributed, or modified
 * without prior written permission.
 *
 **************************************************************************************************/

#ifndef _LOG_BASE_TYPE_H_
#define _LOG_BASE_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* types of variables typedef */
typedef unsigned long long int  APP_LOG_U64;
typedef unsigned int            APP_LOG_U32;
typedef unsigned short          APP_LOG_U16;
typedef unsigned char           APP_LOG_U8;
typedef long long int           APP_LOG_S64;
typedef int                     APP_LOG_S32;
typedef short                   APP_LOG_S16;
typedef signed char             APP_LOG_S8;
typedef char                    APP_LOG_CHAR;
typedef long                    APP_LOG_LONG;
typedef unsigned long           APP_LOG_ULONG;
typedef unsigned long           APP_LOG_ADDR;
typedef float                   APP_LOG_F32;
typedef double                  APP_LOG_F64;
typedef void                    APP_LOG_VOID;
typedef unsigned int            APP_LOG_SIZE_T;

typedef enum {
    APP_LOG_FALSE = 0,
    APP_LOG_TRUE  = 1,
} APP_LOG_BOOL;

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

static const APP_LOG_CHAR *LEVEL_STR[] __attribute__((unused)) = {
    "EMERG  ",
    "ALERT  ",
    "CRIT   ",
    "ERROR  ",
    "WARNING",
    "NOTICE ",
    "INFO   ",
    "DEBUG  ",
    "DATA   ",
    "UNKNOWN",
};

#ifndef LOG_SUCCESS
#define LOG_SUCCESS                  (0)
#endif

#ifndef LOG_ENABLE
#define LOG_ENABLE                   (1)
#endif

#ifndef LOG_DISABLE
#define LOG_DISABLE                  (0)
#endif

#ifndef LOG_NULL
#if defined(__cplusplus)
#define LOG_NULL 0
#else
#define LOG_NULL ((void *)0)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
