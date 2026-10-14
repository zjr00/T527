#ifndef __SIPCONFIG_H__
#define __SIPCONFIG_H__

#include <stdlib.h> //标准函数库定义
#include <fcntl.h> //文件控制定义
#include <termios.h> //POSIX终端控制定义
#include <string>
#include <vector>
#include "config.h"
#include <netinet/in.h>


#define PS_HDR_LEN  14 // ps header 字节长度
#define SYS_HDR_LEN 18 // ps system header 字节长度
#define PSM_HDR_LEN 24 // ps system map    字节长度
#define PES_HDR_LEN 19 // ps pes header    字节长度
#define RTP_HDR_LEN 12 // rtp header       字节长度
#define RTP_VERSION 2  // rtp 版本号
#define RTP_MAX_PACKET_BUFF 1460 // rtp传输时的最大包长
#define PS_PES_PAYLOAD_SIZE 65522 // 分片进循发送的最大长度上限

#define DefaultTimeFormat       "%Y-%m-%dT%H-%M-%S"
#define RecordInfoTimeFormat    "%Y-%m-%dT%H:%M:%S"	//StartTime = 2025-10-01T00:00:00    EndTime = 2025-10-01T23:59:59
#define RecordFileTimeFormat    "%Y-%m-%dT%H-%M-%S"
#define TIMESTRING	"YYYY-MM-DDTHH-MM-SS"
#define MAX_CHN     10
#define REALTIME_FIFO_OFFSET 0
#define AUDIO_CODEC_PCMA 1

typedef int (*StreamControlFunc)(int, bool);


enum{
	GB28181_TRANS_TYPE_UDP  = 0,
	GB28181_TRANS_TYPE_TCP,
};



typedef struct _device_status/*设备状态结构体*/
{
	char status_on[0x20];/*设备打开状态*//*取值范围：ON/OFF*//*默认值: ON*/
	char status_ok[0x20];/*是否正常工作*//*取值范围：OK/ERROR*//*默认值: OK*/
	char status_online[0x20];/*是否在线*//*取值范围：ONLINE/OFFLINE*//*默认值: ONLINE*/
	char status_guard[0x20];/*布防状态*//*取值范围：ONDUTY/OFFDUTY/ALARM*//*默认值: OFFDUTY*/
	char status_time[0x20];/*设备日期和时间*//*格式：xxxx-xx-xxTxx:xx:xx*//*默认值: 2012-12-18T16:23:32*/
}GbDevStatus;



typedef struct _sdp_track {
    int  port;
	int  protocol;
	int  codeStream;
	int  hasAudio;
	int  audioPort;
	int  audioCodec;
    float scale;
	time_t startTime;
	time_t endTime;
	uint32_t ssrc;
	char sdpVersion[0x10];
    char userName[0x10];
    char sessionId[0x10];
    char sessionVersion[0x10];
    char netWorkType[0x10];
    char addressType[0x10];
    char address[0x10];
	char sessionName[0x10];
	char mediaType[0x10];
	char fmtList[0x10];
	char transportProtocol[0x10];
}SdpTrack;

struct Data_Info_s {
	int channel;
    int IFrame;
    uint64_t s64CurPts;
    uint16_t u16CSeq;
    uint32_t u32Ssrc;
    char szBuff[RTP_MAX_PACKET_BUFF];
};

struct bits_buffer_s {
    unsigned char* p_data;
    unsigned char  i_mask;
    int i_size;
    int i_data;
};

typedef struct _stream_resolution {
	int width;
	int height;
}StreamSize;

typedef struct _sipclient_status {
	bool allowReg;/*是否允许注册*//*默认值：true*/
	bool quit;/*是否退出SIP协议*//*默认值：false*/
    bool mRegistered;/*是否已注册*//*默认值：false*/
    int  mRegisterId;/*注册ID/用来更新注册或取消注册*/
	int  errorCountReg;/*注册失败次数*//*默认值：0*/
	uint32_t sn;/*Command Sequence Number  seq or sn*/ /*默认值：0*/
	pthread_t pid;
    thread threads;
} SipClientStatus;



typedef struct _rtpsend_info {
	int fps;/*FPS*//*默认值: 25*/
	int pyloadType; /* H264 or H265 */
	int hasAudio;
	FILE *fp;/*指向历史回放流文件的文件指针*//*默认值: NULL*/
	bool mQuit;/*退流状态*//*默认值: true*/
	bool mPlay;/*播放控制*//*默认值: false*/
	bool mRun;/*线程状态*/
	void *buffer;
	float scale;/*播放速度*//*默认值: 1*/
	int channel;/*通道号*//*默认值: 0/1/2/3/4/5*/
	bool usefifo;/*环形队列使用*//*默认值: false*/

	int mSockFd;/*SDP网络句柄*//*默认值: 0*/
	//pthread_t pid;/*推流线程号*//*默认值: 0*/
    thread pid_thread;/*推流线程号*///
	time_t endTime;/*会话结束时间*//*默认值: 0*/
	time_t startTime;/*会话开始时间*//*默认值: 0*/
	uint32_t u32Ssrc;/*SSRC*//*默认值: 0200000002*/
	int sdpProtocol;/*SDP流传输模式*//*默认值: UDP*/
	int rtpLocalPort;/*SDP本地端口*//*默认值: 30000*/
	int rtpServerPort;/*SDP服务端口*//*默认值: 50000*/
	char recordDir[0x40];/*通道录像存储目录*//*默认值: device_dir/video0/1/2/3/4/5*/
	char sessionName[0x10];
    char rtpServerIp[0x10];
	char rtplocalIp[0x10];
	char rtpProtocol[0x10];
	char callidNumber[0x40];
	char callidHost[0x20];
}RtpSendPs;


typedef struct _sip_info {/*设备信息结构体*/
	int timeZone;/*时区*//*默认值：8*/
	int alertId;
	int codeStream;/*主/子码流*//*0:主*/
	int sipTimeout;/*SIP timeout*/
	int regExpires;/*注册到期时间 单位:s*//*默认值: 3600*/
	int heartBeatCount; /*心跳次数*//*容忍的连续超时次数‌*/
	int heartBeatInterval;/*心跳周期 单位:s*//*默认值: 30*/
	int sipServerPort;/*SIP服务器IP端口*//*默认值: 5060*/
	int sipRtpServerPort;/*SIP服务器RTP接收端口*//*默认值: 50000*/
	int sipLoaclPort;/*SIP客户端IP端口*//*默认值: 5060*/
	int sipRtpLoaclPort;/*SIP客户端RTP发送端口*//*默认值: 30000*/
	int sipProtocol;/*SIP信令传输模式*//*默认值: UDP*/
	int sdpProtocol;/*SDP流传输模式*//*默认值: UDP*/
	float loadSpeed;/*DownloadSpeed*/
	char resolution[0x20];/*9/16*/
	char realm[0x20];/*默认值: 3402000000*/
	char nonce[0x20];/*SIP服务随机数值 默认值： 1234567890123456*/
	char serverId[0x20];/*SIP服务器ID*//*默认值: 34020000002000000001*/
	char serverIp[0x20];/*SIP服务器IP地址*//*默认值: 192.168.10.1*/

	char userAgent[0x20];/*user agent*//*默认值: camera*/
	char userid[0x20];/*媒体流发送者ID*//*默认值: 34020000001320000001*/
	char deviceIp[0x20];/*媒体流发送者IP地址*//*默认值: 192.168.1.1*/
	char deviceName[0x20];/*IPC 名称*//*默认值: HiPC*/
	char username[0x20];/*用户名*//*默认值: 34020000001320000001*/
	char passwd[0x20];/*媒体流发送者密码*//*默认值: 12345678*/
	
	char** channelId;//channelId[MAX_CHN][0x20];/*通道ID*//*默认值: 0/1/2/3/4/5*/
    char** channelName;//char channelName[MAX_CHN][0x20];/*设备/区域/系统名称*//*默认值: IPC0/1/2/3/4/5*/
	char manufacturer[0x20];/*设备厂商*//*默认值: Hikvision*/
	char deviceModel[0x20];/*设备型号*//*默认值: YF1109*/
	char deviceFirmware[0x20];/*设备固件版本*//*默认值: V0.0.1*/
	char deviceEncode[0x20];/*是否编码*//*取值范围：ON/OFF*//*默认值: OFF*/
	char deviceRecord[0x20];/*是否录像*//*取值范围：ON/OFF*//*默认值: ON*/
} SipInfo_t;


enum {
	GB_H264 = 0,	/**< H264视频协议类型 */
	GB_H265,		/**< H265视频协议类型 */
};

typedef struct {
    int status; //开关状态 off - 关闭，on- 开启
    int regExpires; //注册超时时间（秒）
    int heartBeatInterval; //心跳间隔
	int codeStream; //码流类型 0-主 ，1-副
    int sipLoaclPort; //SIP 本地端口
    int sipServerPort;//sip服务器端口
    int encode;//编码类型选择 ，disable - 禁用编码，enable- 启用编码
    int alertId; //报警ID
    char *serverIp; //服务器ip
    char *domain; //SIP域名称 类似 3402000000
	char *serverId;//服务器id 类似 34020000002000000001
    char *userid;//设备ID（20位数字编码）34020000001110000001
    char *passwd;//SIP认证密码
} DeviceConfig_t;



class SipConfig
{
    public:
    SipConfig() {}
    ~SipConfig() {}
    
    void register_gb_stream_controller(StreamControlFunc func);
    StreamControlFunc get_gb_stream_controller();
    int get_fifo_offset();
    int set_fifo_offset(int ofs);
    int64_t getCurTimestamp();
    int SetGBDeviceConfig(DeviceConfig_t config);
    int SetRtpStreamPyloadType(int type);
    int SetSipClientDefaultInfo();
    int SetRtpRecordFileDir(int chn, char *dir);

    int GetStreamDuration(char *StreamPath, time_t *startTime, time_t *endTime, time_t *duration);
    int ParseFileNameToTimestamp(char *path, char *timeFormat, time_t *timeStamp);
    int ParseTimstrToTimestamp(char *timeStr, char *strFormat, time_t *timeStamp);
    int ParseTimestampToTimestr(time_t timeStamp, char *strFormat, char *timeStr,size_t strSize);

    int FindChannelByDeviceId(char * deviceId);
    int ParseSDPInfo(char *sdpToken, SdpTrack *m_sdptrack);
    int UpdateRtpSendCfgBySDPInfo(int channel, SdpTrack m_sdptrack);
    char *MyStrptime(const char *buf, const char *format, struct tm *tm);
    int FindVideoByTime(char *dirName, int channel, time_t startTime, time_t endTime, int *total);
    int get_sip_command_sequence_number();
    int GetGBDeviceConfig(DeviceConfig_t *config);
	bool GetDeviceStatus(void);
    
	bool GetSipClientStatus();
	bool GetSipClientRegister();
	int GetRtpStreamFps(int *fps);
	int SetRtpStreamFps(int fps);
	int GetRtpRecordFileDir(int chn,char * dir);
	int GetRtpFifoSize(int *width, int *height);
	int SetRtpFifoSize(int width, int height);
	int GetRtpStreamResolution(int *width, int *height);
	int SetRtpStreamResolution(int width, int height);

	int SendGbStreamData(int channel, unsigned char *buffer,int size, int encode_type);
	bool GetRtpStreamStatus(int channel);

    void rtp_array_destroy(RtpSendPs** array);
    GbDevStatus g_devsta;
    static int g_max_channels;
    static int g_realtime_fifo_offset;
    StreamSize g_ringFifoSize={640,360};
    StreamSize g_streamSize={1280,720};
    SipClientStatus g_client = {.quit = true};
    RtpSendPs *g_rtpsendPs = NULL;
    SipInfo_t g_sipinfo;

private:
    static StreamControlFunc g_stream_controller;
    
    pthread_mutex_t mx_sip = PTHREAD_MUTEX_INITIALIZER;
    RtpSendPs* rtp_array_create(int channel);
    int gcd(int a, int b);
};




#endif