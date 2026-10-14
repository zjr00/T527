#ifndef __SIPCLIENT_H__
#define __SIPCLIENT_H__


#include "Sipconfig.h"
#include <functional>



class SipClient
{
    public:
    SipClient();
    ~SipClient();


    
	// 定义回调函数类型
    using OnParameterCallback = function<void(int tslKey,int content)>;
    
    // 设置参数回调函数（可以有多个）
    void addParameterCallback(int tslKey, OnParameterCallback callback);
    
    // 清理所有回调
    void clearParameterCallbacks();

    int sipClient_start();
    int get_max_channels(void);
    int set_max_channels(int chn);
    int set_fifo_offset(int ofs);

    void test_h264();

    int gb28181_client_init(void);
    int SipClientExit();
    void *gb28181_client_func(void *arg);

    int init_sip_client();
    int parse_xml(const char* data, const char* s_mark, bool with_s_make, const char* e_mark, bool with_e_make, char* dest);
    int request_register();
    int request_message_keepalive();
    int request_media_status(int chn);
    
    
    

    void dump_request(eXosip_event_t* evt);

    void dump_response(eXosip_event_t* evt);

    int response_message_answer(eXosip_event_t* evtp, int code);
    int response_playback_control(eXosip_event_t* evtp);

   

    

    int response_message(eXosip_event_t* evtp);

    int response_invite(eXosip_event_t* evtp);

    int response_ack(eXosip_event_t* evtp);

    int response_bye(eXosip_event_t* evtp);

    int response_video_ctrl(eXosip_event_t* evtp);

    int sip_event_handle(eXosip_event_t* evtp);

    SipConfig sipConfig;
    
private:
    int response_message_catalog(eXosip_event_t *evtp, int *sn);
    void *response_message_catalog_thread(void *args);

    int response_message_deviceinfo(eXosip_event_t *evtp, int *sn);

    void *response_message_recordinfo_thread(void *args);

    int response_message_recordinfo(eXosip_event_t *evtp, int *sn);

    int response_message_devicestatus(eXosip_event_t *evtp, int *sn);
    int response_message_devicecontrol(eXosip_event_t *evtp, int *sn);
    int response_message_configdownload(eXosip_event_t *evtp, int *sn);

    int request_unregister();
    int response_subscribe(eXosip_event_t *evtp);
    int response_subscribe_alarm(eXosip_event_t *evtp, int *sn);

    static void cleanup_resources(void *arg);


    
    void bits_write(struct bits_buffer_s *buffer, int count, int bits);

    int gb28181_make_ps_header(char* pData, unsigned long long s64Scr);

    int gb28181_make_sys_header(char* pData);

    int gb28181_make_psm_header(char* pData);

    int gb28181_make_pes_header(char* pData, int stream_id, int payload_len, unsigned long long pts, unsigned long long dts);

    int gb28181_make_rtp_header(char* pData, int marker_flag, unsigned short cseq, long long curpts, unsigned int ssrc);

    int SendDataBuff(char* buff, int size, int chn);

    int gb28181_send_rtp_pack(char* databuff, int nDataLen, int mark_flag, struct Data_Info_s* pPacker);

    int gb28181_streampackageForH264(char* pData, int nFrameLen, struct Data_Info_s* pPacker, int stream_type);

    void *SendFileDataThread(void *arg);

    void *SendDataThread(void *arg);

   
    int rtpsendPs_init(int chn);

    int rtpsendPs_stop(int chn);

    int rtpsendPs_start(int chn);


    int response_invite_ack(eXosip_event_t *evtp);

    // 触发回调
    void triggerCallbacks(int tslKey, int content);

    struct eXosip_t* mSipCtx;
    unordered_map<int, vector<OnParameterCallback>> parameterCallbacks_;


   
};

struct CleanupArg {
    SipClient *client;
    int channel;
};

#endif