#include "SipClient.h"
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>



SipClient::SipClient()
{
    
}

SipClient::~SipClient()
{
    //gb28181_client_close();
}

void SipClient::addParameterCallback(int tslKey, OnParameterCallback callback)
{
    parameterCallbacks_[tslKey].push_back(callback);
}

void SipClient::clearParameterCallbacks()
{
    parameterCallbacks_.clear();
}

int SipClient::sipClient_start()
{
    // std::thread test(&SipClient::test_h264, this);
    // test.detach();
    

    sipConfig.g_client.threads = std::thread([this] { gb28181_client_func(this); });
    if(!sipConfig.g_client.threads.joinable())
    {
        LOGE("gb28181 client start error\n");
        return -1;
    }
    // if(pthread_create(&sipConfig.g_client.pid, NULL, gb28181_client_func, NULL) < 0) {
    //     LOGE("gb28181 client start error\n");
    //     return -1;
    // }
   
    LOGD("gb28181 client start");
    return 1;
}

int SipClient::get_max_channels(void)
{
    return SipConfig::g_max_channels;
}

int SipClient::set_max_channels(int chn)
{
    SipConfig::g_max_channels = chn;
    return 0;
}

int SipClient::set_fifo_offset(int ofs)
{
    SipConfig::g_realtime_fifo_offset = ofs;
    return 0;
}

void SipClient::test_h264()
{
    int prefixLen;
    int i;
    unsigned char naluType;
    unsigned char H264Stream[1024*1024] = {0};
    char *filepath = "./data/video0/2022-12-14T10-24-40.h264";
    FILE *fp = fopen(filepath, "rb");
    if(fp == NULL){
        LOGE("fopen error:%s\n",filepath);
        return ;
    }
    while(true)
    {
        int size = Config::getNextNalu(fp, (unsigned char*)H264Stream);
        if (size <= 0) {
            LOGD("Send data completed, Play from the beginning\n");
            fseek(fp, 0, SEEK_SET);
            continue;
        }
        if(Config::getNalupos2((unsigned char*)H264Stream, size, &prefixLen, &naluType) < 0)
        {
            continue;
        }
        if((naluType&0x1F) == 0x07)
        {
            size+=Config::getNextNalu(fp, (unsigned char*)(H264Stream+size));
        }
        for(i=0;i<MAX_CHN;i++)
            sipConfig.SendGbStreamData(i, H264Stream,size, 0);
        //rtmp_push_stream_data(i, H264Stream,size, 0);
        //rtsp_push_stream_data(i+1, H264Stream,size, 0);
        if((naluType&0x1F) == 0x01 || (naluType&0x1F) == 0x05)
            usleep(39*1000);
    }
    fclose(fp);
}

// 修改
int SipClient::gb28181_client_init(void)
{
    sipConfig.SetSipClientDefaultInfo();
    return 0;
}

// 修改
void *SipClient::gb28181_client_func(void *arg)
{
    int64_t lastKeepaliveTimestamp = 0;
    int64_t lastRegistereTimestamp = 0;
    int64_t curTimestamp = 0;
    int64_t hbInterval = sipConfig.g_sipinfo.heartBeatInterval*1000;// 客户端发送keepalive检测的间隔，单位：毫秒  3000
    int64_t regInterval = sipConfig.g_sipinfo.regExpires*1000;// 客户端发送keepalive检测的间隔，单位：毫秒  3000
    char localip[128] = {0};
    sipConfig.g_client.quit = false;
    sipConfig.g_client.allowReg = true;
    sipConfig.g_client.errorCountReg = 0;
    sipConfig.g_client.mRegistered = false;

    if(init_sip_client() != 0) {
        goto exit;
    }
    while (!sipConfig.g_client.quit) {
        // 首次发起注册
        if (sipConfig.g_client.allowReg && !sipConfig.g_client.mRegistered) {
                sipConfig.g_client.allowReg = false;
            request_register();
        }
        // 心跳机制 start （开发过程中，为防止影响抓包，可以先注释）
        if(sipConfig.g_client.mRegistered){
            curTimestamp = sipConfig.getCurTimestamp();
            if(lastKeepaliveTimestamp == 0) {
                lastKeepaliveTimestamp = curTimestamp - hbInterval - 1;
            } else {
                if((curTimestamp - lastKeepaliveTimestamp) > hbInterval) {
                    request_message_keepalive();
                    if(eXosip_guess_localip(mSipCtx,AF_INET,localip,sizeof(localip)) == 0) {
                        if(strcmp(localip,sipConfig.g_sipinfo.deviceIp)!=0)
                            strncpy(sipConfig.g_sipinfo.deviceIp,localip,sizeof(sipConfig.g_sipinfo.deviceIp));
                    }
                    lastKeepaliveTimestamp = curTimestamp;
                    lastRegistereTimestamp = curTimestamp;
                }
            }
            if(lastRegistereTimestamp == 0) {
                lastRegistereTimestamp = curTimestamp;
            } else {
                if((curTimestamp - lastRegistereTimestamp) > regInterval) {
                    request_register();
                    lastRegistereTimestamp = curTimestamp;
                }
            }
        }
        // 心跳机制 end
        eXosip_event_t* evtp = eXosip_event_wait(mSipCtx, 0, 100);
        if (!evtp) {
            eXosip_automatic_action(mSipCtx);
            osip_usleep(100*1000);
            continue;
        }
        eXosip_automatic_action(mSipCtx);
        sip_event_handle(evtp);
        eXosip_event_free(evtp); // 释放
    }
    eXosip_quit(mSipCtx);
exit:
    sipConfig.g_client.quit=true;
    pthread_exit((void *)0);
}

// 修改
int SipClient::SipClientExit()
{
    LOGI("gb28181 client exit start");
    LOGI("Sip Client run status: %s",sipConfig.g_client.quit==true?"NoWORK":"OnWORK");
    // LOGI("Sip Client thread pid: %lu",g_client.pid);
    if(!sipConfig.g_client.quit || sipConfig.g_client.threads.joinable()) {
        for(int i = 0; i <SipConfig::g_max_channels; i++)
            if(!sipConfig.g_rtpsendPs[i].mQuit) {
                rtpsendPs_stop(i);
            }
        sipConfig.g_client.quit = true;
        sipConfig.g_client.threads.join();
        if(sipConfig.g_sipinfo.channelId) {
            for (int i = 0; i < SipConfig::g_max_channels; i++) {
                free(sipConfig.g_sipinfo.channelId[i]);
                 sipConfig.g_sipinfo.channelId[i] = NULL;
                
            }
            // 释放指针数组本身
            free(sipConfig.g_sipinfo.channelId);
            sipConfig.g_sipinfo.channelId = NULL;
        }
        if(sipConfig.g_sipinfo.channelName) {
            for(int i = 0; i <SipConfig::g_max_channels; i++)
            {
                free(sipConfig.g_sipinfo.channelName[i]);
                sipConfig.g_sipinfo.channelName[i] = NULL;
            }
            free(sipConfig.g_sipinfo.channelName);
            sipConfig.g_sipinfo.channelName = NULL;
        }
        sipConfig.rtp_array_destroy(&sipConfig.g_rtpsendPs);
    }
    LOGD("gb28181 client exit end");
    return 0;
}

// 修改
int SipClient::init_sip_client()
{

     int ret = 1;
    mSipCtx = eXosip_malloc();
    if (!mSipCtx) {
        LOGE("new uas context error");
        return -1;
    }
    if (eXosip_init(mSipCtx)) {
        LOGE("exosip init error");
        return -1;
    }
    //初始化传输层协议
    // ret =eXosip_listen_addr(mSipCtx, IPPROTO_TCP, NULL, 5060, AF_INET, 0);  // TCP
    // ret =eXosip_listen_addr(mSipCtx, IPPROTO_UDP, NULL, 5060, AF_INET, 0);  // UDP
    // ret =eXosip_listen_addr(mSipCtx, IPPROTO_TCP, NULL, 5060, AF_INET, 1);  // TLS
    // ret =eXosip_listen_addr(mSipCtx, IPPROTO_UDP, NULL, 5060, AF_INET, 1);  // DTLS
    if(sipConfig.g_sipinfo.sipProtocol == GB28181_TRANS_TYPE_UDP)
        ret = eXosip_listen_addr(mSipCtx,IPPROTO_UDP, NULL, sipConfig.g_sipinfo.sipLoaclPort, AF_INET, 0);
    else if(sipConfig.g_sipinfo.sipProtocol == GB28181_TRANS_TYPE_TCP)
        ret =eXosip_listen_addr(mSipCtx, IPPROTO_TCP, NULL, sipConfig.g_sipinfo.sipLoaclPort, AF_INET, 0);
    if(ret) {
        LOGE("listen error\n");
        eXosip_quit(mSipCtx);
        return -1;
    }

	//ret = eXosip_set_option(mSipCtx,EXOSIP_OPT_UDP_KEEP_ALIVE,&ret);
    //user agent
    eXosip_set_user_agent(mSipCtx, sipConfig.g_sipinfo.userAgent);
    //LOGI("username =%s,userid=%s,passwd=%s,realm =%s\n",sipConfig.g_sipinfo.username, sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.passwd, sipConfig.g_sipinfo.realm);
    if (eXosip_add_authentication_info(mSipCtx, sipConfig.g_sipinfo.username, sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.passwd, NULL, sipConfig.g_sipinfo.realm) < 0) {
        LOGE("eXosip_add_authentication_info error");
        return -1;
    }
    LOGI("eXosip_listen_addr port %d success!\n",sipConfig.g_sipinfo.sipLoaclPort);
    return 0;

}

/*****************************************************************************
//	描述：	从xml数据中获取指定数据 修改
//data xml数据
//s_mark 开始标记 用于定位内容的开始位置
//with_s_make true则不包括s_mark的长度 false则包括s_mark的长度
//e_mark 结束标记 用于定位内容的结束位置
//evtp 事件结构体
*****************************************************************************/
int SipClient::parse_xml(const char *data, const char *s_mark, bool with_s_make, const char *e_mark, bool with_e_make, char *dest)
{
    const char* satrt = strstr(data, s_mark);

    if (satrt != NULL) {
        const char* end = strstr(satrt, e_mark);
        if (end != NULL) {
            int s_pos = with_s_make ? 0 : strlen(s_mark);
            int e_pos = with_e_make ? strlen(e_mark) : 0;

            strncpy(dest, satrt + s_pos, (end + e_pos) - (satrt + s_pos));
        }
        return 0;
    }
    return -1;
}


/*****************************************************************************
//	描述：	开始注册gb28181用户 修改
*****************************************************************************/
int SipClient::request_register()
{
      int ret = -1;
    osip_message_t *msg = NULL;
    char from[1024] = {0};
    char contact[1024] = {0};
    char proxy[1024] = {0};

    if (sipConfig.g_client.mRegistered) { // refresh register
        LOGI("refresh register mRegisterId=%d", sipConfig.g_client.mRegisterId);
        ret = eXosip_register_build_register(mSipCtx, sipConfig.g_client.mRegisterId, sipConfig.g_sipinfo.regExpires, &msg);
        if (!ret) {
            LOGE("eXosip_register_build_register error: ret=%d", ret);
            return -1;
        }
    } else { // new register
        LOGI("unregistered mRegisterId=%d", sipConfig.g_client.mRegisterId);
        if(eXosip_guess_localip(mSipCtx,AF_INET,sipConfig.g_sipinfo.deviceIp,sizeof(sipConfig.g_sipinfo.deviceIp)) != 0)
            strcpy(sipConfig.g_sipinfo.deviceIp,"192.168.1.1");//set to default ip
        sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
        sprintf(proxy, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);
        sprintf(contact, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
        LOGI("from = %s\n",from);

        sipConfig.g_client.mRegisterId = eXosip_register_build_initial_register(mSipCtx, from, proxy, contact, sipConfig.g_sipinfo.regExpires, &msg);
        if (sipConfig.g_client.mRegisterId <= 0) {
            LOGE("register info:\nfrom %s \nproxy %s\ncontact %s\n", from,proxy,contact);
            LOGE("eXosip_register_build_initial_register error: mRegisterId=%d", sipConfig.g_client.mRegisterId);
            return -1;
        }
    }
    eXosip_lock(mSipCtx);
    ret = eXosip_register_send_register(mSipCtx, sipConfig.g_client.mRegisterId, msg);
    eXosip_unlock(mSipCtx);
    if (ret) {
        LOGE("eXosip_register_send_register error: ret=%d", ret);
        return ret;
    }
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send request registration message:\n%s", s);
    osip_free(s);
    return ret;
}

// 修改
int SipClient::request_message_keepalive()
{
    char from[1024] = {0};
    char to[1024] = {0};

    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(to, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);

    osip_message_t* msg;
    char body[1024] = { 0 };
    uint32_t cseq = sipConfig.get_sip_command_sequence_number();

     sprintf(
        body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Notify>\r\n"
        "<CmdType>Keepalive</CmdType>\r\n"
        "<SN>%d</SN>\r\n"
        "<DeviceID>%s</DeviceID>\r\n"
        "<Status>OK</Status>\r\n"
        "</Notify>\r\n",
        cseq,sipConfig.g_sipinfo.userid);

    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send request keepalive message:\n%s", s);
    osip_free(s);
    return 0;
}

// 修改
int SipClient::request_media_status(int chn)
{
     char from[1024] = {0};
    char to[1024] = {0};
    char call_id[64] = {0};

    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(to, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);

    osip_message_t* msg;
    char body[1024] = {0};
    uint32_t cseq = sipConfig.get_sip_command_sequence_number();

    sprintf(
        body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Notify>\r\n"
        "<CmdType>MediaStatus</CmdType>\r\n"
        "<SN>%d</SN>\r\n"
        "<DeviceID>%s</DeviceID>\r\n"
        "<NotifyType>121</NotifyType>\r\n"
        "</Notify>\r\n",
        cseq,sipConfig.g_sipinfo.channelId[chn]);
    sprintf(call_id,"%s@%s",sipConfig.g_rtpsendPs[chn].callidNumber,sipConfig.g_rtpsendPs[chn].callidHost);
    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    msg->call_id = NULL,
    osip_message_set_call_id(msg,call_id);
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);

    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send request media status message:\n%s", s);
    osip_free(s);
    return 0;
}

/*****************************************************************************
//	描述：创建数据通道线程 修改
//	输入参数：
//args 序号
*****************************************************************************/
void *SipClient::response_message_catalog_thread(void *args)
{
     int i;
    int *seq = (int *)args;
    char from[1024] = {0};
    char to[1024] = {0};
    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(to, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);

    osip_message_t* msg;
    char body[1024] = {0};

    for(i=0;i<get_max_channels();i++) {
        sprintf(
            body,
            "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
            "<Response>\r\n"
                "<CmdType>Catalog</CmdType>\r\n"
                "<SN>%d</SN>\r\n"
                "<DeviceID>%s</DeviceID>\r\n"
                "<SumNum>%d</SumNum>\r\n"
                "<DeviceList Num=\"1\">\r\n"
                    "<Item>\r\n"
                        "<DeviceID>%s</DeviceID>\r\n"
                        "<Name>%s</Name>\r\n"
                        "<Manufacturer>%s</Manufacturer>\r\n"
                        "<Model>%s</Model>\r\n"
                        "<Owner>Owner</Owner>\r\n"
                        "<CivilCode>CivilCode</CivilCode>\r\n"//CivilCode 行政区域的划分编码
                        "<Address>Address</Address>\r\n"
                        "<Parental>0</Parental>\r\n"
                        "<ParentID>%s</ParentID>\r\n"
                        "<SafetyWay>0</SafetyWay>\r\n"
                        "<RegisterWay>1</RegisterWay>\r\n"
                        "<Secrecy>0</Secrecy>\r\n"
                        "<Status>%s</Status>\r\n"
                    "</Item>\r\n"
                "</DeviceList>\r\n"
            "</Response>\r\n",
            *seq,sipConfig.g_sipinfo.userid,get_max_channels(),sipConfig.g_sipinfo.channelId[i], sipConfig.g_sipinfo.channelName[i], \
            sipConfig.g_sipinfo.manufacturer, sipConfig.g_sipinfo.deviceModel, sipConfig.g_sipinfo.userid, sipConfig.g_devsta.status_on);

        eXosip_lock(mSipCtx);
        eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
        osip_message_set_body(msg, body, strlen(body));
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(mSipCtx, msg);
        eXosip_unlock(mSipCtx);
        char *s;
        size_t len;
        osip_message_to_str(msg, &s, &len);
        LOGI("send response catalog message:\n%s", s);
        osip_free(s);
        // usleep(100*1000);
    }
    free(seq);
    pthread_exit((void *)0);
}


/*****************************************************************************
//	描述：创建线程来实现创建通道 修改
*****************************************************************************/
int SipClient::response_message_catalog(eXosip_event_t *evtp, int *sn)
{
    std::thread t([this, sn]() {
        response_message_catalog_thread(sn);
    });
    t.detach();
    return 0;
}

// 修改
int SipClient::response_message_deviceinfo(eXosip_event_t *evtp, int *sn)
{
 int *seq = sn;
    char from[1024] = {0};
    char to[1024] = {0};

    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(to, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);

    osip_message_t* msg;
    char body[1024] = {0};

    sprintf(
        body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Response>\r\n"
            "<CmdType>DeviceInfo</CmdType>\r\n"
            "<SN>%d</SN>\r\n"
            "<DeviceID>%s</DeviceID>\r\n"
            "<DeviceName>%s</DeviceName>\r\n"
            "<Result>OK</Result>\r\n"
            "<DeviceType>IPC</DeviceType>\r\n"
            "<Manufacturer>%s</Manufacturer>\r\n"
            "<Model>%s</Model>\r\n"
            "<Firmware>%s</Firmware>\r\n"
            "<MaxCamera>1</MaxCamera>\r\n"
            "<MaxAlarm>1</MaxAlarm>\r\n"
        "</Response>\r\n",
        *seq,sipConfig.g_sipinfo.userid,sipConfig.g_sipinfo.deviceName,sipConfig.g_sipinfo.manufacturer, \
        sipConfig.g_sipinfo.deviceModel,sipConfig.g_sipinfo.deviceFirmware );
    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send response device info message:\n%s", s);
    osip_free(s);
    free(seq);
    return 0;
}

// 修改
void SipClient::dump_request(eXosip_event_t *evt)
{
    char* s;
    size_t len;
    osip_message_to_str(evt->request, &s, &len);
    LOGI("\nPrint request package begins\ntype=%d\n%s\nPrint request package completed\n", evt->type, s);
    osip_free(s);
}

// 修改
void SipClient::dump_response(eXosip_event_t *evt)
{
    char* s;
    size_t len;
    osip_message_to_str(evt->response, &s, &len);
    LOGI("\n打印响应包开始\ntype=%d\n%s\n打印响应包结束\n", evt->type, s);
    osip_free(s);
}

// 修改
int SipClient::response_message_answer(eXosip_event_t *evtp, int code)
{
    int returnCode = 0;
    osip_message_t *pRegister = NULL;
    returnCode = eXosip_message_build_answer(mSipCtx, evtp->tid, code, &pRegister);
    bool bRegister = false;
    if (pRegister) {
        bRegister = true;
    }
    if (returnCode == 0 && bRegister) {
        eXosip_lock(mSipCtx);
        eXosip_message_send_answer(mSipCtx, evtp->tid, code, pRegister);
        eXosip_unlock(mSipCtx);
    } else {
        LOGE("code=%d,returnCode=%d,bRegister=%d", code, returnCode, bRegister);
    }
    return 0;
}

//修改
int SipClient::response_playback_control(eXosip_event_t *evtp)
{
    char* username = evtp->request->to->url->username;
    char* CallID = evtp->request->call_id->number;
    char* Sip_method = evtp->request->sip_method;
    LOGI("Sip_Method:%s", Sip_method);
    LOGI("username:%s", username);
    LOGI("CallID:%s", CallID);

    int channel, Csq;
    char Method[0x20],PauseTime[0x20],Range[0x20];
    float Scale = 0;
    osip_body_t* req_body = NULL;
    char *token = NULL;
    channel = sipConfig.FindChannelByDeviceId(username);
    if(channel < 0)
    {
        LOGE("channel list not find the device id %s\n", username);
        return -1;
    }
    if(!strncmp(Sip_method, "BYE", strlen("BYE")))
    {
        return 0;
    }
    osip_message_get_body(evtp->request, 0, &req_body);
    if(req_body !=NULL)
    {
        token = strtok(req_body->body,"\n");
    }
    while(token != NULL)
    {
        // example: PAUSE MANSRTSP/1.0  PLAY MANSRTSP/1.0   PLAY RTSP/1.0
        if (strstr(token, "RTSP")!=NULL) {
            if (sscanf(token, "%[^ ]",  (char *)&Method) != 1) {
                LOGE("parse line error:%s", token);
                return 0;
            }
            else {
                LOGI("Method:%s", Method);
            }
        }
        // example: CSeq: 496
        if (!strncmp(token, "CSeq", strlen("CSeq"))) {
            if (sscanf(token, "CSeq: %d",  &Csq) != 1) {
                LOGE("parse line error:%s", token);
                return 0;
            }
            else {
                LOGI("CSeq:%d", Csq);
            }
        }
        // example: PauseTime: 0
        if (!strncmp(token, "PauseTime", strlen("PauseTime"))) {
            if (sscanf(token, "PauseTime: %s",  &PauseTime) != 1) {
                LOGE("parse line error:%s", token);
                return 0;
            }
            else {
                LOGI("PauseTime:%s", PauseTime);
            }
        }
        // example: Range: npt=now-
        if (!strncmp(token, "Range", strlen("Range"))) {
            if (sscanf(token, "Range: %s",  (char *)&Range) != 1) {
                LOGE("parse line error:%s", token);
                return 0;
            }
            else {
                LOGI("Range:%s", Range);
            }
        }
        // example: Scale: 1.0
        if (!strncmp(token, "Scale", strlen("Scale"))) {
            if (sscanf(token, "Scale: %f",  &Scale) != 1) {
                LOGE("parse line error:%s", token);
                return 0;
            }
            else {
                LOGI("Scale:%f", Scale);
            }
        }
        token = strtok(NULL, "\n");
    }
    if (!strncmp(Method, "PAUSE", strlen("PAUSE"))) {
        sipConfig.g_rtpsendPs[channel].mPlay = false;
    }
    else if (!strncmp(Method, "PLAY", strlen("PLAY"))) {
        if(Scale > 0)
            sipConfig.g_rtpsendPs[channel].scale = Scale;
        sipConfig.g_rtpsendPs[channel].mPlay = true;
    }
    osip_message_t* answer = NULL;
    eXosip_lock(mSipCtx);
    int ret = eXosip_call_build_answer(mSipCtx, evtp->tid, 200, &answer);
    if (ret != 0) {
        eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
        LOGE("camera: %s eXosip_call_build_answer error", username);
    }
    eXosip_call_send_answer(mSipCtx, evtp->tid, 200, answer);
    eXosip_unlock(mSipCtx);
    return 0;
}

struct RecordInfo {
    int serialNo;
	int channel;
    time_t startTime;//seconds timestamp
    time_t endTime;//seconds timestamp
};

//修改
void *SipClient::response_message_recordinfo_thread(void *args)
{
    struct RecordInfo *recordInfo = (struct RecordInfo *)args;
    char from[1024] = {0};
    char to[1024] = {0};
    char body[1024] = {0};
    char dirpath[128] = {0};
    char filepath[128] = {0};
    char videoStartTime[32] = {0};
    char videoEndTime[32] = {0};

    int serialNo = recordInfo->serialNo;
    int channel = recordInfo->channel;
    time_t filterStartTime = recordInfo->startTime; 
    time_t filterEndTime = recordInfo->endTime; 

    int total = 0;
    osip_message_t* msg;

    DIR *dir = NULL;
    char *ext = NULL;
    struct dirent *entry;
    time_t start_time, end_time, duration;

    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(to, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);
    if(sipConfig.FindVideoByTime(sipConfig.g_rtpsendPs[channel].recordDir, channel,filterStartTime, filterEndTime,&total) < 0)
        total = 0;
    if(total == 0) {
        sprintf(
            body,
            "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
            "<Response>\r\n"
                "<CmdType>RecordInfo</CmdType>\r\n"
                "<SN>%d</SN>\r\n"
                "<DeviceID>%s</DeviceID>\r\n"
                "<Name>Camera</Name>\r\n"
                "<SumNum>%d</SumNum>\r\n"
            "</Response>\r\n",
            serialNo,sipConfig.g_sipinfo.userid, total);
        eXosip_lock(mSipCtx);
        eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
        osip_message_set_body(msg, body, strlen(body));
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(mSipCtx, msg);
        eXosip_unlock(mSipCtx);
        // char *s;
        // size_t len;
        // osip_message_to_str(msg, &s, &len);
        // LOGI("send response recordinfo message:\n%s", s);
        // osip_free(s);
        goto ERROR_EXIT;
    }
    sprintf(dirpath, "%s", sipConfig.g_rtpsendPs[channel].recordDir);
    if (!(dir = opendir(dirpath))) {
        fprintf(stderr, "Cannot open directory: %s\n", dirpath);
        goto ERROR_EXIT;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		ext = strrchr(entry->d_name, '.');
		if (ext == NULL || ((strcmp(ext, ".h264") != 0 && strcmp(ext, ".h265") != 0))) {
            continue;
        }
        sprintf(filepath, "%s/%s", dirpath, entry->d_name);
        if(sipConfig.GetStreamDuration(filepath,&start_time,&end_time,&duration) < 0) {
            LOGE("parse stream duration err\tduration: %s\n",filepath);
            continue;
        }
        if (start_time < filterStartTime || start_time >= filterEndTime) {
            continue;
        }
        LOGD("File: %s starttimestamp:%lld <= filetimestamp:%lld <= endtimestamp:%lld", \
                        entry->d_name,(long long)filterStartTime,(long long)start_time,(long long)filterEndTime);
        sipConfig.ParseTimestampToTimestr(start_time,RecordInfoTimeFormat,videoStartTime,sizeof(videoStartTime));
        LOGD("video start time = %s timestamp = %lld",videoStartTime,(long long)start_time);
        sipConfig.ParseTimestampToTimestr(end_time,RecordInfoTimeFormat,videoEndTime,sizeof(videoEndTime));
        LOGD("video end time = %s timestamp = %lld",videoEndTime,(long long)end_time);
        sprintf(
            body,
            "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
            "<Response>\r\n"
                "<CmdType>RecordInfo</CmdType>\r\n"
                "<SN>%d</SN>\r\n"
                "<DeviceID>%s</DeviceID>\r\n"
                "<Name>Camera</Name>\r\n"
                "<SumNum>%d</SumNum>\r\n"
                "<RecordList Num=\"1\">\r\n"
                    "<Item>\r\n"
                        "<DeviceID>%s</DeviceID>\r\n"
                        "<Name>%s</Name>\r\n"
                        "<FilePath/>\r\n"
                        "<Address/>\r\n"
                        "<StartTime>%s</StartTime>\r\n"
                        "<EndTime>%s</EndTime>\r\n"
                        "<Secrecy>0</Secrecy>\r\n"
                        "<Type>time</Type>\r\n"
                        "<RecorderID/>\r\n"
                    "</Item>\r\n"
                "</RecordList>\r\n"
            "</Response>\r\n",
            serialNo,sipConfig.g_sipinfo.userid, total, sipConfig.g_sipinfo.channelId[channel], entry->d_name, videoStartTime, videoEndTime);
        eXosip_lock(mSipCtx);
        eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
        osip_message_set_body(msg, body, strlen(body));
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(mSipCtx, msg);
        eXosip_unlock(mSipCtx);
        // char *s;
        // size_t len;
        // osip_message_to_str(msg, &s, &len);
        // LOGI("send response recordinfo message:\n%s", s);
        // osip_free(s);
    }
    closedir(dir);
ERROR_EXIT:
    free(recordInfo);
    pthread_exit((void *)0);
}

//修改
int SipClient::response_message_recordinfo(eXosip_event_t *evtp, int *sn)
{
    osip_body_t *body = NULL;
    char EndTime[32] = {0};
    char DeviceID[64] = {0};
    char StartTime[32] = {0};
    osip_message_get_body(evtp->request, 0, &body);
    if(body == NULL)
        return -1;
    parse_xml(body->body, "<EndTime>", false, "</EndTime>", false, EndTime);
    parse_xml(body->body, "<DeviceID>", false, "</DeviceID>", false, DeviceID);
    parse_xml(body->body, "<StartTime>", false, "</StartTime>", false, StartTime);
    
    struct RecordInfo *recordInfo = (struct RecordInfo*)malloc(sizeof(struct RecordInfo));
    recordInfo->serialNo = *sn;
    recordInfo->channel = sipConfig.FindChannelByDeviceId(DeviceID);
    LOGD("StartTime:%s\tEndTime:%s",StartTime, EndTime);
    if(recordInfo->channel < 0) {
        LOGE("channel list not find the device id %s\n", DeviceID);
        return -1;
    }
    if(sipConfig.ParseTimstrToTimestamp(StartTime,RecordInfoTimeFormat,&recordInfo->startTime) < 0) {
        LOGE("parse time err\n");
        return -1;
    }
    LOGD("start timestamp = %lld",(long long)recordInfo->startTime);
    if(sipConfig.ParseTimstrToTimestamp(EndTime,RecordInfoTimeFormat,&recordInfo->endTime) < 0) {
        LOGE("parse time err\n");
        return -1;
    }
    LOGD("end timestamp = %lld",(long long)recordInfo->endTime);
    // pthread_t pid; 
    // int ret = pthread_create(&pid, NULL, response_message_recordinfo_thread, (void *)&recordInfo);
    // if (ret != 0) {
    //     LOGE("sd card check status pthread create failed\n");
    //     return -1;
    // }
    // pthread_detach(pid);
     std::thread t([this, &recordInfo]() {
        response_message_recordinfo_thread(&recordInfo);
    });
    t.detach();
    return 0;
}

//修改
int SipClient::response_message_devicestatus(eXosip_event_t *evtp, int *sn)
{
     int *seq = sn;
    char from[1024] = {0};
    char to[1024] = {0};

    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(to, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);

    osip_message_t* msg;
    char body[1024] = {0};
    time_t now = time(NULL);
    sipConfig.ParseTimestampToTimestr(now,RecordInfoTimeFormat,sipConfig.g_devsta.status_time,sizeof(sipConfig.g_devsta.status_time));
    sprintf(
        body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Response>\r\n"
            "<CmdType>DeviceStatus</CmdType>\r\n"
            "<SN>%d</SN>\r\n"
            "<DeviceID>%s</DeviceID>\r\n"
            "<Result>OK</Result>\r\n"
            "<Online>%s</Online>\r\n"
            "<Status>%s</Status>\r\n"
            "<Encode>%s</Encode>\r\n"
            "<Record>%s</Record>\r\n"
            "<DeviceTime>%s</DeviceTime>\r\n"
            "<Alarmstatus Num=\"0\"/>\r\n"
        "</Response>\r\n",
        *seq,sipConfig.g_sipinfo.userid,sipConfig.g_devsta.status_online,sipConfig.g_devsta.status_ok,sipConfig.g_sipinfo.deviceEncode, \
        sipConfig.g_sipinfo.deviceRecord,sipConfig.g_devsta.status_time);
    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send response device status message:\n%s", s);
    osip_free(s);
    free(seq);
    return 0;
}

//修改
int SipClient::response_message_devicecontrol(eXosip_event_t *evtp, int *sn)
{
     osip_body_t *req_body = NULL;
    int *seq = sn;
    osip_message_get_body(evtp->request, 0, &req_body);
    if(req_body == NULL)
        return -1;
    printf("[%s] %s\n", __FUNCTION__, req_body->body);
    char DeviceID[64] = {0};
    char ControlCmd[32] = {0};
    char ControlType[32] = {0};
    parse_xml(req_body->body, "<DeviceID>", false, "</DeviceID>", false, DeviceID);
    if (parse_xml(req_body->body, "<RecordCmd>", false, "</RecordCmd>", false, ControlCmd) == 0) {
        strcpy(ControlType, "Record");
    }
    else if (parse_xml(req_body->body, "<IFameCmd>", false, "</IFameCmd>", false, ControlCmd) == 0){
        strcpy(ControlType, "IFrame");
    }
    else if (parse_xml(req_body->body, "<PTZCmd>", false, "</PTZCmd>", false, ControlCmd) == 0) {
        strcpy(ControlType, "PTZ");
    } else if (parse_xml(req_body->body, "<GuardCmd>", false, "</GuardCmd>", false, ControlCmd) == 0) {
        strcpy(ControlType, "Guard");
    } else if(parse_xml(req_body->body, "<TeleBoot>", false, "</TeleBoot>", false, ControlCmd) == 0) {
        strcpy(ControlType, "Boot");
    } else {
        LOGE("unknow control cmd\n");
        return -1;
    }
    LOGI("ControlType:%s,ControlCmd:%s", ControlType, ControlCmd);
    int result = 1;
    if (strcmp(ControlType, "Record") == 0) {
        // 录像控制
        if (strcmp(ControlCmd, "Record") == 0) {
            // result = start_recording(channel);
        } else if (strcmp(ControlCmd, "StopRecord") == 0) {
            // result = stop_recording(channel);
        }
    } 
    else if (strcmp(ControlType, "IFrame") == 0) {
        // I帧请求
        if (strcmp(ControlCmd, "Send") == 0) {
            // result = send_iframe(channel);
        }
    }
    else if (strcmp(ControlType, "PTZ") == 0) {
        // PTZ控制(需额外解析PTZCmd)
        // result = execute_ptz_command(channel, ControlCmd);
    }
    else if (strcmp(ControlType, "Guard") == 0) {
        // 布防控制
        if (strcmp(ControlCmd, "SetGuard") == 0) {
            // result = set_guard(channel, 1);
        } else if (strcmp(ControlCmd, "ResetGuard") == 0) {
            // result = set_guard(channel, 0);
        }
    } else if (strcmp(ControlType, "Boot") == 0) {
        if (strcmp(ControlCmd, "Boot") == 0) {
            result = request_unregister();
        }
    }
    char from[1024] = {0};
    char to[1024] = {0};
    osip_message_t* msg = NULL;
    char body[1024] = {0};

    snprintf(from, sizeof(from), "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    snprintf(to, sizeof(to), "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);
    snprintf(body, sizeof(body),
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Response>\r\n"
        "<CmdType>DeviceControl</CmdType>\r\n"
        "<SN>%d</SN>\r\n"
        "<DeviceID>%s</DeviceID>\r\n"
        "<Result>%s</Result>\r\n"
        "</Response>\r\n",
        *seq,
        DeviceID,
        result ? "OK" : "ERROR");
    eXosip_lock(mSipCtx);
    int ret = eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    if (ret != 0 || !msg) {
        LOGE("Build message failed: %d", ret);
        eXosip_unlock(mSipCtx);
        return -1;
    }
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    free(seq);
    return 0;
}

//修改
int SipClient::response_message_configdownload(eXosip_event_t *evtp, int *sn)
{
     osip_body_t *req_body = NULL;
    int *seq = sn;
    osip_message_get_body(evtp->request, 0, &req_body);
    if(req_body == NULL)
        return -1;
    char DeviceID[64] = {0};
    char ConfigType[64] = {0};
    parse_xml(req_body->body, "<DeviceID>", false, "</DeviceID>", false, DeviceID);
    parse_xml(req_body->body, "<ConfigType>", false, "</ConfigType>", false, ConfigType);
    char from[1024] = {0};
    char to[1024] = {0};
    osip_message_t* msg = NULL;
    char body[2048] = {0};
    
    snprintf(from, sizeof(from), "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    snprintf(to, sizeof(to), "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);

    // 根据配置类型生成不同的响应体
    if (strcmp(ConfigType, "BasicParam") == 0) {
        snprintf(body, sizeof(body),
            "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
            "<Response>\r\n"
            "<CmdType>ConfigDownload</CmdType>\r\n"
            "<SN>%d</SN>\r\n"
            "<DeviceID>%s</DeviceID>\r\n"
            "<Result>OK</Result>\r\n"
            "<BasicParam>\r\n"
            "<Name>%s</Name>\r\n"
            "<Expiration>%d</Expiration>\r\n"
            "<HeartBeatInterval>%d</HeartBeatInterval>\r\n"
            "<HeartBeatCount>%d</HeartBeatCount>\r\n"
            "</BasicParam>\r\n"
            "</Response>\r\n",
            *seq,
            DeviceID,
            sipConfig.g_sipinfo.deviceName,
            sipConfig.g_sipinfo.regExpires,
            sipConfig.g_sipinfo.heartBeatInterval,
            sipConfig.g_sipinfo.heartBeatCount);
    } else if (strcmp(ConfigType, "VideoParamOpt") == 0) {
        snprintf(body, sizeof(body),
            "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
            "<Response>\r\n"
            "<CmdType>ConfigDownload</CmdType>\r\n"
            "<SN>%d</SN>\r\n"
            "<DeviceID>%s</DeviceID>\r\n"
            "<Result>OK</Result>\r\n"
            "<VideoParamOpt>\r\n"
            "<DownloadSpeed>%.2f</DownloadSpeed>\r\n"
            "<Resolution>%s</Resolution>\r\n"
            "</VideoParamOpt>\r\n"
            "</Response>\r\n",
            *seq,
            DeviceID,
            sipConfig.g_sipinfo.loadSpeed,
            sipConfig.g_sipinfo.resolution);
    } else {
        LOGE("Unsupported config type: %s", ConfigType);
        return -1;
    }

    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send response configdownload message:\n%s", s);
    osip_free(s);
    free(seq);
    return 0;
}

int SipClient::request_unregister()
{
    int ret = -1;
    osip_message_t *msg = NULL;
    char from[1024] = {0};
    char contact[1024] = {0};
    char proxy[1024] = {0};

    sipConfig.g_sipinfo.regExpires = 0;
    if (sipConfig.g_client.mRegistered) { // remove register
        LOGI("registered mRegisterId=%d", sipConfig.g_client.mRegisterId);
        eXosip_register_remove(mSipCtx,sipConfig.g_client.mRegisterId);
        LOGD("remove register");
    }
    sipConfig.g_client.mRegistered = false;
    // new register
    LOGI("unregistered mRegisterId=%d", sipConfig.g_client.mRegisterId);
    if(eXosip_guess_localip(mSipCtx,AF_INET,sipConfig.g_sipinfo.deviceIp,sizeof(sipConfig.g_sipinfo.deviceIp)) != 0)
        strcpy(sipConfig.g_sipinfo.deviceIp,"192.168.1.1");//set to default ip
    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(proxy, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);
    sprintf(contact, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sipConfig.g_client.mRegisterId = eXosip_register_build_initial_register(mSipCtx, from, proxy, contact, sipConfig.g_sipinfo.regExpires, &msg);
    if (sipConfig.g_client.mRegisterId <= 0) {
        LOGE("register info:\nfrom %s \nproxy %s\ncontact %s", from,proxy,contact);
        LOGE("eXosip_register_build_initial_register error: mRegisterId=%d", sipConfig.g_client.mRegisterId);
        return -1;
    }
    eXosip_lock(mSipCtx);
    ret = eXosip_register_send_register(mSipCtx, sipConfig.g_client.mRegisterId, msg);
    eXosip_unlock(mSipCtx);
    if (ret) {
        LOGE("eXosip_register_send_register error: ret=%d", ret);
        return ret;
    }
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send request unregistration message:\n%s", s);
    osip_free(s);
    return ret;
}

int SipClient::response_subscribe(eXosip_event_t *evtp)
{
    osip_body_t *body = NULL;
    osip_message_get_body(evtp->request, 0, &body);
    int ret;
    char SN[20] = {0}; // 序列号
    char CmdType[64] = {0};

    if(body) {
        parse_xml(body->body, "<SN>", false, "</SN>", false, SN);
        parse_xml(body->body, "<CmdType>", false, "</CmdType>", false, CmdType);
    } else {
        response_message_answer(evtp, 400);
        return -1;
    }
    int *serialNo = (int *)malloc(sizeof(int));
    if (serialNo == NULL) {
        response_message_answer(evtp, 400);
        return -1;
    }
    *serialNo = atoi(SN);
    LOGI("SN:%d", *serialNo);
    LOGI("CmdType:%s", CmdType);
    response_message_answer(evtp, 200);
    if (!strcmp(CmdType, "Alarm")) {
        ret = response_subscribe_alarm(evtp,serialNo);
    }
    if(ret < 0) {
        free(serialNo);
    }
    return ret;
}

int SipClient::response_subscribe_alarm(eXosip_event_t *evtp, int *sn)
{
    int *seq = sn;
    char from[1024] = {0};
    char to[1024] = {0};
    time_t startTime,endTime,alarmTime;
    char alarmTimeStr[32] = {0};
    osip_body_t *req_body = NULL;
    char EndTime[32] = {0};
    char DeviceID[64] = {0};
    char StartTime[32] = {0};
    char AlarmMethod[32] = {0};
    char EndAlarmPriority[32] = {0};
    char StartAlarmPriority[32] = {0};

    osip_message_get_body(evtp->request, 0, &req_body);
    if(req_body == NULL)
        return -1;
    parse_xml(req_body->body, "<EndTime>", false, "</EndTime>", false, EndTime);
    parse_xml(req_body->body, "<DeviceID>", false, "</DeviceID>", false, DeviceID);
    parse_xml(req_body->body, "<StartTime>", false, "</StartTime>", false, StartTime);
    parse_xml(req_body->body, "<AlarmMethod>", false, "</AlarmMethod>", false, AlarmMethod);
    parse_xml(req_body->body, "<EndAlarmPriority>", false, "</EndAlarmPriority>", false, EndAlarmPriority);
    parse_xml(req_body->body, "<StartAlarmPriority>", false, "</StartAlarmPriority>", false, StartAlarmPriority);

    if(sipConfig.ParseTimstrToTimestamp(StartTime, RecordInfoTimeFormat, &startTime) < 0) {
        LOGE("parse time err\n");
        return -1;
    }
    if(sipConfig.ParseTimstrToTimestamp(EndTime, RecordInfoTimeFormat, &endTime) < 0) {
        LOGE("parse time err\n");
        return -1;
    }
    alarmTime = startTime + (endTime - startTime) / 2;
    sipConfig.ParseTimestampToTimestr(alarmTime, RecordInfoTimeFormat, alarmTimeStr, sizeof(alarmTimeStr));
    sprintf(from, "sip:%s@%s:%d", sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.deviceIp, sipConfig.g_sipinfo.sipLoaclPort);
    sprintf(to, "sip:%s@%s:%d", sipConfig.g_sipinfo.serverId, sipConfig.g_sipinfo.serverIp, sipConfig.g_sipinfo.sipServerPort);
    osip_message_t* msg;
    char resp_body[1024] = {0};
    time_t now = time(NULL);
    sipConfig.ParseTimestampToTimestr(now,RecordInfoTimeFormat,sipConfig.g_devsta.status_time,sizeof(sipConfig.g_devsta.status_time));
    sprintf(
        resp_body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Notify>\r\n"
            "<CmdType>Alarm</CmdType>\r\n"
            "<SN>%d</SN>\r\n"
            "<DeviceID>%s</DeviceID>\r\n"
            "<AlarmPriority>4</AlarmPriority>\r\n"
            "<AlarmMethod>2</AlarmMethod>\r\n"
            "<AlarmTime>%s</AlarmTime>\r\n"
        "</Notify>\r\n",
        *seq,DeviceID,alarmTimeStr);
    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, resp_body, strlen(resp_body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send response subscribe alarm:\n%s", s);
    osip_free(s);
    free(seq);
    return 0;
}

void SipClient::cleanup_resources(void *arg)
{
    LOGI("开始清理资源\n");
    CleanupArg* cleanupArg = static_cast<CleanupArg*>(arg);
    SipClient* client = cleanupArg->client;  
    int channel = cleanupArg->channel;
    
    // 现在可以安全使用
    if (client->sipConfig.g_rtpsendPs[channel].mSockFd > 0) {
        close(client->sipConfig.g_rtpsendPs[channel].mSockFd);
        client->sipConfig.g_rtpsendPs[channel].mSockFd = 0;
    }
    
    if (client->sipConfig.g_rtpsendPs[channel].fp != NULL) {
        fclose(client->sipConfig.g_rtpsendPs[channel].fp);
        client->sipConfig.g_rtpsendPs[channel].fp = NULL;
    }
    
    if (client->sipConfig.g_rtpsendPs[channel].buffer != NULL) {
        free(client->sipConfig.g_rtpsendPs[channel].buffer);
        client->sipConfig.g_rtpsendPs[channel].buffer = NULL;
    }
    
    client->sipConfig.g_rtpsendPs[channel].mPlay = false;
    client->sipConfig.g_rtpsendPs[channel].mQuit = true;
    client->sipConfig.g_rtpsendPs[channel].mRun = false;
    LOGI("清理资源完成\n");
}

/*****************************************************************************
//	描述：	处理message 请求 //修改
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::response_message(eXosip_event_t *evtp)
{
    osip_body_t *body = NULL;
    osip_message_get_body(evtp->request, 0, &body);

    int ret;
    char SN[20] = {0}; // 序列号
    char CmdType[64] = {0};
    if(body) {
        parse_xml(body->body, "<SN>", false, "</SN>", false, SN);
        parse_xml(body->body, "<CmdType>", false, "</CmdType>", false, CmdType);
    } else {
        response_message_answer(evtp, 400);
        return -1;
    }
    int *serialNo = (int *)malloc(sizeof(int));
    if (serialNo == NULL) {
        response_message_answer(evtp, 400);
        return -1;
    }
    *serialNo = atoi(SN);
    LOGI("SN:%d", *serialNo);
    LOGI("CmdType:%s", CmdType);
    response_message_answer(evtp, 200);
    if (!strcmp(CmdType, "Catalog")) {
        ret = response_message_catalog(evtp,serialNo);
    } else if (!strcmp(CmdType, "DeviceInfo")) {
        ret = response_message_deviceinfo(evtp,serialNo);
    } else if(!strcmp(CmdType, "RecordInfo")) {
        ret = response_message_recordinfo(evtp,serialNo);
    } else if (!strcmp(CmdType, "DeviceStatus")) {
        ret = response_message_devicestatus(evtp,serialNo);
    } else if(!strcmp(CmdType, "Keepalive")){
        ;
    } else if (!strcmp(CmdType, "DeviceControl")) {
        ret = response_message_devicecontrol(evtp, serialNo);
    } else if (!strcmp(CmdType, "ConfigDownload")) {
        ret = response_message_configdownload(evtp, serialNo);//(serialNo, DeviceID, configType);
    } else if (!strcmp(CmdType, "Alarm")) {
        ret = 0;
        // ret = response_message_alarm(evtp, serialNo);
    } else {
        LOGE("Unknown CmdType");
        ret = -1;
    }
    if (ret < 0) {
        response_message_answer(evtp, 400);
        free(serialNo);
    }
    return 0;

}

/*****************************************************************************
//	描述：	处理呼叫请求 //修改
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::response_invite(eXosip_event_t *evtp)
{
    char *username = evtp->request->to->url->username;//对应摄像头的DeviceID
    char *CallID = evtp->request->call_id->number;
    char *Message = NULL;
    LOGI("username:%s", username);
    LOGI("CallID:%s", CallID);

    int ret;
    char *token = NULL;
    osip_message_t* answer = NULL;

    char ServerIP[0x10] = {0};
    Message = evtp->request->message;
    if(Message == NULL)
        Message = evtp->response->message;
    if(Message != NULL) {
        char *callid_line = strstr(Message, "Call-ID");
        if (callid_line) {
            char* at_pos = strchr(callid_line, '@');
            if (at_pos) {
                if(sscanf(at_pos + 1, "%15[^;]", ServerIP) == 1) {
                    if(!strncmp(ServerIP,"0.0.0.0",strlen("0.0.0.0"))) {
                        char *received_pos = strstr(Message, "received=");
                        if(received_pos)
                            sscanf(received_pos + 9, "%15[^;]", ServerIP);
                        else
                            memset(ServerIP,0,sizeof(ServerIP));
                    }
                }
            }
        }
    }
    eXosip_lock(mSipCtx);
    // Ringing
    eXosip_call_send_answer(mSipCtx, evtp->tid, 180, NULL);
    ret = eXosip_call_build_answer(mSipCtx, evtp->tid, 200, &answer);
    if (ret != 0) {
        eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
        LOGE("camera: %s eXosip_call_build_answer error", username);
    } else {
/*
        // 采用exosip的函数解析sdp
        //eXosip_get_remote_sdp 函数解析失败，不能使用库解析
        LOGD("-----------exosip parse start-----------\n");
        sdp_message_t *remote_sdp = eXosip_get_remote_sdp(mSipCtx, evtp->did);
        if(remote_sdp){
            sdp_media_t *video_sdp = eXosip_get_video_media(remote_sdp);
            if (video_sdp) {
                int pos = 0;
                char *video_port = video_sdp->m_port; // audio_port
                for (int i = 0; i < video_sdp->a_attributes.nb_elt; i++)
                {
                    sdp_attribute_t *attr = (sdp_attribute_t *)osip_list_get(&video_sdp->a_attributes, i);
                    LOGD("1-%s : %s\n", attr->a_att_field, attr->a_att_value);
                }
                while (!osip_list_eol(&(remote_sdp->a_attributes), pos)) {
                    sdp_attribute_t *at;
                    at = (sdp_attribute_t *)osip_list_get(&remote_sdp->a_attributes, pos);
                    LOGD(
                        "2-%s : %s\n", at->a_att_field,
                        at->a_att_value); // 这里解释了为什么在SDP消息体中属性a里面存放必须是两列
                    pos++;
                }
                while (!osip_list_eol(&(remote_sdp->m_medias), pos)) {
                    sdp_attribute_t *at;

                    at = (sdp_attribute_t *)osip_list_get(&remote_sdp->m_medias, pos);
                    LOGD(
                        "3-%s : %s\n", at->a_att_field,
                        at->a_att_value); // 这里解释了为什么在SDP消息体中属性a里面存放必须是两列

                    pos++;
                }
            }
        }
        LOGD("-----------exosip parse end-----------\n");
*/
        osip_body_t *req_body = NULL;
        osip_message_get_body(evtp->request, 0, &req_body);
        if(req_body != NULL)
            token = strtok(req_body->body,"\n");
        SdpTrack m_sdptrack;
        if(sipConfig.ParseSDPInfo(token, &m_sdptrack) < 0) {
            eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
            goto exit;
        }
        int chn = sipConfig.FindChannelByDeviceId(username);
        if(chn < 0) {
            LOGE("channel list not find the device id %s\n", username);
            eXosip_call_send_answer(mSipCtx, evtp->tid, 500, NULL);
            goto exit;
        }
        if(strcmp(ServerIP,m_sdptrack.address) != 0 && ServerIP[0] != '\0')
            strcpy(m_sdptrack.address,ServerIP);
        LOGI("ServerIP:%s", m_sdptrack.address);
        sipConfig.UpdateRtpSendCfgBySDPInfo(chn,m_sdptrack);
        int sdpLen = 0;
        char sdpBuf[1024];
        time_t now = time(NULL);
        if(sipConfig.g_rtpsendPs[chn].startTime == 0 && sipConfig.g_rtpsendPs[chn].endTime == 0) {
            sipConfig.g_rtpsendPs[chn].startTime = now;
            sipConfig.g_rtpsendPs[chn].endTime = now + 3600;
        }
        sdpLen += snprintf(
            sdpBuf, sizeof(sdpBuf),
            "v=0\r\n"
            "o=%s 0 0 IN IP4 %s\r\n"
            "s=%s\r\n"
            "c=IN IP4 %s\r\n"
            "t=%lld %lld\r\n"
            "m=video %d %s 96 98 97\r\n"
            "a=sendonly\r\n"
            "a=rtpmap:96 PS/90000\r\n"
            "a=rtpmap:98 H264/90000\r\n"
            "a=rtpmap:97 MPEG4/90000\r\n",
            sipConfig.g_sipinfo.channelId[chn], sipConfig.g_sipinfo.deviceIp,  sipConfig.g_rtpsendPs[chn].sessionName,
            sipConfig.g_sipinfo.deviceIp, (long long)sipConfig.g_rtpsendPs[chn].startTime, (long long)sipConfig.g_rtpsendPs[chn].endTime,
            sipConfig.g_rtpsendPs[chn].rtpLocalPort, sipConfig.g_rtpsendPs[chn].rtpProtocol);
        if(sipConfig.g_rtpsendPs[chn].hasAudio) {
            sdpLen += snprintf(sdpBuf + sdpLen, sizeof(sdpBuf) - sdpLen,
            "m=audio 8000 RTP/AVP 8\r\n"// 音频媒体行：端口8000，RTP/AVP协议，payload类型8
            "a=sendonly\r\n"            // 媒体属性：只发送
            "a=rtpmap:98 PCMA/8000\r\n");// 定义payload类型8为PCMA编码，采样率8000Hz
        }
        sdpLen += snprintf(sdpBuf + sdpLen, sizeof(sdpBuf) - sdpLen,
            "y=%010d\r\n",
            sipConfig.g_rtpsendPs[chn].u32Ssrc);
        LOGD("sdplen: %d\tsdpbuf:\n%s\n",sdpLen, sdpBuf);
        osip_message_set_body(answer, sdpBuf, sdpLen);
        osip_message_set_content_type(answer, "application/sdp");
        eXosip_call_send_answer(mSipCtx, evtp->tid, 200, answer);
    }
exit:
    eXosip_unlock(mSipCtx);
    return 0;
}

/*****************************************************************************
//	描述：	接收到ack请求 //修改
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::response_ack(eXosip_event_t *evtp)
{
    char* username = evtp->request->to->url->username;//对应摄像头的DeviceID
    char* CallID = evtp->request->call_id->number;
    LOGI("Start ps over rtp push stream");
    LOGD("username:%s\n", username);
    LOGD("CallID:%s\n", CallID);
    //获取对应通道
    int channel = sipConfig.FindChannelByDeviceId(username);
    if(channel < 0)
    {
        LOGE("channel list not find the device id %s\n", username);
        eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
        return -1;
    }
    strcpy(sipConfig.g_rtpsendPs[channel].callidNumber,evtp->request->call_id->number);
    strcpy(sipConfig.g_rtpsendPs[channel].callidHost,evtp->request->call_id->host);
    // 正式开始推流
    if(!sipConfig.g_rtpsendPs[channel].mQuit){
        rtpsendPs_stop(channel);
    }
    if(rtpsendPs_init(channel)!= 0)
    {
        LOGE("gb28181 socket init err\n");
        eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
        return -1;
    }
    if(rtpsendPs_start(channel) < 0)
    {
        eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
        return -1;
    }
    LOGD("rtpsendPs_start[%d]\n",channel);
    return 0;
}

/*****************************************************************************
//	描述：	接收到bye请求 //修改
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::response_bye(eXosip_event_t *evtp)
{
    char* username = evtp->request->to->url->username;//对应摄像头的DeviceID
    char* CallID = evtp->request->call_id->number;
    LOGI("Stop ps over rtp push stream");
    LOGI("username:%s", username);
    LOGI("CallID:%s", CallID);

    //获取对应通道
    int channel = sipConfig.FindChannelByDeviceId(username);
    if(channel < 0)
    {
        LOGE("channel list not find the device id %s\n", username);
        return -1;
    }
    triggerCallbacks(0x02,channel);

    // 停止推流
    if(!sipConfig.g_rtpsendPs[channel].mQuit){
        rtpsendPs_stop(channel);
    }
    return 0;
}

//修改
int SipClient::response_video_ctrl(eXosip_event_t *evtp)
{
    char* username = evtp->request->to->url->username;
    char* CallID = evtp->request->call_id->number;
    char* Sip_method = evtp->request->sip_method;
    LOGI("Sip_Method:%s", Sip_method);
    LOGI("username:%s", username);
    LOGI("CallID:%s", CallID);
    if(!strncmp(Sip_method, "BYE", strlen("BYE")))
    {
        return 0;
    }
    osip_message_t* answer = NULL;
    eXosip_lock(mSipCtx);
    int ret = eXosip_call_build_answer(mSipCtx, evtp->tid, 200, &answer);
    if (ret != 0) {
        eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
        LOGE("camera: %s eXosip_call_build_answer error", username);
    }
    eXosip_call_send_answer(mSipCtx, evtp->tid, 200, answer);
    eXosip_unlock(mSipCtx);
    return 0;
}


/*****************************************************************************
//	描述：	事件处理 //修改
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::sip_event_handle(eXosip_event_t *evtp)
{
   if(evtp->request) 
        printf("+++++++++++++++%s\n",evtp->request->sip_method);
    switch (evtp->type) {
        case EXOSIP_REGISTRATION_SUCCESS://0 //OK
            LOGI("Process Event EXOSIP_REGISTRATION_SUCCESS\tinfo: user is successfully registred");
            if(MSG_IS_REGISTER(evtp->request)) {
                sipConfig.g_client.mRegistered = true;
                sipConfig.g_client.errorCountReg = 0;
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            LOGI("mRegistered=%d,mRegisterId=%d", sipConfig.g_client.mRegistered, sipConfig.g_client.mRegisterId);
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_REGISTRATION_FAILURE://1 //OK
            LOGI("Process Event EXOSIP_REGISTRATION_FAILURE\tinfo: user is not registred");
            sipConfig.g_client.errorCountReg++;
            sipConfig.g_client.mRegistered = false;
            if(sipConfig.g_client.errorCountReg >= 5) {
                sipConfig.g_client.allowReg = false;
                LOGE("Registrations retach max failed count %d\n",sipConfig.g_client.errorCountReg);
                break;
            }
            if(MSG_IS_REGISTER(evtp->request)) {
                if(evtp->response) {
                    LOGD("register failed, return code %d, reason: %s",evtp->response->status_code,evtp->response->reason_phrase);
                    if(evtp->response->status_code == 401) {
                        if (eXosip_add_authentication_info(mSipCtx, sipConfig.g_sipinfo.username, sipConfig.g_sipinfo.userid, sipConfig.g_sipinfo.passwd, NULL, sipConfig.g_sipinfo.realm) < 0) {
                            LOGI("eXosip_add_authentication_info error");
                        } else {
                            LOGD("add authentication info");
                            // sipConfig.g_client.allowReg = true;
                            break;
                        }
                    }
                } else {
                    LOGE("couldn`t connetc sip, please check your config or network");
                }
                sipConfig.g_client.allowReg = true;
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            eXosip_register_remove(mSipCtx,sipConfig.g_client.mRegisterId);
            LOGI("mRegistered=%d,mRegisterId=%d", sipConfig.g_client.mRegistered, sipConfig.g_client.mRegisterId);
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_CALL_INVITE://2 //OK
            LOGI("Process Event EXOSIP_CALL_INVITE\tinfo: announce a new call");
            if(MSG_IS_INVITE(evtp->request)) {
                response_invite(evtp);
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_CALL_NOANSWER://4
            LOGI("Process Event EXOSIP_CALL_NOANSWER\tinfo: announce no answer within the timeout");
            break;

        case EXOSIP_CALL_ANSWERED://7 //OK
            LOGI("Process Event EXOSIP_CALL_ANSWERED\tinfo: announce start of call");
            response_invite_ack(evtp);
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_CALL_ACK://12 //OK
            LOGI("Process Event EXOSIP_CALL_ACK\tinfo: ACK received for 200ok to INVITE");
            if(MSG_IS_INVITE(evtp->request)) {
                response_ack(evtp);
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            // not to response ack
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_CALL_MESSAGE_NEW://14 //OK
            LOGI("Process Event EXOSIP_CALL_MESSAGE_NEW\tinfo: announce new incoming request");
            if (MSG_IS_BYE(evtp->request)) {
                // not to response ack
                response_bye(evtp);
            } else if(MSG_IS_INFO(evtp->request)) {
                response_playback_control(evtp);
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_CALL_CLOSED://21 //OK
            LOGI("Process Event EXOSIP_CALL_CLOSED\tinfo: a BYE was received for this call");
            if(MSG_IS_BYE(evtp->request)) {
                LOGD("recv bye, not resposne to message");
                // not to response ack
                // clearClientMap();
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_CALL_RELEASED://22
            LOGI("Process Event EXOSIP_CALL_RELEASED\tinfo: call context is cleared");
            break;

        case EXOSIP_MESSAGE_NEW://23 //OK
            LOGI("Process Event EXOSIP_MESSAGE_NEW\tinfo: announce new incoming request");
            if (MSG_IS_MESSAGE(evtp->request)) {
                response_message(evtp);
            } else if(MSG_IS_BYE(evtp->request)) {
                response_bye(evtp);
            } else {
                // need to response ack
                response_message_answer(evtp,200);
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            // dump_request(evtp);
            // dump_response(evtp);
            /*
            // 可能会出现的请求
            BYE sip:00662800000403000001@192.168.8.200:5060 SIP/2.0
            Via: SIP/2.0/UDP 192.168.8.114:5060;branch=z9hG4bK695c5ff8b5c014866ffc6a554c242a6d
            From: <sip:00662800000401000001@0066280000>;tag=185326220
            To: <sip:00662802002006028104@0066280000>;tag=2009556327
            Call-ID: 05a7fc88c30878338ff311a788e9cefa@192.168.8.114
            CSeq: 185 BYE
            Max-forwards: 70
            Content-Length: 0
            */
            break;

        case EXOSIP_MESSAGE_ANSWERED://25 //OK
            LOGI("Process Event EXOSIP_MESSAGE_ANSWERED\tinfo: announce a 200ok");
            if(MSG_IS_MESSAGE(evtp->request)) {
                LOGD("recv 200ok, not resposne to message");
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            // dump_request(evtp);
            // dump_response(evtp);
            break;

        case EXOSIP_MESSAGE_REQUESTFAILURE://27
            LOGI("Process Event EXOSIP_MESSAGE_REQUESTFAILURE\tinfo: announce a failure");
            LOGE("evtp->textinfo= '%s' ", evtp->textinfo);
            if (evtp->ack) {
                char* ack_str;
                size_t ack_str_len;
                osip_message_to_str(evtp->ack, &ack_str, &ack_str_len);
                LOGE("ack_str=%s", ack_str);
                osip_free(ack_str);
            }
            sipConfig.g_client.allowReg = true;
            sipConfig.g_client.mRegistered = false;
            dump_request(evtp);
            dump_response(evtp);
            break;

        case EXOSIP_IN_SUBSCRIPTION_NEW:
            LOGI("Process Event EXOSIP_IN_SUBSCRIPTION_NEW\tinfo: announce new incoming SUBSCRIBE/REFER");
            if(MSG_IS_SUBSCRIBE(evtp->request)) {
                response_subscribe(evtp);
            /*
            <?xml version="1.0"?> 
            <Query>
            <CmdType>Alarm</CmdType>
            <SN>17430</SN>
            <DeviceID>34020000001110000001</DeviceID>
            <StartAlarmPriority>1</StartAlarmPriority>
            <EndAlarmPriority>4</EndAlarmPriority>
            <AlarmMethod>0</AlarmMethod>
            <StartTime>2025-10-25T16:29:13</StartTime>
            <EndTime>2025-10-25T16:34:13</EndTime>
            </Query>
            */
            } else {
                LOGE("unknown sip method %s\n",evtp->request->sip_method);
            }
            dump_request(evtp);
            response_message(evtp);
            break;

        default:
            LOGI("type=%d unknown ", evtp->type);
            LOGE("sip client event type not support");
            break;
        }
        return 0;
}



/***
 *@remark:  讲传入的数据按位一个一个的压入数据
 *@param :  buffer   [in]  压入数据的buffer
 *          count    [in]  需要压入数据占的位数
 *          bits     [in]  压入的数值
 */
void SipClient::bits_write(bits_buffer_s *buffer, int count, int bits)
{
    struct bits_buffer_s *p_buffer = (buffer);
	int i_count = (count);
	uint64_t i_bits = (bits);
    while (i_count > 0)
    {
        i_count--;
        if ((i_bits >> i_count) & 0x01)
        {
            p_buffer->p_data[p_buffer->i_data] |= p_buffer->i_mask;
        }
        else
        {
            p_buffer->p_data[p_buffer->i_data] &= ~p_buffer->i_mask;
        }
        p_buffer->i_mask >>= 1;         /*操作完一个字节第一位后，操作第二位*/
        if (p_buffer->i_mask == 0)     /*循环完一个字节的8位后，重新开始下一位*/
        {
            p_buffer->i_data++;
            p_buffer->i_mask = 0x80; 
        }
    }
}

/***
*@remark:   ps头的封装,里面的具体数据的填写已经占位，可以参考标准
*@param :   pData  [in] 填充ps头数据的地址
*           s64Src [in] 时间戳
*@return:   0 success, others failed
*/
int SipClient::gb28181_make_ps_header(char *pData, unsigned long long s64Scr)
{
    unsigned long long lScrExt = (s64Scr) % 100;
    //s64Scr = s64Scr / 100;

    // 这里除以100是由于sdp协议返回的video的频率是90000，帧率是25帧/s，所以每次递增的量是3600,
    // 所以实际你应该根据你自己编码里的时间戳来处理以保证时间戳的增量为3600即可，
    //如果这里不对的话，就可能导致卡顿现象了
    struct bits_buffer_s bitsBuffer;
    bitsBuffer.i_size = PS_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
    bitsBuffer.p_data = (unsigned char*)(pData);
    memset(bitsBuffer.p_data, 0, PS_HDR_LEN);
    bits_write(&bitsBuffer, 32, 0x000001BA);      /*start codes*/
    bits_write(&bitsBuffer, 2, 1);           /*marker bits '01b'*/
    bits_write(&bitsBuffer, 3, (s64Scr >> 30) & 0x07);     /*System clock [32..30]*/
    bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
    bits_write(&bitsBuffer, 15, (s64Scr >> 15) & 0x7FFF);   /*System clock [29..15]*/
    bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
    bits_write(&bitsBuffer, 15, s64Scr & 0x7fff);         /*System clock [14..0]*/
    bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
    bits_write(&bitsBuffer, 9, lScrExt & 0x01ff);    /*System clock ext*/
    bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
    bits_write(&bitsBuffer, 22, (255) & 0x3fffff);    /*bit rate(n units of 50 bytes per second.)*/
    bits_write(&bitsBuffer, 2, 3);           /*marker bits '11'*/
    bits_write(&bitsBuffer, 5, 0x1f);          /*reserved(reserved for future use)*/
    bits_write(&bitsBuffer, 3, 0);           /*stuffing length*/
    return 0;
}


/***
*@remark:   sys头的封装,里面的具体数据的填写已经占位，可以参考标准
*@param :   pData  [in] 填充ps头数据的地址
*@return:   0 success, others failed
*/
int SipClient::gb28181_make_sys_header(char *pData)
{
    struct bits_buffer_s bitsBuffer;
    bitsBuffer.i_size = SYS_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char*)(pData);
    memset(bitsBuffer.p_data, 0, SYS_HDR_LEN);
    /*system header*/
    bits_write(&bitsBuffer, 32, 0x000001BB); /*start code*/
    bits_write(&bitsBuffer, 16, SYS_HDR_LEN - 6);/*header_length 表示次字节后面的长度，后面的相关头也是次意思*/
    bits_write(&bitsBuffer, 1, 1);            /*marker_bit*/
    bits_write(&bitsBuffer, 22, 50000);    /*rate_bound*/
    bits_write(&bitsBuffer, 1, 1);            /*marker_bit*/
    bits_write(&bitsBuffer, 6, 1);            /*audio_bound*/
    bits_write(&bitsBuffer, 1, 0);            /*fixed_flag */
    bits_write(&bitsBuffer, 1, 1);          /*CSPS_flag */
    bits_write(&bitsBuffer, 1, 1);          /*system_audio_lock_flag*/
    bits_write(&bitsBuffer, 1, 1);          /*system_video_lock_flag*/
    bits_write(&bitsBuffer, 1, 1);          /*marker_bit*/
    bits_write(&bitsBuffer, 5, 1);          /*video_bound*/
    bits_write(&bitsBuffer, 1, 0);          /*dif from mpeg1*/
    bits_write(&bitsBuffer, 7, 0x7F);       /*reserver*/
    /*audio stream bound*/
    bits_write(&bitsBuffer, 8, 0xC0);         /*stream_id*/
    bits_write(&bitsBuffer, 2, 3);          /*marker_bit */
    bits_write(&bitsBuffer, 1, 0);            /*PSTD_buffer_bound_scale*/
    bits_write(&bitsBuffer, 13, 512);          /*PSTD_buffer_size_bound*/
    /*video stream bound*/
    bits_write(&bitsBuffer, 8, 0xE0);         /*stream_id*/
    bits_write(&bitsBuffer, 2, 3);          /*marker_bit */
    bits_write(&bitsBuffer, 1, 1);          /*PSTD_buffer_bound_scale*/
    bits_write(&bitsBuffer, 13, 2048);       /*PSTD_buffer_size_bound*/
    return 0;
}


/***
*@remark:   psm头的封装,里面的具体数据的填写已经占位，可以参考标准
*@param :   pData  [in] 填充ps头数据的地址
*@return:   0 success, others failed
*/
int SipClient::gb28181_make_psm_header(char *pData)
{
    struct bits_buffer_s bitsBuffer;
    bitsBuffer.i_size = PSM_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char*)(pData);
    memset(bitsBuffer.p_data, 0, PSM_HDR_LEN);
    bits_write(&bitsBuffer, 24, 0x000001); /*start code*/
    bits_write(&bitsBuffer, 8, 0xBC);   /*map stream id*/
    bits_write(&bitsBuffer, 16, 18);     /*program stream map length*/
    bits_write(&bitsBuffer, 1, 1);      /*current next indicator */
    bits_write(&bitsBuffer, 2, 3);      /*reserved*/
    bits_write(&bitsBuffer, 5, 0);      /*program stream map version*/
    bits_write(&bitsBuffer, 7, 0x7F);   /*reserved */
    bits_write(&bitsBuffer, 1, 1);      /*marker bit */
    bits_write(&bitsBuffer, 16, 0);      /*programe stream info length*/
    bits_write(&bitsBuffer, 16, 8);     /*elementary stream map length  is*/
    /*audio*/
    bits_write(&bitsBuffer, 8, 0x90);       /*stream_type*/
    bits_write(&bitsBuffer, 8, 0xC0);   /*elementary_stream_id*/
    bits_write(&bitsBuffer, 16, 0);     /*elementary_stream_info_length is*/
    /*video*/
    bits_write(&bitsBuffer, 8, 0x1B);       /*stream_type*/
    bits_write(&bitsBuffer, 8, 0xE0);   /*elementary_stream_id*/
    bits_write(&bitsBuffer, 16, 0);     /*elementary_stream_info_length */
    /*crc (2e b9 0f 3d)*/
    bits_write(&bitsBuffer, 8, 0x45);   /*crc (24~31) bits*/
    bits_write(&bitsBuffer, 8, 0xBD);   /*crc (16~23) bits*/
    bits_write(&bitsBuffer, 8, 0xDC);   /*crc (8~15) bits*/
    bits_write(&bitsBuffer, 8, 0xF4);   /*crc (0~7) bits*/
    return 0;
}


/***
*@remark:   pes头的封装,里面的具体数据的填写已经占位，可以参考标准
*@param :   pData      [in] 填充ps头数据的地址
*           stream_id  [in] 码流类型
*           paylaod_len[in] 负载长度
*           pts        [in] 时间戳
*           dts        [in]
*@return:   0 success, others failed
*/
int SipClient::gb28181_make_pes_header(char *pData, int stream_id, int payload_len, unsigned long long pts, unsigned long long dts)
{
    struct bits_buffer_s   bitsBuffer;
    bitsBuffer.i_size = PES_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char*)(pData);
    memset(bitsBuffer.p_data, 0, PES_HDR_LEN);
    /*system header*/
    bits_write(&bitsBuffer, 24, 0x000001);  /*start code*/
    bits_write(&bitsBuffer, 8, (stream_id)); /*streamID*/
    bits_write(&bitsBuffer, 16, (payload_len)+13);  /*packet_len*/ //指出pes分组中数据长度和该字节后的长度和
    bits_write(&bitsBuffer, 2, 2);    /*'10'*/
    bits_write(&bitsBuffer, 2, 0);    /*scrambling_control*/
    bits_write(&bitsBuffer, 1, 0);    /*priority*/
    bits_write(&bitsBuffer, 1, 0);    /*data_alignment_indicator*/
    bits_write(&bitsBuffer, 1, 0);    /*copyright*/
    bits_write(&bitsBuffer, 1, 0);    /*original_or_copy*/
    bits_write(&bitsBuffer, 1, 1);    /*PTS_flag*/
    bits_write(&bitsBuffer, 1, 1);    /*DTS_flag*/
    bits_write(&bitsBuffer, 1, 0);    /*ESCR_flag*/
    bits_write(&bitsBuffer, 1, 0);    /*ES_rate_flag*/
    bits_write(&bitsBuffer, 1, 0);    /*DSM_trick_mode_flag*/
    bits_write(&bitsBuffer, 1, 0);    /*additional_copy_info_flag*/
    bits_write(&bitsBuffer, 1, 0);    /*PES_CRC_flag*/
    bits_write(&bitsBuffer, 1, 0);    /*PES_extension_flag*/
    bits_write(&bitsBuffer, 8, 10);    /*header_data_length*/
    // 指出包含在 PES 分组标题中的可选字段和任何填充字节所占用的总字节数。该字段之前
    //的字节指出了有无可选字段。

    /*PTS,DTS*/
    bits_write(&bitsBuffer, 4, 3);                    /*'0011'*/
    bits_write(&bitsBuffer, 3, ((pts) >> 30) & 0x07);     /*PTS[32..30]*/
    bits_write(&bitsBuffer, 1, 1);
    bits_write(&bitsBuffer, 15, ((pts) >> 15) & 0x7FFF);    /*PTS[29..15]*/
    bits_write(&bitsBuffer, 1, 1);
    bits_write(&bitsBuffer, 15, (pts) & 0x7FFF);          /*PTS[14..0]*/
    bits_write(&bitsBuffer, 1, 1);
    bits_write(&bitsBuffer, 4, 1);                    /*'0001'*/
    bits_write(&bitsBuffer, 3, ((dts) >> 30) & 0x07);     /*DTS[32..30]*/
    bits_write(&bitsBuffer, 1, 1);
    bits_write(&bitsBuffer, 15, ((dts) >> 15) & 0x7FFF);    /*DTS[29..15]*/
    bits_write(&bitsBuffer, 1, 1);
    bits_write(&bitsBuffer, 15, (dts) & 0x7FFF);          /*DTS[14..0]*/
    bits_write(&bitsBuffer, 1, 1);
    return 0;
}

int SipClient::gb28181_make_rtp_header(char *pData, int marker_flag, unsigned short cseq, long long curpts, unsigned int ssrc)
{
    struct bits_buffer_s bitsBuffer;
    if (pData == NULL)
        return -1;
    bitsBuffer.i_size = RTP_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char*)(pData);
    memset(bitsBuffer.p_data, 0, RTP_HDR_LEN);
    bits_write(&bitsBuffer, 2, RTP_VERSION);  /* rtp version  */
    bits_write(&bitsBuffer, 1, 0);        /* rtp padding  */
    bits_write(&bitsBuffer, 1, 0);        /* rtp extension  */
    bits_write(&bitsBuffer, 4, 0);        /* rtp CSRC count */
    bits_write(&bitsBuffer, 1, (marker_flag));      /* rtp marker   */
    bits_write(&bitsBuffer, 7, 96);     /* rtp payload type*/
    bits_write(&bitsBuffer, 16, (cseq));      /* rtp sequence    */
    bits_write(&bitsBuffer, 32, (curpts));    /* rtp timestamp   */
    bits_write(&bitsBuffer, 32, (ssrc));    /* rtp SSRC    */
    return 0;
}

int SipClient::SendDataBuff(char *buff, int size, int chn)
{
    /* 设置address */
    struct sockaddr_in addr_serv;
    int len;
    memset(&addr_serv, 0, sizeof(addr_serv));  //memset 在一段内存块中填充某个给定的值，它是对较大的结构体或数组进行清零操作的一种最快方法
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_addr.s_addr = inet_addr(sipConfig.g_rtpsendPs[chn].rtpServerIp);
    addr_serv.sin_port = htons(sipConfig.g_rtpsendPs[chn].rtpServerPort);
    len = sizeof(addr_serv);
    int ret = -1;
    if(sipConfig.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_UDP) {
        ret = sendto(sipConfig.g_rtpsendPs[chn].mSockFd, buff, size, 0, (struct sockaddr*)&addr_serv, len);  //send函数专用于TCP链接，sendto函数专用与UDP连接。
    }
    else if(sipConfig.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_TCP) {
        ret = send(sipConfig.g_rtpsendPs[chn].mSockFd, buff, size, 0);  //send函数专用于TCP链接，sendto函数专用与UDP连接。
    }
    else {
        LOGE("sdp transmit protocol set error,sdpProtocol: %d",sipConfig.g_rtpsendPs[chn].sdpProtocol);
    }
    if (ret <= 0) {
        LOGE("%s sendto port %d ret=%d",sipConfig.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_UDP ? "UDP" : "TCP",sipConfig.g_rtpsendPs[chn].rtpServerPort,ret);
    }
    return ret;
}


/***
*@remark:   rtp头的打包，并循环发送数据
*@param :   pData      [in] 发送的数据地址
*           nDatalen   [in] 发送数据的长度
*           mark_flag  [in] mark标志位
*           curpts     [in] 时间戳
*           pPacker    [in] 数据包的基本信息
*@return:   0 success, others failed
*/
int SipClient::gb28181_send_rtp_pack(char *databuff, int nDataLen, int mark_flag, Data_Info_s *pPacker)
{
    int nRes = 0;
    int nPlayLoadLen = 0;
    int nSendSize = 0;
    char szRtpHdr[RTP_HDR_LEN];
    memset(szRtpHdr, 0, RTP_HDR_LEN);

    if (nDataLen + RTP_HDR_LEN <= RTP_MAX_PACKET_BUFF)// 1460 pPacker指针本来有一个1460大小的buffer数据缓存
    {
        // 一帧数据发送完后，给mark标志位置1
        gb28181_make_rtp_header(szRtpHdr, ((mark_flag == 1) ? 1 : 0), ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
        memcpy(pPacker->szBuff, szRtpHdr, RTP_HDR_LEN);
        memcpy(pPacker->szBuff + RTP_HDR_LEN, databuff, nDataLen);
        nRes = SendDataBuff(pPacker->szBuff, nDataLen + RTP_HDR_LEN, pPacker->channel);
        if (nRes != (RTP_HDR_LEN + nDataLen))
        {
            LOGE(" udp send error !\n");
            return -1;
        }

    }
    else
    {
        nPlayLoadLen = RTP_MAX_PACKET_BUFF - RTP_HDR_LEN; // 每次只能发送的数据长度 除去rtp头
        gb28181_make_rtp_header(pPacker->szBuff, 0, ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
        memcpy(pPacker->szBuff + RTP_HDR_LEN, databuff, nPlayLoadLen);
        nRes = SendDataBuff(pPacker->szBuff, RTP_HDR_LEN + nPlayLoadLen, pPacker->channel);
        if (nRes != (RTP_HDR_LEN + nPlayLoadLen))
        {
            LOGE("SendDataBuff error");
            return -1;
        }

        nDataLen -= nPlayLoadLen;
        // databuff += (nPlayLoadLen - RTP_HDR_LEN);
        databuff += nPlayLoadLen; // 表明前面到数据已经发送出去
        databuff -= RTP_HDR_LEN; // 用来存放rtp头
        while (nDataLen > 0)
        {
            if (nDataLen <= nPlayLoadLen)
            {
                //一帧数据发送完，置mark标志位
                gb28181_make_rtp_header(databuff, mark_flag, ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
                nSendSize = nDataLen;
            }
            else
            {
                gb28181_make_rtp_header(databuff, 0, ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
                nSendSize = nPlayLoadLen;
            }

            nRes = SendDataBuff(databuff, RTP_HDR_LEN + nSendSize, pPacker->channel);
            if (nRes != (RTP_HDR_LEN + nSendSize))
            {
                LOGE("SendDataBuff error");
                return -1;
            }
            nDataLen -= nSendSize;
            databuff += nSendSize;
            //因为buffer指针已经向后移动一次rtp头长度后，
            //所以每次循环发送rtp包时，只要向前移动裸数据到长度即可，这是buffer指针实际指向到位置是
            //databuff向后重复的rtp长度的裸数据到位置上

        }

    }
    return 0;
}


/*** 修改
*@remark:  音视频数据的打包成ps流，并封装成rtp
*@param :  pData      [in] 需要发送的音视频数据
*          nFrameLen  [in] 发送数据的长度
*          pPacker    [in] 数据包的一些信息，包括时间戳，rtp数据buff，发送的socket相关信息
*          stream_type[in] 数据类型 0 视频 1 音频
*@return:  0 success others failed
*/
int SipClient::gb28181_streampackageForH264(char *pData, int nFrameLen, Data_Info_s *pPacker, int stream_type)
{
    char szTempPacketHead[256];
    int  nSizePos = 0;
    int  nSize = 0;
    char* pBuff = NULL;
    memset(szTempPacketHead, 0, 256);
    // 1 package for ps header
    gb28181_make_ps_header(szTempPacketHead + nSizePos, pPacker->s64CurPts);
    nSizePos += PS_HDR_LEN;
    //2 system header
    if (pPacker->IFrame == 1)
    {
        // 如果是I帧的话，则添加系统头
        gb28181_make_sys_header(szTempPacketHead + nSizePos);
        nSizePos += SYS_HDR_LEN;
        //这个地方我是不管是I帧还是p帧都加上了map的，貌似只是I帧加也没有问题

            gb28181_make_psm_header(szTempPacketHead + nSizePos);
            nSizePos += PSM_HDR_LEN;

    }

    //加上rtp发送出去，这样的话，后面的数据就只要分片分包就只有加上pes头和rtp头了
    if (gb28181_send_rtp_pack(szTempPacketHead, nSizePos, 0, pPacker) != 0)
        return -1;

    // 这里向后移动是为了方便拷贝pes头
    //这里是为了减少后面音视频裸数据的大量拷贝浪费空间，所以这里就向后移动，在实际处理的时候，要注意地址是否越界以及覆盖等问题
    pBuff = pData - PES_HDR_LEN;
    while (nFrameLen > 0)
    {
        //pes帧的长度不要超过short类型，超过就需要分片循环行发送
        nSize = (nFrameLen > PS_PES_PAYLOAD_SIZE) ? PS_PES_PAYLOAD_SIZE : nFrameLen;
        // 添加pes头
        gb28181_make_pes_header(pBuff, stream_type ? 0xC0 : 0xE0, nSize, pPacker->s64CurPts, pPacker->s64CurPts);

        //最后在添加rtp头并发送数据
        if (gb28181_send_rtp_pack(pBuff, nSize + PES_HDR_LEN, ((nSize == nFrameLen) ? 1 : 0), pPacker) != 0)
        {
            LOGE("gb28181_send_rtp_pack error");
            return -1;
        }
        //分片后每次发送的数据移动指针操作
        nFrameLen -= nSize;
        //这里也只移动nSize,因为在while向后移动的pes头长度，正好重新填充pes头数据
        pBuff += nSize;

    }
    return 0;
}

//修改
void *SipClient::SendFileDataThread(void *arg)
{
       struct Data_Info_s pPacker;
    char filepath[128] = {0};
    int IsFindFile = 0;
    int Frame_interval = 0;//ms
    int elapsedTime = 0;
    char *dirpath;
    int prefixLen;
    unsigned char naluType;
    DIR *dir = NULL;
    char *ext = NULL;
    struct dirent *entry;
    unsigned char *StreamPack = NULL;
    time_t start_time, end_time, duration;
    int channel = *(int*)arg;


    CleanupArg clean_arg;
    clean_arg.client = this;
    clean_arg.channel = channel;

    pPacker.channel = channel;
    pPacker.IFrame = 1;
    pPacker.u32Ssrc = sipConfig.g_rtpsendPs[pPacker.channel].u32Ssrc;
    pPacker.s64CurPts = 0;
    dirpath = sipConfig.g_rtpsendPs[pPacker.channel].recordDir;
    pthread_cleanup_push(cleanup_resources, &clean_arg);
    if (!(dir = opendir(dirpath))) {
        fprintf(stderr, "Cannot open directory: %s\n", dirpath);
        goto exit;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		ext = strrchr(entry->d_name, '.');
		if (ext == NULL || ((strcmp(ext, ".h264") != 0 && strcmp(ext, ".h265") != 0))) {
            continue;
        }
        sprintf(filepath, "%s/%s", dirpath, entry->d_name);
        if(sipConfig.GetStreamDuration(filepath,&start_time,&end_time,&duration) < 0) {
            LOGE("parse stream duration err\tduration: %s\n",filepath);
            continue;
        }
        end_time = start_time+duration;
        if(sipConfig.g_rtpsendPs[pPacker.channel].startTime < end_time && sipConfig.g_rtpsendPs[pPacker.channel].endTime > start_time) {
            IsFindFile = 1;
            break;
        }
    }
    closedir(dir);
    if(IsFindFile == 0) {
        LOGE("not find file startTime and endTime same need\n");
        goto exit;
    }
    elapsedTime =(sipConfig.g_rtpsendPs[pPacker.channel].startTime - start_time)*25;
    elapsedTime=elapsedTime > 0 ? elapsedTime : 0;
    sipConfig.g_rtpsendPs[pPacker.channel].fp = fopen(filepath, "rb");
    if(sipConfig.g_rtpsendPs[pPacker.channel].fp == NULL){
        LOGE("fopen error:%s\n",filepath);
        goto exit;
    }
    StreamPack = (unsigned char*)malloc(sizeof(unsigned char) * sipConfig.g_streamSize.height * sipConfig.g_streamSize.width);

    memset(StreamPack, 0x00, sizeof(unsigned char) * sipConfig.g_streamSize.height * sipConfig.g_streamSize.width);
    sipConfig.g_rtpsendPs[pPacker.channel].mPlay = true;
    sipConfig.g_rtpsendPs[pPacker.channel].mRun = true;
    sipConfig.g_rtpsendPs[pPacker.channel].buffer = StreamPack;

    while (!sipConfig.g_rtpsendPs[pPacker.channel].mQuit) {
        while (!sipConfig.g_rtpsendPs[pPacker.channel].mPlay && !sipConfig.g_rtpsendPs[pPacker.channel].mQuit) {
            usleep(100*1000);
        }
        int size = Config::getNextNalu(sipConfig.g_rtpsendPs[pPacker.channel].fp, (unsigned char*)(StreamPack + PES_HDR_LEN)); //PES_HDR_LEN 19   size：发送数据的长度
        if (size <= 0) {
            LOGI("Send data completed, active exit\n");
            request_media_status(pPacker.channel);
            break;
        }
        if(Config::getNalupos2((unsigned char*)(StreamPack + PES_HDR_LEN), size, &prefixLen, &naluType) < 0) {
            continue;
        }
        if(elapsedTime > 0) {
            if((naluType&0x1F) == 0x01 || (naluType&0x1F) == 0x05) {
                elapsedTime--;
            }
        }
        // 将h264码流读取到的一个一个nalu封装到ps并通过rtp推流
        gb28181_streampackageForH264((char *)StreamPack + PES_HDR_LEN, size, &pPacker, 0); //0 表示传递的是视频数据
        if((naluType&0x1F) == 0x01 || (naluType&0x1F) == 0x05) {
            pPacker.s64CurPts += 90000/sipConfig.g_rtpsendPs[pPacker.channel].fps/sipConfig.g_rtpsendPs[pPacker.channel].scale;
            Frame_interval=(int)(float)(1000/sipConfig.g_rtpsendPs[pPacker.channel].fps/sipConfig.g_rtpsendPs[pPacker.channel].scale);
            usleep((Frame_interval) * 1000);//函数的休眠单位是微秒，这里休眠40毫秒
        }
    }
exit:
    // sipConfig.g_rtpsendPs[pPacker.channel].mPlay = false;
    // sipConfig.g_rtpsendPs[pPacker.channel].mQuit = true;
    // if(StreamPack)
    //     free(StreamPack);
    // StreamPack = NULL;
    // close(sipConfig.g_rtpsendPs[pPacker.channel].mSockFd);
    // fclose(sipConfig.g_rtpsendPs[pPacker.channel].fp);
    // sipConfig.g_rtpsendPs[pPacker.channel].mSockFd = 0;
    // sipConfig.g_rtpsendPs[pPacker.channel].fp = NULL;
    // sipConfig.g_rtpsendPs[pPacker.channel].mRun = false;
    pthread_cleanup_pop(1);
    pthread_exit((void *)0);
}

// 修改
void *SipClient::SendDataThread(void *arg)
{
    int ringbuflen = 0;
    struct ringbuf ringfifo;
    struct Data_Info_s pPacker;
    int prefixLen;
    unsigned char naluType;
    int  channel = *(int *)arg;

    CleanupArg clean_arg;
    clean_arg.client = this;
    clean_arg.channel = channel;


    pPacker.channel = channel;
    pPacker.IFrame = 1;
    pPacker.u32Ssrc = sipConfig.g_rtpsendPs[pPacker.channel].u32Ssrc;
    pPacker.s64CurPts = 0;
    int ofs = sipConfig.get_fifo_offset();
    StreamControlFunc stream_controller = sipConfig.get_gb_stream_controller();

    // char h264_file_name[64] = {0};
    // sprintf(h264_file_name, "/home/test_save_vodeo%d.h264", pPacker.channel);
    // FILE *file_h264 = fopen(h264_file_name,"w"); 
    unsigned char * StreamPack = (unsigned char*)malloc(sizeof(unsigned char) * sipConfig.g_streamSize.height * sipConfig.g_streamSize.width);
    memset(StreamPack, 0x00, sizeof(unsigned char) * sipConfig.g_streamSize.height * sipConfig.g_streamSize.width);
    sipConfig.g_rtpsendPs[pPacker.channel].buffer = StreamPack;
    pthread_cleanup_push(cleanup_resources, &clean_arg);
    Config::ringmalloc_fifochn(pPacker.channel+ofs,sipConfig.g_ringFifoSize.height * sipConfig.g_ringFifoSize.width); //初始化内存

    triggerCallbacks(0x01,channel);

    if(stream_controller)
        stream_controller(pPacker.channel,true);
    sipConfig.g_rtpsendPs[pPacker.channel].scale = 1;
    sipConfig.g_rtpsendPs[pPacker.channel].mRun = true;
    sipConfig.g_rtpsendPs[pPacker.channel].mQuit = false;
    sipConfig.g_rtpsendPs[pPacker.channel].usefifo = true;
    while (!sipConfig.g_rtpsendPs[pPacker.channel].mQuit) {
        ringbuflen = Config::ringget_fifochn(pPacker.channel+ofs,&ringfifo);
        if(ringbuflen !=0)
		{
            // LOGD("gb client get video[%d]  data \n",pPacker.channel);
            // fwrite(ringfifo.buffer, ringfifo.size, 1, file_h264);
            // 将h264码流读取到的一个一个nalu封装到ps并通过rtp推流
            if(Config::getNalupos2((unsigned char*)ringfifo.buffer, ringfifo.size, &prefixLen, &naluType) < 0) {
                pPacker.IFrame = 1;
            } else {
                if(sipConfig.g_rtpsendPs[pPacker.channel].pyloadType == GB_H264) {
                    if((naluType&0x1F) >= 0x05 && (naluType&0x1F) < 0x09)
                        pPacker.IFrame = 1;
                    else
                        pPacker.IFrame = 0;
                } else if(sipConfig.g_rtpsendPs[pPacker.channel].pyloadType == GB_H265) {
                    if(((naluType&0x7E)>>1) >= 0x10)
                        pPacker.IFrame = 1;
                    else
                        pPacker.IFrame = 0;
                }
            }
            memcpy(StreamPack + PES_HDR_LEN, ringfifo.buffer, ringfifo.size);
            gb28181_streampackageForH264((char *)StreamPack + PES_HDR_LEN, ringfifo.size, &pPacker, 0); //0 表示传递的是视频数据
            pPacker.s64CurPts += 90000/sipConfig.g_rtpsendPs[pPacker.channel].fps/sipConfig.g_rtpsendPs[pPacker.channel].scale;
		}
        else {
            usleep(100*1000);
        }
    }
    sipConfig.g_rtpsendPs[pPacker.channel].usefifo = false;
    sipConfig.g_rtpsendPs[pPacker.channel].mQuit = true;
    // free(StreamPack);
    // sipConfig.g_rtpsendPs[pPacker.channel].mRun = false;
    if(stream_controller)
        stream_controller(pPacker.channel,false);
    Config::ringfree_fifochn(pPacker.channel+ofs);
    pthread_cleanup_pop(1);
    pthread_exit((void *)0);
}

int SipClient::rtpsendPs_init(int chn)
{
    struct sockaddr_in clientaddr,serveraddr;

    // 创建TCP/UDP套接字
    if(sipConfig.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_UDP) {
        if ((sipConfig.g_rtpsendPs[chn].mSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            perror("socket creation failed");
            LOGE("创建套接字失败");
            return -1;
        }
    }
    else if(sipConfig.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_TCP) {
        if ((sipConfig.g_rtpsendPs[chn].mSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
            perror("socket creation failed");
            LOGE("创建套接字失败");
            return -1;
        }
    }

    // 设置SO_REUSEADDR套接字选项
    int optval = 1;
    if (setsockopt(sipConfig.g_rtpsendPs[chn].mSockFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt failed");
        close(sipConfig.g_rtpsendPs[chn].mSockFd);
        LOGE("setsockopt failed");
        return -1;
    }
    
    // 设置本地发送端口
    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = inet_addr(g_rtpsendPs.rtplocalIp);
    clientaddr.sin_addr.s_addr = INADDR_ANY;
    clientaddr.sin_port = htons(sipConfig.g_rtpsendPs[chn].rtpLocalPort);

   // 绑定本地地址和端口
    if (bind(sipConfig.g_rtpsendPs[chn].mSockFd, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
        perror("bind failed");
        close(sipConfig.g_rtpsendPs[chn].mSockFd);
        LOGE("绑定套接字失败 mSockFd %d",sipConfig.g_rtpsendPs[chn].mSockFd);
        return -1;
    }

    if(sipConfig.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_TCP) {
        // 设置服务器地址和端口
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        inet_pton(AF_INET, sipConfig.g_rtpsendPs[chn].rtpServerIp, &serveraddr.sin_addr);
        serveraddr.sin_port = htons(sipConfig.g_rtpsendPs[chn].rtpServerPort);

        // 连接到服务器
        if (connect(sipConfig.g_rtpsendPs[chn].mSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
        {
            perror("connection err\r\n");
            close(sipConfig.g_rtpsendPs[chn].mSockFd);
            LOGE("connection err\r\n");
            return -1;
        }
    }
    //设置为非阻塞模式
    int ul = 1;
    if(ioctl(sipConfig.g_rtpsendPs[chn].mSockFd, FIONBIO, &ul) == -1)
    {
        perror("ioctl err\r\n");
        close(sipConfig.g_rtpsendPs[chn].mSockFd);
        LOGE("设置套接字非阻塞失败");
        return -1;
    }
    return 0;
}


/*****************************************************************************
//	描述：	停止推流 修改
//	输入参数：
//chn 通道
*****************************************************************************/
int SipClient::rtpsendPs_stop(int chn)
{
   int i;


    StreamControlFunc stream_controller = sipConfig.get_gb_stream_controller();
    sipConfig.g_rtpsendPs[chn].mQuit = true;
    sipConfig.g_rtpsendPs[chn].mPlay = false;
    sipConfig.g_rtpsendPs[chn].usefifo = false;
    sipConfig.g_rtpsendPs[chn].scale = 1;
    i = 0;
    while(sipConfig.g_rtpsendPs[chn].mRun == true && i < 30) { 
        usleep(100*1000);
        i++;
    }

    sipConfig.g_rtpsendPs[chn].pid_thread.join();
    if(sipConfig.g_rtpsendPs[chn].mSockFd)
        close(sipConfig.g_rtpsendPs[chn].mSockFd);
    if(sipConfig.g_rtpsendPs[chn].fp)
        fclose(sipConfig.g_rtpsendPs[chn].fp);
    if(stream_controller)
        stream_controller(chn,false);
    //sipConfig.g_rtpsendPs[chn].pid = 0;
    sipConfig.g_rtpsendPs[chn].mSockFd = 0;
    sipConfig.g_rtpsendPs[chn].fp = NULL;
    Config::ringfree_fifochn(chn+sipConfig.get_fifo_offset());
    return 0;
}

/*****************************************************************************
//	描述：	接收到ack响应，开始推流
//	输入参数：
//chn 通道
*****************************************************************************/
int SipClient::rtpsendPs_start(int chn)
{
    //int ret;
    int *channel = &sipConfig.g_rtpsendPs[chn].channel;
    if(sipConfig.g_rtpsendPs[chn].mQuit){

        if(!strcmp(sipConfig.g_rtpsendPs[chn].sessionName,"Play"))//实时视频
        {
            //printf("sdjfowesdfwerefsfd\n");
            //ret = pthread_create(&sipConfig.g_rtpsendPs[chn].pid, NULL, SendDataThread, (void *)channel);
            sipConfig.g_rtpsendPs[chn].pid_thread = std::thread([this, channel]() {
                SendDataThread((void *)channel);//
            });
            
        }
        else if(!strcmp(sipConfig.g_rtpsendPs[chn].sessionName,"Playback"))//设备录像
        {
            //ret = pthread_create(&sipConfig.g_rtpsendPs[chn].pid, NULL, SendFileDataThread, (void *)channel);
            sipConfig.g_rtpsendPs[chn].pid_thread = std::thread([this, channel]() {
                SendFileDataThread((void *)channel);
            });
        }
        else if(!strcmp(sipConfig.g_rtpsendPs[chn].sessionName,"Download"))//云端下载
        {
            //ret = pthread_create(&sipConfig.g_rtpsendPs[chn].pid, NULL, SendFileDataThread, (void *)channel);
            sipConfig.g_rtpsendPs[chn].pid_thread = std::thread([this, channel]() {
                SendFileDataThread((void *)channel);
            });
        }
        else
        {
            LOGE("%s Operation not supported\n",sipConfig.g_rtpsendPs[chn].sessionName);
            return -1;
        }
        // if (ret != 0)
        // {
        //     LOGE("Error create rtp[%d] thread\n",chn);
        //     return -1;
        // }
        // LOGD("pthread_create[%d] pid = %lu\n",chn,g_rtpsendPs[chn].pid);
    }
    else {
        LOGE("start already exist stream\n");
    }
    return 0;
}

int SipClient::response_invite_ack(eXosip_event_t *evtp)
{
     osip_message_t *msg = NULL;
    int ret = eXosip_call_build_ack(mSipCtx, evtp->did, &msg);
    if (!ret && msg) {
        eXosip_call_send_ack(mSipCtx, evtp->did, msg);
        return 0;
    } else {
        LOGE("eXosip_call_send_ack error=%d", ret);
    }
    return -1;
}

void SipClient::triggerCallbacks(int tslKey, int content)
{
   
    auto it = parameterCallbacks_.find(tslKey);
    if (it != parameterCallbacks_.end()) {
        for (auto& callback : it->second) {
            callback(tslKey, content);
        }
    }
}
