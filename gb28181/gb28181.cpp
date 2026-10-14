#include "gb28181.h"

#define GB_FIX_CONFIG_ENABLE    1
#define GB_AUTO_TEST_CONFIG     0

gb28181::gb28181()
{

}

gb28181::~gb28181()
{
    sipClient.clearParameterCallbacks();
}

int gb28181::gb28181_client_start(void)
{
    int ret = 0;
    gb_config_args args;
    gb28181_client_close();
    if(gb28181_client_init() < 0) {
        ret = -1;
        LOGE("gb28181 client init error\n");
        goto ERR_EXIT;
    }

    if(read_gb_config(&args) < 0) {
        ret = -1;
        LOGE("gb28181 client read ini config error\n");
        goto ERR_EXIT;
    }
    set_gb_config(args);
/*************************************************************************** */
/* Structure encapsulation protection configuration is required  */
    //user set
    #if(GB_FIX_CONFIG_ENABLE == 1)
    DeviceConfig_t config;
    config.status = 1;//ON
    config.codeStream = 0;//master
    config.serverIp = "192.168.10.221";
    config.sipServerPort = 5060;
    config.serverId = "44010200492000000001";
    config.domain = "4401020049";
    config.encode = 0;
    config.passwd = "admin123";
    config.regExpires = 900;
    config.heartBeatInterval= 60;
    config.userid="44010200492000000011";
    config.sipLoaclPort = 5060;
    config.alertId = 0;
    sipClient.sipConfig.SetGBDeviceConfig(config);
    #elif(GB_AUTO_TEST_CONFIG == 1)
    DeviceConfig_t config;
    config.status = 1;//ON
    config.codeStream = 0;//master
    config.serverIp = "192.168.30.172";
    config.sipServerPort = 5060;
    config.serverId = "34020000002000000001";
    config.domain = "3402000000";
    config.encode = 0;
    config.passwd = "12345678";
    config.regExpires = 900;
    config.heartBeatInterval= 60;
    config.userid="34020000001110000001";
    config.sipLoaclPort = 5060;
    config.alertId = 0;
    sipClient.sipConfig.SetGBDeviceConfig(config);
    #endif
    sipClient.sipConfig.SetRtpStreamFps(25);
#ifdef USE_H265_STREAM
    sipClient.sipConfig.SetRtpStreamPyloadType(GB_H265);//GB_H265 GB_H264
#elif defined(USE_H264_STREAM)
    sipClient.sipConfig.SetRtpStreamPyloadType(GB_H264);//GB_H265 GB_H264
#else
    // 强制要求至少选择一个编码标准
    #error "Must define either USE_H265_STREAM or USE_H264_STREAM"
    // 或设置默认值
    // GB_UNKNOWN;  
#endif
    sipClient.sipConfig.SetRtpFifoSize(1280, 720);
    sipClient.sipConfig.SetRtpStreamResolution(1920, 1080);
    // for(int i=0;i<MAX_CHN;i++) {
    //     char buff[128] = {0};
    //     if(get_recoder_path(i, buff, sizeof(buff)) < 0) {
    //         ret = -1;
    //         LOGE("gb28181 client get recorder path error\n");
    //         goto ERR_EXIT;
    //     }
    //     SetRtpRecordFileDir(i,buff);
    // }
    //register_gb_stream_controller(gb28181_set_stream_activity);//播放录像
    /****************************************************************************/

    if(get_gbclient_status()) {
        ret = sipClient.sipClient_start();
        if (ret < 0)
        {
            LOGI("1232314454\n");
            goto ERR_EXIT;
        }
        
    }
    ret = 0;
ERR_EXIT:
    // LOGI("1465641546103461035415\n");
    // if(args.serverId)
    //     free(args.serverId);
    // if(args.domain)
    //     free(args.domain);
    // if(args.passwd)
    //     free(args.passwd);
    // if(args.userid)
    //     free(args.userid);
    return ret;
}

int gb28181::gb28181_client_close(void)
{
    if(sipClient.sipConfig.GetSipClientStatus())
    {
        LOGI("run1111232133131");
        sipClient.SipClientExit();
        
    }
        
    LOGI("run gb28181 client exit");
    return 0;
}

int gb28181::get_gbclient_status(void)
{
    return sipClient.sipConfig.GetDeviceStatus();
}

int gb28181::get_gb_config(gb_config_args *args)
{
    DeviceConfig_t config;
    sipClient.sipConfig.GetGBDeviceConfig(&config);
    args->status = config.status;
    args->codeStream = config.codeStream;
    strcpy(args->serverIp,config.serverIp);
    args->sipServerPort = config.sipServerPort;
    args->serverId = config.serverId;
    args->domain = config.domain;
    args->encode = config.encode;
    args->passwd = config.passwd;
    args->regExpires = config.regExpires;
    args->heartBeatInterval = config.heartBeatInterval;
    args->userid = config.userid;
    args->sipLoaclPort = config.sipLoaclPort;
    args->alertId = config.alertId;
    return 0;
}

void gb28181::processReceivedData(int chn,unsigned char* data, size_t len)
{
    sipClient.sipConfig.SendGbStreamData(chn, data,len, 0);
}

int gb28181::gb28181_client_init(void)
{
    sipClient.set_max_channels(MAX_CHN);
    sipClient.set_fifo_offset(REALTIME_FIFO_OFFSET);
    return sipClient.sipConfig.SetSipClientDefaultInfo();
}

int gb28181::read_gb_config(gb_config_args *args)
{
    args->status = (Config::Get("GB28181","status")=="off") ? 0 : 1;
    args->codeStream =(Config::Get("GB28181","codeStream")=="sub") ? 0 : 1;

    strcpy(args->serverIp,Config::Get("GB28181","serverip").c_str());
    args->sipServerPort = stoi(Config::Get("GB28181","serverport"));

    args->serverId = (char*)Config::Get("GB28181","serverid").c_str();
    args->domain =  (char*)Config::Get("GB28181","domain").c_str();

    args->encode = (Config::Get("GB28181","encode")=="disable") ? 0 : 1;
    args->passwd = (char*) Config::Get("GB28181","password").c_str();
    args->regExpires = stoi(Config::Get("GB28181","regtimeout"));
    args->heartBeatInterval = stoi(Config::Get("GB28181","heartbeat"));
    args->userid = (char*) Config::Get("GB28181","deviceid").c_str();
    args->sipLoaclPort = stoi(Config::Get("GB28181","deviceport"));
    args->alertId =  stoi(Config::Get("GB28181","alertId"));
    return 0;
}

int gb28181::set_gb_config(gb_config_args args)
{
    DeviceConfig_t config;
    config.status = args.status;
    config.codeStream = args.codeStream;
    config.serverIp = args.serverIp;
    config.sipServerPort = args.sipServerPort;
    config.serverId = args.serverId;
    config.domain = args.domain;
    config.encode = args.encode;
    config.passwd = args.passwd;
    config.regExpires = args.regExpires;
    config.heartBeatInterval = args.heartBeatInterval;
    config.userid = args.userid;
    config.sipLoaclPort = args.sipLoaclPort;
    config.alertId = args.alertId;
    sipClient.sipConfig.SetGBDeviceConfig(config);
    return 0;
}
