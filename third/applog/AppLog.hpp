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
#include <stdio.h>
#include <mutex>
#include <string>
#include "IAppLog.hpp"

#define MAX_APP_LOG_FILE_COUNT (5)

class CAppLog : public IAppLog {
public:
    CAppLog(APP_LOG_VOID) = default;
    virtual ~CAppLog(APP_LOG_VOID) = default;

    APP_LOG_BOOL Open(const APP_LOG_ATTR_T &stAttr) override;
    APP_LOG_VOID Log(APP_LOG_S32 nLv, const APP_LOG_CHAR *pStr) override;
    APP_LOG_VOID Close(APP_LOG_VOID) override;

protected:
    APP_LOG_BOOL SwitchFile(APP_LOG_VOID);

private:
    FILE *m_fp{nullptr};
    std::string m_arrFiles[MAX_APP_LOG_FILE_COUNT];
    std::mutex m_mtx;
};