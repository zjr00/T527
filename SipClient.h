#ifndef __SIPCLIENT_H__
#define __SIPCLIENT_H__

#include <string>
#include <vector>
#include "config.h"
#include <netinet/in.h>



class SipClient
{
    public:
    SipClient();
    ~SipClient();

    void test_h264();

    int gb28181_client_init(void);
	int gb28181_client_start(void);
	int gb28181_client_close(void);

    int SipClientExit();
    void *gb28181_client_func(void *arg);

    int init_sip_client();
    int parse_xml(const char* data, const char* s_mark, bool with_s_make, const char* e_mark, bool with_e_make, char* dest);
    int request_register();
    int request_message_keepalive();
    int request_media_status(int chn);
    void *request_message_catalog_proc(void *args);
    int request_message_catalog(int serialNo);
    int request_message_deviceinfo(int serialNo);

    void dump_request(eXosip_event_t* evt);

    void dump_response(eXosip_event_t* evt);

    int response_message_answer(eXosip_event_t* evtp, int code);
    int response_playback_control(eXosip_event_t* evtp);

    void *request_message_recordInfo_proc(void *args);

    int request_message_recordInfo(int serialNo, char * deviceID, char * startTime, char * endTime);

    int request_message_devicestatus(int SerialNo);

    int response_message(eXosip_event_t* evtp);

    int response_invite(eXosip_event_t* evtp);

    int response_ack(eXosip_event_t* evtp);

    int response_bye(eXosip_event_t* evtp);

    int response_video_ctrl(eXosip_event_t* evtp);

    int sip_event_handle(eXosip_event_t* evtp);
    
private:
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


    Config config;
    struct eXosip_t* mSipCtx;
};
#endif