#ifndef __GB28181_H__
#define __GB28181_H__
#include "config.h"
#include "SipClient.h"

#ifndef USE_H265_STREAM
#define USE_H264_STREAM
#endif


#pragma pack (1)
typedef struct {
    int status;
    int regExpires;
    int heartBeatInterval;
	int codeStream;
    int sipLoaclPort;
    int sipServerPort;
    int encode;
    int alertId;
    char serverIp[0x10];
    char *domain;
    char *address;
	char *serverId;
    char *userid;
    char *passwd;
} gb_config_args;
#pragma pack ()


class gb28181
{

public:
    gb28181();
    ~gb28181();

    int gb28181_client_start(void);
    int gb28181_client_close(void);
    int get_gbclient_status(void);
    int get_gb_config(gb_config_args *args);
    // 处理接收到的数据
    void processReceivedData(int chn,unsigned char* data, size_t len);
    SipClient sipClient;
private:
    int gb28181_client_init(void);
    int read_gb_config(gb_config_args *args);
    int set_gb_config(gb_config_args args);
    
    
};


#endif