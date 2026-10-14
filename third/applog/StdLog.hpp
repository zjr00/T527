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
#include <string>
#include "IAppLog.hpp"

class CStdLog : public IAppLog {
public:
    CStdLog(APP_LOG_VOID) = default;
    ~CStdLog(APP_LOG_VOID) = default;

    APP_LOG_BOOL Open(const APP_LOG_ATTR_T &stAttr) override {
        return APP_LOG_TRUE;
    };

    APP_LOG_VOID Log(APP_LOG_S32 nLv, const APP_LOG_CHAR *pStr) override {
        if ( !fmsg ) fmsg = stderr;
        fprintf(fmsg, "%s", pStr);
    };

    APP_LOG_VOID Close(APP_LOG_VOID) override{};

    APP_LOG_VOID SetOutput(FILE *file) {
	    fmsg = file;
    };

private:
    FILE *fmsg;
};