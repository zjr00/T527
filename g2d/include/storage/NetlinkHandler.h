/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NETLINKHANDLER_H
#define _NETLINKHANDLER_H

#include "NetlinkListener.h"

class NetlinkHandler: public NetlinkListener
{

public:
    NetlinkHandler(int listenerSocket);
    virtual ~NetlinkHandler();

    int start(void);
    int stop(void);

protected:
    virtual void onEvent(NetlinkEvent *evt);
};

typedef void (*aut_event_cb_func)(NetlinkEvent *, void *usrdata);

class AutEvent
{
public:
    AutEvent(){}

    static AutEvent *Instance();
    aut_event_cb_func mCbFunc = NULL;
    void *mUsrDat = NULL;
    //int runEventMon();
    int setEventCb(aut_event_cb_func CbFunc, void *usrdata)
    {
        if (CbFunc == NULL)
            return -1;
        mCbFunc = CbFunc;
        mUsrDat = usrdata;
        return 0;
    }
    int sendEvent(NetlinkEvent *evt)
    {
        if (mCbFunc != NULL)
            mCbFunc(evt, mUsrDat);
        else
            ALOGE("mCbFunc NO REGISYST");
        return 0;
    }
private:
    static  AutEvent *sInstance;
};
#endif
