/**************************************************************************************************
 *
 * Copyright (c) 2024 author. All Rights Reserved.
 *
 * This source file is the property of the author
 * and may not be copied, distributed, or modified
 * without prior written permission.
 *
 **************************************************************************************************/

#pragma once
#include "AppLogApi.h"

class IAppLog {
public:
    virtual ~IAppLog(APP_LOG_VOID) = default;

    virtual APP_LOG_BOOL Open(const APP_LOG_ATTR_T &stAttr) = 0;
    virtual APP_LOG_VOID Log(APP_LOG_S32 nLv, const APP_LOG_CHAR *pStr) = 0;
    virtual APP_LOG_VOID Close(APP_LOG_VOID) = 0;
};