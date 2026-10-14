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
#include "IAppLog.hpp"
#ifndef __X86__
#include <log_sys_log.h>
#endif
#include <stdio.h>
#include <string.h>

class CSysLog : public IAppLog {
public:
    CSysLog(APP_LOG_VOID) = default;
    ~CSysLog(APP_LOG_VOID) = default;

    APP_LOG_BOOL Open(const APP_LOG_ATTR_T &stAttr) override {
        strcpy(m_szAppName, stAttr.szAppName);
        return APP_LOG_TRUE;
    };

    APP_LOG_VOID SetSysModuleInited(APP_LOG_BOOL bInited) {
        m_bSysModuleInited = bInited;
    };

    APP_LOG_VOID Log(APP_LOG_S32 nLv, const APP_LOG_CHAR *pStr) override {
#ifndef __X86__
        if (m_bSysModuleInited) {
            LOG_SYS_LogPrint(nLv, m_szAppName, "%s", pStr);
        } else {
            fprintf(stdout, "%s", pStr);
        }
#endif
    };

    APP_LOG_VOID Close(APP_LOG_VOID) override{};

private:
    APP_LOG_CHAR m_szAppName[MAX_APP_NAME_LEN]{"APPLOG"};
    APP_LOG_BOOL m_bSysModuleInited{APP_LOG_FALSE};
};