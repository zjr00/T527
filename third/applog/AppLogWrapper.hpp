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
#include "AppLog.hpp"
#include "StdLog.hpp"
#include "SysLog.hpp"

#ifdef __cplusplus
extern "C" {
#endif

class CAppLogWrapper {
public:
    CAppLogWrapper(APP_LOG_VOID) = default;
    virtual ~CAppLogWrapper(APP_LOG_VOID) = default;

    virtual APP_LOG_S32 Init(const APP_LOG_ATTR_T *pstAttr);
    virtual APP_LOG_VOID DeInit(APP_LOG_VOID);

    APP_LOG_S32 GetLogLevel(APP_LOG_VOID) const {
        return m_stAttr.nLv;
    };

    APP_LOG_VOID SetLogLevel(APP_LOG_S32 nLv) {
        if (nLv >= APP_LOG_ALERT && nLv < APP_LOG_BUTT) {
            m_stAttr.nLv = nLv;
        }
    };

    APP_LOG_VOID SetOutput(FILE *file) {
        if (m_pStdLog) {
            m_pStdLog->SetOutput(file);
        }
    }

    APP_LOG_VOID SetSysModuleInited(APP_LOG_BOOL bInited) {
        if (m_pSysLog) {
            m_pSysLog->SetSysModuleInited(bInited);
        }
    }

    virtual APP_LOG_VOID LogArgStr(APP_LOG_S32 nLv, const APP_LOG_CHAR *pFmt, va_list va);
    virtual APP_LOG_VOID LogFmtStr(APP_LOG_S32 nLv, const APP_LOG_CHAR *pFmt, ...);
    virtual APP_LOG_VOID LogBufData(APP_LOG_S32 nLv, const APP_LOG_VOID *pBuf, APP_LOG_U32 nBufSize, APP_LOG_U32 nFlag);
    virtual APP_LOG_VOID LogHexData(APP_LOG_S32 nLv, const APP_LOG_VOID *pBuf, APP_LOG_U32 nBufSize);
    virtual APP_LOG_VOID LogHexString(APP_LOG_S32 nLv, const APP_LOG_VOID *pBuf, APP_LOG_U32 nBufSize);
    virtual APP_LOG_VOID LogProgressBar(APP_LOG_S32 nLv, APP_LOG_F64 elapsed, APP_LOG_F64 total);

protected:
    APP_LOG_VOID Logging(APP_LOG_S32 nLv, const APP_LOG_CHAR *pStr) {
        if (m_pSysLog) {
            m_pSysLog->Log(nLv, pStr);
        }

        if (m_pStdLog) {
            m_pStdLog->Log(nLv, pStr);
        }

        if (m_pAppLog) {
            m_pAppLog->Log(nLv, pStr);
        }
    };

    static APP_LOG_U64 GetTickCount(APP_LOG_VOID);
    static APP_LOG_U64 GetUpTime(APP_LOG_VOID);

protected:
    APP_LOG_ATTR_T m_stAttr;
    CSysLog *m_pSysLog{nullptr};
    CStdLog *m_pStdLog{nullptr};
    CAppLog *m_pAppLog{nullptr};
};

#ifdef __cplusplus
}
#endif