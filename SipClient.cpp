#include "SipClient.h"
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
SipClient::SipClient()
{
    std::thread test(&SipClient::test_h264, this);
    test.detach();
    //Config::SetToFile("GB28181", "port","6666");
    //gb28181_client_start();
    //std::cout << "Database Host: " << config.Get("Database", "host") << std::endl;
    // std::cout << "Database Port: " << config.Get("Database", "port") << std::endl;
    // std::cout << "Server IP: " << config.Get("Server", "ip") << std::endl;
    // std::cout << "Server Port: " << config.Get("Server", "port") << std::endl;
}

SipClient::~SipClient()
{
    //gb28181_client_close();
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
            config.SendStreamData(i, H264Stream,size, 0);
        //rtmp_push_stream_data(i, H264Stream,size, 0);
        //rtsp_push_stream_data(i+1, H264Stream,size, 0);
        if((naluType&0x1F) == 0x01 || (naluType&0x1F) == 0x05)
            usleep(39*1000);
    }
}

int SipClient::gb28181_client_init(void)
{
    config.SetSipClientDefaultInfo();
    return 0;
}

int SipClient::gb28181_client_start(void)
{ 
    gb28181_client_init();
/*************************************************************************** */
/* Structure encapsulation protection configuration is required  */
    //user set
	strcpy(config.g_devinfo.server_domain,"4401020049");
	strcpy(config.g_devinfo.server_id,"44010200492000000001");
	strcpy(config.g_devinfo.server_ip,"219.134.62.202");//121.15.156.83 129.211.172.129  192.168.10.221 219.134.62.202
    
	strcpy(config.g_devinfo.ipc_id,"44010200492000000011");
	strcpy(config.g_devinfo.ipc_usrname,"44010200492000000011");
	strcpy(config.g_devinfo.ipc_pwd,"admin123");

	strcpy(config.g_devinfo.ipc_ip,Config::Get("GB28181","Local_ip").c_str());//

    config.g_devinfo.server_port = 5060;
    config.g_devinfo.ipc_port = 5060;

	config.g_devinfo.reg_expires = 3600;
	config.g_devinfo.hb_period= 60;//

    config.SetRtpStreamFps(25);
    config.SetRtpFifoSize(640, 360);
    config.SetRtpStreamResolution(1280, 720);

    config.SetRtpRecordFileDir("./data");
/*************************************************************************** */
    //pthread_create(&gclntsta.pid, NULL, gb28181_client_func, NULL);
    // std::thread ConnectThread([this] { gb28181_client_func(this); });
    // ConnectThread.detach();

    config.gclntsta.threads = std::thread([this] { gb28181_client_func(this); });
    LOGD("gb28181 client start\n");
    return 0;
}

int SipClient::gb28181_client_close(void)
{
    if(config.GetSipClientStatus())
        SipClientExit();
    return 0;
}


void *SipClient::gb28181_client_func(void *arg)
{
   static bool allowReg = true;
    
    int64_t lastKeepaliveTimestamp = 0;
    int64_t lastRegistereTimestamp = 0;
    int64_t curTimestamp = 0;
    int64_t hbInterval = config.g_devinfo.hb_period*1000;// 客户端发送keepalive检测的间隔，单位：毫秒  3000
    int64_t regInterval =config.g_devinfo.reg_expires*1000;// 客户端发送keepalive检测的间隔，单位：毫秒  3000

    config.gclntsta.quit = false;
    config.gclntsta.mRegistered = false;

    if(init_sip_client() != 0)
    {
        goto exit;
    }

    while (!config.gclntsta.quit) {

        // 首次发起注册
        if (allowReg && !config.gclntsta.mRegistered) {
                allowReg = false;
            request_register();
        }

        // 心跳机制 start （开发过程中，为防止影响抓包，可以先注释）
        if(config.gclntsta.mRegistered){
            curTimestamp = config.getCurTimestamp();
            if(lastKeepaliveTimestamp == 0){
                lastKeepaliveTimestamp = curTimestamp;
            } else {
                if((curTimestamp - lastKeepaliveTimestamp) > hbInterval){
                    request_message_keepalive();
                    lastKeepaliveTimestamp = curTimestamp;
                    lastRegistereTimestamp = curTimestamp;
                }
            }
            if(lastRegistereTimestamp == 0){
                lastRegistereTimestamp = curTimestamp;
            } else {
                if((curTimestamp - lastRegistereTimestamp) > regInterval){
                    request_register();
                    lastRegistereTimestamp = curTimestamp;
                }
            }
        }
        // 心跳机制 end
        eXosip_event_t* evtp = eXosip_event_wait(mSipCtx, 0, 20);
        if (!evtp) {
            eXosip_automatic_action(mSipCtx);
            osip_usleep(100*1000);
            continue;
        }
        eXosip_automatic_action(mSipCtx);
        sip_event_handle(evtp); //事件处理
        eXosip_event_free(evtp); // 释放
        usleep(100*1000);
    }
    eXosip_quit(mSipCtx);
exit:
    config.gclntsta.quit=true;
    pthread_exit((void *)0);
}


int SipClient::SipClientExit()
{
    LOGD("gb28181 client exit start\n");
    LOGI(" Sip Client run status: %s\n",config.gclntsta.quit==true?"NoWORK":"OnWORK");
    //LOGI(" Sip Client thread pid: %lu\n",config.gclntsta.pid);
    if(!config.gclntsta.quit && config.gclntsta.threads.joinable())
    {
        int i;
        for(i = 0; i <MAX_CHN; i++)
        if(!config.g_rtpsendPs[i].mQuit){
            rtpsendPs_stop(i);
        }
        pthread_cancel(config.gclntsta.pid);
        config.gclntsta.quit = true;
        // pthread_join(config.gclntsta.pid, NULL);
        config.gclntsta.threads.join();
        //config.gclntsta.pid = 0;
        config.gclntsta.quit = true;
    }
    LOGD("gb28181 client exit end\n");
    return 0;
}


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

    if(config.g_devinfo.sip_protocol == GB28181_TRANS_TYPE_UDP)
        ret = eXosip_listen_addr(mSipCtx,IPPROTO_UDP, NULL, config.g_devinfo.ipc_port, AF_INET, 0);
    else if(config.g_devinfo.sip_protocol == GB28181_TRANS_TYPE_TCP)
        ret =eXosip_listen_addr(mSipCtx, IPPROTO_TCP, NULL, config.g_devinfo.ipc_port, AF_INET, 0);
    if(ret)
    {
        LOGE("listen error");
        eXosip_quit(mSipCtx);
        return -1;
    }

    //user agent
    eXosip_set_user_agent(mSipCtx, config.g_devinfo.ipc_ua);
    if (eXosip_add_authentication_info(mSipCtx,config.g_devinfo.ipc_usrname, config.g_devinfo.ipc_id, config.g_devinfo.ipc_pwd, NULL, NULL) < 0) {
        LOGI("eXosip_add_authentication_info error");
        return -1;
    }
    LOGI("eXosip_listen_addr port %d success!\r\n",config.g_devinfo.ipc_port);
    return 0;
}


/*****************************************************************************
//	描述：	从xml数据中获取指定数据
//data xml数据
//s_mark 开始标记 用于定位内容的开始位置
//with_s_make true则不包括s_mark的长度 false则包括s_mark的长度
//e_mark 结束标记 用于定位内容的结束位置
//evtp 事件结构体
//evtp 事件结构体
//evtp 事件结构体
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
//	描述：	开始注册gb28181用户
*****************************************************************************/
int SipClient::request_register()
{
    int ret = -1;

    osip_message_t* msg = NULL;
    char from[1024] = { 0 };
    char contact[1024] = { 0 };
    char proxy[1024] = { 0 };

    if (config.gclntsta.mRegistered) { // refresh register
        LOGI("refresh register mRegisterId=%d", config.gclntsta.mRegisterId);
        ret = eXosip_register_build_register(mSipCtx, config.gclntsta.mRegisterId, config.g_devinfo.reg_expires, &msg);
        if (!ret) {
            LOGE("eXosip_register_build_register error: ret=%d", ret);
            return -1;
        }
    }
    else { // new register
        LOGI("unregistered mRegisterId=%d", config.gclntsta.mRegisterId);

        sprintf(from, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
        sprintf(proxy, "sip:%s@%s:%d", config.g_devinfo.server_id, config.g_devinfo.server_ip, config.g_devinfo.server_port);
        sprintf(contact, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
        config.gclntsta.mRegisterId = eXosip_register_build_initial_register(mSipCtx, from, proxy, contact, config.g_devinfo.reg_expires, &msg);
        
        LOGE("register info:\nfrom %s \nproxy %s\ncontact %s\n", from,proxy,contact);
        if (config.gclntsta.mRegisterId <= 0) {
            LOGE("register info:\nfrom %s \nproxy %s\ncontact %s\n", from,proxy,contact);
            LOGE("eXosip_register_build_initial_register error: mRegisterId=%d", config.gclntsta.mRegisterId);
            return -1;
        }
    }
    eXosip_lock(mSipCtx);
    ret = eXosip_register_send_register(mSipCtx, config.gclntsta.mRegisterId, msg);
    eXosip_unlock(mSipCtx);
    if (ret) {
        LOGE("eXosip_register_send_register error: ret=%d", ret);
        return ret;
    }

    char* msg_str;
    size_t msg_strlen;
    osip_message_to_str(msg, &msg_str, &msg_strlen);
    LOGI("send request registration message: \n%s", msg_str);
    osip_free(msg_str);
    return ret;
}

int SipClient::request_message_keepalive()
{
    char from[1024] = { 0 };
    char to[1024] = { 0 };

    sprintf(from, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
    sprintf(to, "sip:%s@%s:%d", config.g_devinfo.server_id, config.g_devinfo.server_ip, config.g_devinfo.server_port);

    osip_message_t* msg;
    char body[1024] = { 0 };

    config.gclntsta.hb_sn++;
	if(config.gclntsta.hb_sn > 0xffff)
		config.gclntsta.hb_sn = 1;

    sprintf(
        body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Notify>\r\n"
        "<CmdType>Keepalive</CmdType>\r\n"
        "<SN>%d</SN>\r\n"
        "<DeviceID>%s</DeviceID>\r\n"
        "<Status>OK</Status>\r\n"
        "</Notify>\r\n",
        config.gclntsta.hb_sn,config.g_devinfo.ipc_id);
    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send request keepalive message: \n%s", s);
    osip_free(s);
    return 0;
}

int SipClient::request_media_status(int chn)
{
     char from[1024] = { 0 };
    char to[1024] = { 0 };
    char call_id[64] = { 0 };

    sprintf(from, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
    sprintf(to, "sip:%s@%s:%d", config.g_devinfo.server_id, config.g_devinfo.server_ip, config.g_devinfo.server_port);

    osip_message_t* msg;
    char body[1024] = { 0 };

    config.gclntsta.alarm_sn++;
	if(config.gclntsta.alarm_sn > 0xffff)
		config.gclntsta.alarm_sn = 1;

    sprintf(
        body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Notify>\r\n"
        "<CmdType>MediaStatus</CmdType>\r\n"
        "<SN>%d</SN>\r\n"
        "<DeviceID>%s</DeviceID>\r\n"
        "<NotifyType>121</NotifyType>\r\n"
        "</Notify>\r\n",
        config.gclntsta.alarm_sn,config.g_devinfo.device_id[chn]);
    sprintf(call_id,"%s@%s",config.g_rtpsendPs[chn].callidNumber,config.g_rtpsendPs[chn].callidHost);
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
    LOGI("send request media status message: \n%s", s);
    return 0;
}

/*****************************************************************************
//	描述：创建数据通道线程
//	输入参数：
//args 序号
*****************************************************************************/
void *SipClient::request_message_catalog_proc(void *args)
{
     int i,serialNo;
    int * param = (int *)args;
    serialNo = * param;
    char from[1024] = { 0 };
    char to[1024] = { 0 };

    sprintf(from, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
    sprintf(to, "sip:%s@%s:%d", config.g_devinfo.server_id, config.g_devinfo.server_ip, config.g_devinfo.server_port);

    osip_message_t* msg;
    char body[1024] = { 0 };

    for(i=0;i<MAX_CHN;i++)
    {
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
                        "<Manufacturer>%s</Manufacturer\r\n>"
                        "<Model>%s</Model>\r\n"
                        "<Owner>Owner</Owner>\r\n"
                        "<CivilCode>%s</CivilCode>\r\n"
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
            serialNo,config.g_devinfo.ipc_id,MAX_CHN,config.g_devinfo.device_id[i],config.g_devinfo.device_name[i],config.g_devinfo.device_manufacturer, \
            config.g_devinfo.device_model,config.g_devinfo.ipc_id,config.g_devinfo.ipc_id,config.g_devsta.status_on);

        eXosip_lock(mSipCtx);
        eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
        osip_message_set_body(msg, body, strlen(body));
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(mSipCtx, msg);
        eXosip_unlock(mSipCtx);
        char *s;
        size_t len;
        osip_message_to_str(msg, &s, &len);
        //printf("send request catalog message: \n%s", s);
        LOGI("send request catalog message: \n%s", s);
        osip_free(s);
        usleep(100*1000);
    }
    pthread_exit((void *)0);
}


/*****************************************************************************
//	描述：创建线程来实现创建通道
//	输入参数：
//serialNo 序号
*****************************************************************************/
int SipClient::request_message_catalog(int serialNo)
{
    int *attr = &serialNo;
    // pthread_t pid; 
    // int ret = pthread_create(&pid, NULL, request_message_catalog_proc, (void *)attr);
    // if (ret != 0) {
    //     LOGE("sd card check status pthread create failed\n");
    //     return -1;
    // }
    // pthread_detach(pid);
    std::thread t([this, attr]() {
        request_message_catalog_proc(attr);
    });
    t.detach();
    return 0;
}

int SipClient::request_message_deviceinfo(int serialNo)
{
    char from[1024] = { 0 };
    char to[1024] = { 0 };

    sprintf(from, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
    sprintf(to, "sip:%s@%s:%d", config.g_devinfo.server_id, config.g_devinfo.server_ip, config.g_devinfo.server_port);

    osip_message_t* msg;
    char body[1024] = { 0 };

    sprintf(
        body,
        "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\" ?>\r\n"
        "<Response>\r\n"
            "<CmdType>DeviceInfo</CmdType>\r\n"
            "<SN>%d</SN>\r\n"
            "<DeviceID>%s</DeviceID>\r\n"
//            "<DeviceName>%s</DeviceName>\r\n" //g_devinfo.ipc_name,
            "<Result>OK</Result>\r\n"
            "<DeviceType>IPC</DeviceType>\r\n"
            "<Manufacturer>%s</Manufacturer\r\n>"
            "<Model>%s</Model>\r\n"
            "<Firmware>%s</Firmware>\r\n"
            "<MaxCamera>1</MaxCamera>\r\n"
            "<MaxAlarm>1</MaxAlarm>\r\n"
        "</Response>\r\n",
        serialNo,config.g_devinfo.ipc_id, config.g_devinfo.device_manufacturer, \
        config.g_devinfo.device_model,config.g_devinfo.device_firmware );
    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send cmd catalog: \n%s", s);
    osip_free(s);
    return 0;
}

void SipClient::dump_request(eXosip_event_t *evt)
{
    char* s;
    size_t len;
    osip_message_to_str(evt->request, &s, &len);
    LOGI("\nPrint request packet starts\n\n%s\nEnd of print request packet\n", s);
    osip_free(s);
}

void SipClient::dump_response(eXosip_event_t *evt)
{
    char* s;
    size_t len;
    osip_message_to_str(evt->response, &s, &len);
    LOGI("\n打印响应包开始\ntype=%d\n%s\n打印响应包结束\n", evt->type, s);
    osip_free(s);
}

int SipClient::response_message_answer(eXosip_event_t *evtp, int code)
{
    osip_message_t* msg = NULL;
    int returnCode = eXosip_message_build_answer(mSipCtx, evtp->tid, code, &msg);

    if (returnCode == 0 && msg) {
        eXosip_lock(mSipCtx);
        eXosip_message_send_answer(mSipCtx, evtp->tid, code, msg);
        eXosip_unlock(mSipCtx);
        // osip_message_free(msg);
    }
    else {
        bool msg_state = false;
        if (msg) {
            msg_state = true;
        }
        LOGE("error: code=%d,returnCode=%d,msg=%d", code, returnCode, msg_state);
    }

    return 0;
}

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
    channel = config.FindChannelByDeviceId(username);
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
        config.g_rtpsendPs[channel].mPlay = false;
    }
    else if (!strncmp(Method, "PLAY", strlen("PLAY"))) {
        if(Scale > 0)
            config.g_rtpsendPs[channel].scale = Scale;
        config.g_rtpsendPs[channel].mPlay = true;
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

void *SipClient::request_message_recordInfo_proc(void *args)
{
    struct RecordInfo *recordInfo = (struct RecordInfo *)args;

    char from[1024] = { 0 };
    char to[1024] = { 0 };
    char body[1024] = { 0 };
    char dirpath[128] = {0};
    char filepath[128] = {0};
    char videoStartTime[32] = {0};
    char videoEndTime[32] = {0};

    int serialNo = recordInfo->serialNo;
    int channel = recordInfo->channel;
    time_t filterStartTime = recordInfo->startTime; 
    time_t filterEndTime = recordInfo->endTime; 

    int i = 0,total = 0;
    char *s;
    size_t len;
    osip_message_t* msg;

    DIR *dir;
    struct stat st;
    struct tm time_info, start_info, end_info;
    struct dirent *entry;
    time_t start_time, end_time;

    sprintf(from, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
    sprintf(to, "sip:%s@%s:%d", config.g_devinfo.server_id, config.g_devinfo.server_ip, config.g_devinfo.server_port);

    if(config.FindVideoByTime(config.g_rtpsendPs[channel].recordDir,channel,filterStartTime,filterEndTime,&total) < 0)
        total = 0;
    if(total == 0)
    {
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
            serialNo,config.g_devinfo.ipc_id, total);
        eXosip_lock(mSipCtx);
        eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
        osip_message_set_body(msg, body, strlen(body));
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(mSipCtx, msg);
        eXosip_unlock(mSipCtx);

        osip_message_to_str(msg, &s, &len);
        LOGI("send request recordinfo message: \n%s", s);
        osip_free(s);
        return 0;
    }
    sprintf(dirpath, "%s", config.g_rtpsendPs[channel].recordDir);
    if (!(dir = opendir(dirpath))) {
        fprintf(stderr, "Cannot open directory: %s\n", dirpath);
        return 0;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        if(config.ParseFileNameTime(entry->d_name, &start_time) < 0)
        {
            LOGE("parse file name time err\n");
            continue;
        }
        if (start_time >= filterStartTime && start_time <= filterEndTime) {
            localtime_r(&recordInfo->startTime, &start_info);
            localtime_r(&recordInfo->endTime, &end_info);
            LOGD("File: %s was created between %s and %s    filetimestamp = %ld  starttimestamp = %ld  endtimestamp = %ld\n", entry->d_name, 
                asctime(&start_info), asctime(&start_info), start_time,filterStartTime,filterEndTime);
            sprintf(filepath, "%s/%s", dirpath, entry->d_name);
            if (stat(filepath, &st) == -1) {
                LOGE("Get file stat err %s\n",filepath);
                continue;
            }
            LOGD("st.st_mtime = %ld st.st_ctime = %ld \n",st.st_mtime,st.st_ctime);
            end_time = st.st_mtime;//st_ctime
        }
        else
        {
            continue;
        }
        localtime_r(&start_time, &start_info);
        localtime_r(&end_time, &end_info);
        strftime(videoStartTime, sizeof(videoStartTime), "%Y-%m-%dT%H:%M:%S", &start_info);
        start_info.tm_isdst = -1;
        LOGD("video start time = %s timestamp = %ld\n",videoStartTime,mktime(&start_info));
        
        strftime(videoEndTime, sizeof(videoEndTime), "%Y-%m-%dT%H:%M:%S", &end_info);
        end_info.tm_isdst = -1;
        LOGD("video end time = %s timestamp = %ld\n",videoEndTime,mktime(&end_info));
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
            serialNo,config.g_devinfo.ipc_id, total, config.g_devinfo.device_id[channel], entry->d_name, videoStartTime, videoEndTime);
        eXosip_lock(mSipCtx);
        eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
        osip_message_set_body(msg, body, strlen(body));
        osip_message_set_content_type(msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(mSipCtx, msg);
        eXosip_unlock(mSipCtx);
        char *s;
        size_t len;
        osip_message_to_str(msg, &s, &len);
        LOGD("Number of recordings is %d\n",++i);
        LOGI("send cmd catalog: \n%s", s);
        osip_free(s);
        usleep(100*1000);
    }
    closedir(dir);
    pthread_exit((void *)0);
}

int SipClient::request_message_recordInfo(int serialNo, char *deviceID, char *startTime, char *endTime)
{
    struct RecordInfo recordInfo;
    struct tm timeInfo;
    recordInfo.serialNo = serialNo;
    recordInfo.channel = config.FindChannelByDeviceId(deviceID);
    LOGD("StartTime = %s\tEndTime = %s\n",startTime, endTime);
    if(recordInfo.channel < 0)
    {
        LOGE("channel list not find the device id %s\n", deviceID);
        return -1;
    }
    if(config.MyStrptime(startTime, "%Y-%m-%dT%H:%M:%S", &timeInfo) == NULL)
    {
        LOGE("parse time err\n");
        return -1;
    }
    timeInfo.tm_isdst = -1;
    recordInfo.startTime = mktime(&timeInfo);
    LOGD("start timestamp = %ld\n",recordInfo.startTime);
    if(config.MyStrptime(endTime, "%Y-%m-%dT%H:%M:%S", &timeInfo) == NULL)
    {
        LOGE("parse time err\n");
        return -1;
    }
    timeInfo.tm_isdst = -1;
    recordInfo.endTime = mktime(&timeInfo);
    LOGD("end timestamp = %ld\n",recordInfo.endTime);
    // pthread_t pid; 
    // int ret = pthread_create(&pid, NULL, request_message_recordInfo_proc, (void *)&recordInfo);
    // if (ret != 0) {
    //     LOGE("sd card check status pthread create failed\n");
    //     return -1;
    // }
    // pthread_detach(pid);
     std::thread t([this, &recordInfo]() {
        request_message_recordInfo_proc(&recordInfo);
    });
    t.detach();
    return 0;
}

int SipClient::request_message_devicestatus(int SerialNo)
{
    char from[1024] = { 0 };
    char to[1024] = { 0 };

    sprintf(from, "sip:%s@%s:%d", config.g_devinfo.ipc_id, config.g_devinfo.ipc_ip, config.g_devinfo.ipc_port);
    sprintf(to, "sip:%s@%s:%d", config.g_devinfo.server_id, config.g_devinfo.server_ip, config.g_devinfo.server_port);

    osip_message_t* msg;
    char body[1024] = { 0 };
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(config.g_devsta.status_time, sizeof(config.g_devsta.status_time), "%Y-%m-%d %H:%M:%S", t);
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
        SerialNo,config.g_devinfo.ipc_id,config.g_devsta.status_online,config.g_devsta.status_ok,config.g_devinfo.device_encode, \
        config.g_devinfo.device_record,config.g_devsta.status_time );
    eXosip_lock(mSipCtx);
    eXosip_message_build_request(mSipCtx, &msg, "MESSAGE", to, from, NULL);
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(mSipCtx, msg);
    eXosip_unlock(mSipCtx);
    char *s;
    size_t len;
    osip_message_to_str(msg, &s, &len);
    LOGI("send cmd catalog: \n%s", s);
    osip_free(s);
    return 0;
}


/*****************************************************************************
//	描述：	处理message 请求
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::response_message(eXosip_event_t *evtp)
{
    osip_body_t* req_body = NULL;
    osip_message_get_body(evtp->request, 0, &req_body);
    LOGI("req_body->body: \n%s", req_body->body);

    char cmd[64] = { 0 };
    parse_xml(req_body->body, "<CmdType>", false, "</CmdType>", false, cmd);
    LOGI("got message: %s", cmd);

    int ret;
    char SerialNo[20] = { 0 }; // 序列号
    int serialNo = 0;// 序列号
    char DeviceID[100] = { 0 }; // 设备编码
    // char DecoderChannelID[100] = { 0 }; // 解码器通道编码
    // char PlayUrl[512] = { 0 }; // 源视频地址
    char StartTime[32] = { 0 };
    char EndTime[32] = { 0 };

    parse_xml(req_body->body, "<SN>", false, "</SN>", false, SerialNo);
    parse_xml(req_body->body, "<DeviceID>", false, "</DeviceID>", false, DeviceID);
    // parse_xml(req_body->body, "<DecoderChannelID>", false, "</DecoderChannelID>", false, DecoderChannelID);
    // parse_xml(req_body->body, "<PlayUrl>", false, "</PlayUrl>", false, PlayUrl);
    parse_xml(req_body->body, "<StartTime>", false, "</StartTime>", false, StartTime);
    parse_xml(req_body->body, "<EndTime>", false, "</EndTime>", false, EndTime);

    LOGI("DeviceID:%s", DeviceID);
    serialNo = atoi(SerialNo);
    LOGI("SN:%d", serialNo);

    if (!strcmp(cmd, "Catalog")) {
        response_message_answer(evtp, 200);
        ret = request_message_catalog(serialNo);
    }
    else if (!strcmp(cmd, "DeviceInfo")) {
        response_message_answer(evtp, 200);
        ret = request_message_deviceinfo(serialNo);
    }
    else if(!strcmp(cmd, "RecordInfo")) {
        response_message_answer(evtp, 200);
        ret = request_message_recordInfo(serialNo, DeviceID, StartTime, EndTime);
    }
    else if (!strcmp(cmd, "DeviceStatus")) {
        response_message_answer(evtp, 200);
        ret = request_message_devicestatus(serialNo);
    }
    else {//if (!strcmp(cmd, "DeviceControl")) {
        // response_message_answer(evtp, 200);
        ret = -1;
    }
    if(ret < 0)
    {
        response_message_answer(evtp, 400);
    }
    return 0;
}

/*****************************************************************************
//	描述：	处理呼叫请求
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::response_invite(eXosip_event_t *evtp)
{
        char* username = evtp->request->to->url->username;//对应摄像头的DeviceID
        char* CallID = evtp->request->call_id->number;//呼叫id
        LOGI("username:%s", username);
        LOGI("CallID:%s", CallID);

        int ret;
        char *token = NULL;
        osip_message_t* answer = NULL;
        eXosip_lock(mSipCtx);//上锁

        // Ringing
        eXosip_call_send_answer(mSipCtx, evtp->tid, 180, NULL);//发送180振动响应
        ret = eXosip_call_build_answer(mSipCtx, evtp->tid, 200, &answer);//构建200 报文响应
        if (ret != 0) {
            eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);//构建200 响应失败 则发送400报文响应
            LOGE("camera: %s eXosip_call_build_answer error", username);
        }
        else {
            osip_body_t* req_body = NULL;
            osip_message_get_body(evtp->request, 0, &req_body);//获取消息体
            if(req_body != NULL)
                token = strtok(req_body->body,"\n");//获取第一段数据
            SdpTrack m_sdptrack;
            if(config.ParseSDPInfo(token, &m_sdptrack) < 0)//解析sdp数据信息
            {
                eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
                goto exit;
            }
            int chn = config.FindChannelByDeviceId(username);//获取到的通道id
            if(chn < 0)
            {
                LOGE("channel list not find the device id %s\n", username);
                eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
                goto exit;
            }
            config.UpdateRtpSendCfgBySDPInfo(chn,m_sdptrack);// 根据SDP信息更新RTP发送配置
            //还有一点需要注意，这个设置：a=sendonly
            char sdpBuf[1024];
            snprintf(
                sdpBuf, sizeof(sdpBuf),
                "v=0\r\n"
                "o=%s 0 0 IN IP4 %s\r\n"
                "s=%s\r\n"
                "c=IN IP4 %s\r\n"
                "t=%ld %ld\r\n"
                "m=video %d %s 96 98 97\r\n"
                "a=sendonly\r\n"
                "a=rtpmap:96 PS/90000\r\n"
                "a=rtpmap:98 H264/90000\r\n"
                "a=rptmap:97 MPEG4/90000\r\n"
                "y=%010d\r\n",
                config.g_devinfo.device_id[chn],config.g_devinfo.ipc_ip, config.g_rtpsendPs[chn].sessionName,
                config.g_devinfo.ipc_ip,config.g_rtpsendPs[chn].startTime,config.g_rtpsendPs[chn].endTime,
                config.g_rtpsendPs[chn].rtpLocalPort,config.g_rtpsendPs[chn].RtpProtocol,config.g_rtpsendPs[chn].u32Ssrc);

            // LOGD("sdpbuf\n: %s\n",sdpBuf);
            osip_message_set_body(answer, sdpBuf, strlen(sdpBuf));
            osip_message_set_content_type(answer, "application/sdp");
            eXosip_call_send_answer(mSipCtx, evtp->tid, 200, answer);
        }
    exit:
        eXosip_unlock(mSipCtx);
        LOGD("INVITE IPC IP ADDR : %s\n",config.g_devinfo.ipc_ip);
        return 0;
}

/*****************************************************************************
//	描述：	接收到ack请求
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
    int channel = config.FindChannelByDeviceId(username);
    if(channel < 0)
    {
        LOGE("channel list not find the device id %s\n", username);
        eXosip_call_send_answer(mSipCtx, evtp->tid, 400, NULL);
        return -1;
    }
    strcpy(config.g_rtpsendPs[channel].callidNumber,evtp->request->call_id->number);
    strcpy(config.g_rtpsendPs[channel].callidHost,evtp->request->call_id->host);
    // 正式开始推流
    if(!config.g_rtpsendPs[channel].mQuit){
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
//	描述：	接收到bye请求
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
    int channel = config.FindChannelByDeviceId(username);
    if(channel < 0)
    {
        LOGE("channel list not find the device id %s\n", username);
        return -1;
    }
    // 停止推流
    if(!config.g_rtpsendPs[channel].mQuit){
        rtpsendPs_stop(channel);
    }
    return 0;
}

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
//	描述：	事件处理
//	输入参数：
//evtp 事件结构体
*****************************************************************************/
int SipClient::sip_event_handle(eXosip_event_t *evtp)
{
    switch (evtp->type) {
    case EXOSIP_REGISTRATION_SUCCESS://0注册成功
        LOGI("Process Event EXOSIP_REGISTRATION_SUCCESS\tinfo: user is successfully registred");
        config.gclntsta.mRegistered = true;
        LOGI("mRegistered=%d,mRegisterId=%d", config.gclntsta.mRegistered, config.gclntsta.mRegisterId);
        //request_message_keepalive();
        break;

    case EXOSIP_REGISTRATION_FAILURE://1 注册失败
        LOGI("Process Event EXOSIP_REGISTRATION_FAILURE\tinfo: user is not registred");
        LOGI("mRegistered=%d,mRegisterId=%d", config.gclntsta.mRegistered, config.gclntsta.mRegisterId);
        config.gclntsta.mRegistered = false;

        if (eXosip_add_authentication_info(mSipCtx, config.g_devinfo.ipc_usrname, config.g_devinfo.ipc_id, config.g_devinfo.ipc_pwd, NULL, NULL) < 0) {
            LOGI("eXosip_add_authentication_info error");
        }
        break;

    case EXOSIP_CALL_INVITE://2 收到呼叫邀请
        LOGI("Process Event EXOSIP_CALL_INVITE\tinfo: announce a new call");
        response_invite(evtp); //处理呼叫邀请
        break;

    case EXOSIP_CALL_NOANSWER://4 超时无应答
        LOGI("Process Event EXOSIP_IN_SUBSCRIPTION_NEW\tinfo: announce no answer within the timeout");
        break;

    case EXOSIP_CALL_ANSWERED://7 呼叫应答
        LOGI("Process Event EXOSIP_CALL_ANSWERED\tinfo: announce start of call");
        LOGE("type=%d:这里应该主动发送ACK之后的回复", evtp->type);
        dump_request(evtp);//打印请求和响应信息
        dump_response(evtp);
        break;

    case EXOSIP_CALL_ACK://12 收到ack应答
        LOGI("Process Event EXOSIP_CALL_ACK\tinfo: ACK received for 200ok to INVITE");
        dump_request(evtp);
        response_ack(evtp);//处理接收到的ack信息
        break;

    case EXOSIP_CALL_MESSAGE_NEW://14 处理新的请求
        LOGI("Process Event EXOSIP_CALL_MESSAGE_NEW\tinfo: announce new incoming request");
        dump_request(evtp);
        response_playback_control(evtp);
        dump_response(evtp);
        break;

    case EXOSIP_CALL_CLOSED://21 处理关闭
        LOGI("Process Event EXOSIP_CALL_CLOSED\tinfo: a BYE was received for this call");
        dump_request(evtp);
        dump_response(evtp);
        response_bye(evtp);//处理bye请求
        break;

    case EXOSIP_CALL_RELEASED://22
        LOGI("Process Event EXOSIP_CALL_RELEASED\tinfo: call context is cleared");
        break;

    case EXOSIP_MESSAGE_NEW://23 收到 MESSAGE 请求
        LOGI("Process Event EXOSIP_MESSAGE_NEW\tinfo: announce new incoming request");
        if (MSG_IS_REGISTER(evtp->request)) {
            LOGI("MSG_IS_REGISTER，不应该出现的响应，请排查问题");
        }
        else if (MSG_IS_MESSAGE(evtp->request)) {
            response_message(evtp);//处理 MESSAGE 请求
        }
        else {
            LOGI("未定义类型的MESSAGE");

            dump_request(evtp);

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
        }
        break;

    case EXOSIP_MESSAGE_ANSWERED://25 收到 200 ok 响应
        LOGI("Process Event EXOSIP_MESSAGE_ANSWERED\tinfo: announce a 200ok");
        break;

    case EXOSIP_MESSAGE_REQUESTFAILURE://27 处理请求失败问题
        LOGI("Process Event EXOSIP_MESSAGE_REQUESTFAILURE\tinfo: announce a failure");
        LOGE("evtp->textinfo= '%s' ", evtp->textinfo);
        if (evtp->ack) {
            char* ack_str;
            size_t ack_str_len;
            osip_message_to_str(evtp->ack, &ack_str, &ack_str_len);
            LOGE("ack_str=%s", ack_str);
            osip_free(ack_str);
        }
        dump_request(evtp);
        dump_response(evtp);
        break;

    case EXOSIP_IN_SUBSCRIPTION_NEW://处理新的订阅请求
        LOGI("Process Event EXOSIP_IN_SUBSCRIPTION_NEW\tinfo: announce new incoming SUBSCRIBE/REFER");
        dump_request(evtp);
        response_message(evtp);
        break;

    default: //未知事件
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
    addr_serv.sin_addr.s_addr = inet_addr(config.g_rtpsendPs[chn].rtpServerIp);
    addr_serv.sin_port = htons(config.g_rtpsendPs[chn].rtpServerPort);
    len = sizeof(addr_serv);
    int ret = -1;
    if(config.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_UDP) {
        ret = sendto(config.g_rtpsendPs[chn].mSockFd, buff, size, 0, (struct sockaddr*)&addr_serv, len);  //send函数专用于TCP链接，sendto函数专用与UDP连接。
    }
    else if(config.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_TCP) {
        ret = send(config.g_rtpsendPs[chn].mSockFd, buff, size, 0);  //send函数专用于TCP链接，sendto函数专用与UDP连接。
    }
    else {
        LOGE("sdp transmit protocol set error,sdpProtocol: %d",config.g_rtpsendPs[chn].sdpProtocol);
    }
    if (ret <= 0) {
        LOGE("%s sendto port %d ret=%d",config.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_UDP ? "UDP" : "TCP",config.g_rtpsendPs[chn].rtpServerPort,ret);
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


/***
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
    //if (pPacker->IFrame == 1)
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
    DIR *dir;
    struct stat st;
    struct tm time_info;
    struct dirent *entry;
    time_t start_time, end_time;

    int  *channel = (int *)arg;

    pPacker.channel = *channel;
    pPacker.IFrame = 1;
    pPacker.u32Ssrc = config.g_rtpsendPs[pPacker.channel].u32Ssrc;
    pPacker.s64CurPts = 0;

    dirpath = config.g_rtpsendPs[pPacker.channel].recordDir;
    if (!(dir = opendir(dirpath))) {
        fprintf(stderr, "Cannot open directory: %s\n", dirpath);
        return 0;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        if(config.ParseFileNameTime(entry->d_name, &start_time) < 0)
        {
            LOGE("parse file name time err\n");
            continue;
        }
        sprintf(filepath, "%s/%s", dirpath, entry->d_name);
        int duration = config.GetStreamDuration(filepath);
        if(duration < 0)
        {
            LOGE("parse stream duration err\tduration: %d\n",duration);
            continue;
        }
        end_time = start_time+duration;
        if(config.g_rtpsendPs[pPacker.channel].startTime < end_time && config.g_rtpsendPs[pPacker.channel].endTime > start_time)
        {
            IsFindFile = 1;
            break;
        }
    }
    closedir(dir);
    if(IsFindFile == 0)
    {
        LOGE("not find file startTime and endTime same need\n");
        return ((void *)-1);
    }
    elapsedTime =(config.g_rtpsendPs[pPacker.channel].startTime - start_time)*25;
    elapsedTime=elapsedTime > 0 ? elapsedTime : 0;
    config.g_rtpsendPs[pPacker.channel].fp = fopen(filepath, "rb");
    if(config.g_rtpsendPs[pPacker.channel].fp == NULL){
        LOGE("fopen error:%s\n",filepath);
        return ((void *)-1);
    }
    unsigned char * StreamPack = (unsigned char*)malloc(sizeof(unsigned char) * config.input_size.height * config.input_size.width);

    memset(StreamPack, 0x00, sizeof(unsigned char) * config.input_size.height * config.input_size.width);
    pthread_mutex_lock(&config.g_rtpsendPs[pPacker.channel].mutex);
    config.g_rtpsendPs[pPacker.channel].mQuit = false;
    pthread_mutex_unlock(&config.g_rtpsendPs[pPacker.channel].mutex);
    config.g_rtpsendPs[pPacker.channel].mPlay = true;
    while (!config.g_rtpsendPs[pPacker.channel].mQuit) {
        while (!config.g_rtpsendPs[pPacker.channel].mPlay) {
            usleep(100*1000);
        }
        int size = Config::getNextNalu(config.g_rtpsendPs[pPacker.channel].fp, (unsigned char*)(StreamPack + PES_HDR_LEN)); //PES_HDR_LEN 19   size：发送数据的长度
        if (size <= 0) {
            LOGI("Send data completed, active exit\n");
            request_media_status(pPacker.channel);
            break;
        }
        if(Config::getNalupos2((unsigned char*)(StreamPack + PES_HDR_LEN), size, &prefixLen, &naluType) < 0)
        {
            continue;
        }
        if(elapsedTime > 0)
        {
            if((naluType&0x1F) == 0x01 || (naluType&0x1F) == 0x05)
            {
                elapsedTime--;
            }
            continue;
        }
        // 将h264码流读取到的一个一个nalu封装到ps并通过rtp推流
        gb28181_streampackageForH264((char *)StreamPack + PES_HDR_LEN, size, &pPacker, 0); //0 表示传递的是视频数据
        if((naluType&0x1F) == 0x01 || (naluType&0x1F) == 0x05)
        {
            pPacker.s64CurPts += 90000/config.g_rtpsendPs[pPacker.channel].fps/config.g_rtpsendPs[pPacker.channel].scale;
            Frame_interval=(int)(float)(1000/config.g_rtpsendPs[pPacker.channel].fps/config.g_rtpsendPs[pPacker.channel].scale);
            usleep((Frame_interval) * 1000);//函数的休眠单位是微秒，这里休眠40毫秒
        }
    }
    config.g_rtpsendPs[pPacker.channel].mPlay = false;
    pthread_mutex_lock(&config.g_rtpsendPs[pPacker.channel].mutex);
    config.g_rtpsendPs[pPacker.channel].mQuit = true;
    pthread_mutex_unlock(&config.g_rtpsendPs[pPacker.channel].mutex);
    free(StreamPack);
    close(config.g_rtpsendPs[pPacker.channel].mSockFd);
    fclose(config.g_rtpsendPs[pPacker.channel].fp);
    config.g_rtpsendPs[pPacker.channel].mSockFd = 0;
    config.g_rtpsendPs[pPacker.channel].fp = NULL;
    pthread_exit((void *)0);
}

void *SipClient::SendDataThread(void *arg)
{
    int ringbuflen = 0;
    struct ringbuf ringfifo;
    struct Data_Info_s pPacker;

    int  *channel = (int *)arg;

    pPacker.channel = *channel;
    pPacker.IFrame = 1;
    pPacker.u32Ssrc = config.g_rtpsendPs[pPacker.channel].u32Ssrc;
    pPacker.s64CurPts = 0;

    // char h264_file_name[64] = {0};
    // sprintf(h264_file_name, "/home/test_save_vodeo%d.h264", pPacker.channel);
    // FILE *file_h264 = fopen(h264_file_name,"w");
    unsigned char * StreamPack = (unsigned char*)malloc(sizeof(unsigned char) * config.input_size.height * config.input_size.width);
    memset(StreamPack, 0x00, sizeof(unsigned char) * config.input_size.height * config.input_size.width);
    Config::ringmalloc_fifochn(pPacker.channel+REALTIME_FIFO_OFFSET,config.ringsize.height*config.ringsize.width);
    pthread_mutex_lock(&config.g_rtpsendPs[pPacker.channel].mutex);
    config.g_rtpsendPs[pPacker.channel].mQuit = false;
    config.g_rtpsendPs[pPacker.channel].usefifo = true;
    pthread_mutex_unlock(&config.g_rtpsendPs[pPacker.channel].mutex);
    config.g_rtpsendPs[pPacker.channel].scale = 1;
    while (!config.g_rtpsendPs[pPacker.channel].mQuit) {
        ringbuflen = Config::ringget_fifochn(pPacker.channel+REALTIME_FIFO_OFFSET,&ringfifo);
        if(ringbuflen !=0)
		{
            //printf("Warningsjfowejfowgw: channel\n");
            // LOGD("gb client get video[%d]  data \n",pPacker.channel);
            // fwrite(ringfifo.buffer, ringfifo.size, 1, file_h264);
            // 将h264码流读取到的一个一个nalu封装到ps并通过rtp推流
            memcpy(StreamPack + PES_HDR_LEN, ringfifo.buffer, ringfifo.size);
            gb28181_streampackageForH264((char *)StreamPack + PES_HDR_LEN, ringfifo.size, &pPacker, 0); //0 表示传递的是视频数据
            pPacker.s64CurPts += 90000/config.g_rtpsendPs[pPacker.channel].fps/config.g_rtpsendPs[pPacker.channel].scale;
		}
        else {
            usleep(100*1000);
        }
    }
    pthread_mutex_lock(&config.g_rtpsendPs[pPacker.channel].mutex);
    config.g_rtpsendPs[pPacker.channel].mQuit = true;
    config.g_rtpsendPs[pPacker.channel].usefifo = false;
    pthread_mutex_unlock(&config.g_rtpsendPs[pPacker.channel].mutex);
    free(StreamPack);
    // fclose(file_h264);
    Config::ringfree_fifochn(pPacker.channel+REALTIME_FIFO_OFFSET);
    pthread_exit((void *)0);
}

int SipClient::rtpsendPs_init(int chn)
{
    struct sockaddr_in clientaddr,serveraddr;

    // 创建TCP/UDP套接字
    if(config.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_UDP) {
        if ((config.g_rtpsendPs[chn].mSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            perror("socket creation failed");
            LOGE("创建套接字失败");
            return -1;
        }
    }
    else if(config.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_TCP) {
        if ((config.g_rtpsendPs[chn].mSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
            perror("socket creation failed");
            LOGE("创建套接字失败");
            return -1;
        }
    }

    // 设置SO_REUSEADDR套接字选项
    int optval = 1;
    if (setsockopt(config.g_rtpsendPs[chn].mSockFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt failed");
        close(config.g_rtpsendPs[chn].mSockFd);
        LOGE("setsockopt failed");
        return -1;
    }
    
    // 设置本地发送端口
    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = inet_addr(g_rtpsendPs.rtplocalIp);
    clientaddr.sin_addr.s_addr = INADDR_ANY;
    clientaddr.sin_port = htons(config.g_rtpsendPs[chn].rtpLocalPort);

   // 绑定本地地址和端口
    if (bind(config.g_rtpsendPs[chn].mSockFd, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
        perror("bind failed");
        close(config.g_rtpsendPs[chn].mSockFd);
        LOGE("绑定套接字失败 mSockFd %d",config.g_rtpsendPs[chn].mSockFd);
        return -1;
    }

    if(config.g_rtpsendPs[chn].sdpProtocol == GB28181_TRANS_TYPE_TCP) {
        // 设置服务器地址和端口
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        inet_pton(AF_INET, config.g_rtpsendPs[chn].rtpServerIp, &serveraddr.sin_addr);
        serveraddr.sin_port = htons(config.g_rtpsendPs[chn].rtpServerPort);

        // 连接到服务器
        if (connect(config.g_rtpsendPs[chn].mSockFd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
        {
            perror("connection err\r\n");
            close(config.g_rtpsendPs[chn].mSockFd);
            LOGE("connection err\r\n");
            return -1;
        }
    }
    //设置为非阻塞模式
    int ul = 1;
    if(ioctl(config.g_rtpsendPs[chn].mSockFd, FIONBIO, &ul) == -1)
    {
        perror("ioctl err\r\n");
        close(config.g_rtpsendPs[chn].mSockFd);
        LOGE("设置套接字非阻塞失败");
        return -1;
    }
    return 0;
}


/*****************************************************************************
//	描述：	停止推流
//	输入参数：
//chn 通道
*****************************************************************************/
int SipClient::rtpsendPs_stop(int chn)
{
    pthread_mutex_lock(&config.g_rtpsendPs[chn].mutex);
    config.g_rtpsendPs[chn].mQuit = true;
    config.g_rtpsendPs[chn].usefifo = false;
    pthread_mutex_unlock(&config.g_rtpsendPs[chn].mutex);
    config.g_rtpsendPs[chn].mPlay = false;
    config.g_rtpsendPs[chn].scale = 1;
    //pthread_cancel(config.g_rtpsendPs[chn].pid);
    //pthread_join(config.g_rtpsendPs[chn].pid,NULL);
    config.g_rtpsendPs[chn].pid_thread.join();
    if(config.g_rtpsendPs[chn].mSockFd)
        close(config.g_rtpsendPs[chn].mSockFd);
    if(config.g_rtpsendPs[chn].fp)
        fclose(config.g_rtpsendPs[chn].fp);
    //config.g_rtpsendPs[chn].pid = 0;
    config.g_rtpsendPs[chn].mSockFd = 0;
    config.g_rtpsendPs[chn].fp = NULL;
    Config::ringfree_fifochn(chn+REALTIME_FIFO_OFFSET);
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
    int *channel = &config.g_rtpsendPs[chn].channel;
    if(config.g_rtpsendPs[chn].mQuit){

        if(!strcmp(config.g_rtpsendPs[chn].sessionName,"Play"))//实时视频
        {
            //printf("sdjfowesdfwerefsfd\n");
            //ret = pthread_create(&config.g_rtpsendPs[chn].pid, NULL, SendDataThread, (void *)channel);
            config.g_rtpsendPs[chn].pid_thread = std::thread([this, channel]() {
                SendDataThread((void *)channel);//
            });
            
        }
        else if(!strcmp(config.g_rtpsendPs[chn].sessionName,"Playback"))//设备录像
        {
            //ret = pthread_create(&config.g_rtpsendPs[chn].pid, NULL, SendFileDataThread, (void *)channel);
            config.g_rtpsendPs[chn].pid_thread = std::thread([this, channel]() {
                SendFileDataThread((void *)channel);
            });
        }
        else if(!strcmp(config.g_rtpsendPs[chn].sessionName,"Download"))//云端下载
        {
            //ret = pthread_create(&config.g_rtpsendPs[chn].pid, NULL, SendFileDataThread, (void *)channel);
            config.g_rtpsendPs[chn].pid_thread = std::thread([this, channel]() {
                SendFileDataThread((void *)channel);
            });
        }
        else
        {
            LOGE("%s Operation not supported\n",config.g_rtpsendPs[chn].sessionName);
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
