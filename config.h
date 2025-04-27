/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-06-21 11:34:55
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-02 10:59:38
 * @FilePath: \AOA\config.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE~
 */
#ifndef CONFIG_H
#define CONFIG_H
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <unordered_map>
#include "log.h"
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include "./osip/include/eXosip2/eXosip.h"
#include "./osip/include/eXosip2/eX_call.h"
#include "./osip/include/eXosip2/eX_setup.h"
#include "./osip/include/eXosip2/eX_register.h"
#include "./osip/include/eXosip2/eX_message.h"
#include "./osip/include/osipparser2/osip_list.h"
#include "./osip/include/osipparser2/osip_port.h"
#include "./osip/include/osipparser2/osip_body.h"
#include "./osip/include/osipparser2/osip_parser.h"
#include "./osip/include/osipparser2/sdp_message.h"
#include "./osip/include/osipparser2/osip_message.h"



using namespace std;
#define PS_HDR_LEN  14 // ps header 字节长度
#define SYS_HDR_LEN 18 // ps system header 字节长度
#define PSM_HDR_LEN 24 // ps system map    字节长度
#define PES_HDR_LEN 19 // ps pes header    字节长度
#define RTP_HDR_LEN 12 // rtp header       字节长度
#define RTP_VERSION 2  // rtp 版本号
#define RTP_MAX_PACKET_BUFF 1460 // rtp传输时的最大包长
#define PS_PES_PAYLOAD_SIZE 65522 // 分片进循发送的最大长度上限

//record stream file name format: dir/path/YYYY-MM-DDTHH-MM-SS.h264
#define TIMEFORMAT "%Y-%m-%dT%H-%M-%S"
#define TIMESTRING	"YYYY-MM-DDTHH-MM-SS"
#define MAX_CHN     6
#define REALTIME_FIFO_OFFSET 0

#define NCHNMAX 12
#define NMAX 32

struct ringbuf {
    unsigned char *buffer;
	int frame_type;
    int size;
};

struct ringinfo{
    int iput; /* 环形缓冲区的当前放入位置 */
    int iget; /* 缓冲区的当前取出位置 */
    int n; /* 环形缓冲区中的元素总数量 */
};

enum{
	GB28181_TRANS_TYPE_UDP  = 0,
	GB28181_TRANS_TYPE_TCP,
};

typedef struct _sipclient_status
{
	unsigned int hb_sn;//heartbeat SN /*默认值：0*/
	unsigned int alarm_sn;
	bool quit;/*是否退出SIP协议*//*默认值：false*/
    bool mRegistered;/*是否已注册*//*默认值：false*/
    int  mRegisterId;/*注册ID/用来更新注册或取消注册*/
	pthread_t pid;
	std::thread threads;
} SipClientStatus;

typedef struct _device_info/*设备信息结构体*/
{
	char server_domain[0x20];/*默认值: 3402000000*/
	char server_id[0x20];/*SIP服务器ID*//*默认值: 34020000002000000001*/
	char server_ip[0x20];/*SIP服务器IP地址*//*默认值: 192.168.10.1*/
	int server_port;/*SIP服务器IP端口*//*默认值: 5060*/
	int rtp_server_port;/*SIP服务器RTP接收端口*//*默认值: 50000*/

	char ipc_ua[0x20];/*user agent*//*默认值: camera*/
	char ipc_id[0x20];/*媒体流发送者ID*//*默认值: 34020000001320000001*/
	char ipc_ip[0x20];/*媒体流发送者IP地址*//*默认值: 192.168.1.1*/
	char ipc_name[0x20];/*IPC 名称*//*默认值: HiPC*/
	int ipc_port;/*SIP客户端IP端口*//*默认值: 5060*/
	int rtp_local_port;/*SIP客户端RTP发送端口*//*默认值: 30000*/
	char ipc_usrname[0x20];/*用户名*//*默认值: 34020000001320000001*/
	char ipc_pwd[0x20];/*媒体流发送者密码*//*默认值: 12345678*/

	int sip_protocol;/*SIP信令传输模式*//*默认值: UDP*/
	int sdp_protocol;/*SDP流传输模式*//*默认值: UDP*/
	char ipc_media_id[0x20];/*Media ID*//*默认值: 34020000001320000001*/
	char ipc_warn_id[0x20];/*Warn Id*//*默认值: 34020000001320000001*/

	int reg_expires;/*注册到期时间 单位:s*//*默认值: 3600*/
	int hb_period;/*心跳周期 单位:s*//*默认值: 30*/
	
	char device_id[MAX_CHN][0x20];/*通道ID*//*默认值: 0/1/2/3/4/5*/
	char device_name[MAX_CHN][0x20];/*设备/区域/系统名称*//*默认值: IPC0/1/2/3/4/5*/
	char device_manufacturer[0x20];/*设备厂商*//*默认值: Hikvision*/
	char device_model[0x20];/*设备型号*//*默认值: YF1109*/
	char device_firmware[0x20];/*设备固件版本*//*默认值: V0.0.1*/
	char device_encode[0x20];/*是否编码*//*取值范围：ON/OFF*//*默认值: OFF*/
	char device_record[0x20];/*是否录像*//*取值范围：ON/OFF*//*默认值: ON*/
	char device_record_dir[0x20];/*录像存储目录*//*默认值: /meida*/
} GbDevInfo;

typedef struct _device_status/*设备状态结构体*/
{
	char status_on[0x20];/*设备打开状态*//*取值范围：ON/OFF*//*默认值: ON*/
	char status_ok[0x20];/*是否正常工作*//*取值范围：OK/ERROR*//*默认值: OK*/
	char status_online[0x20];/*是否在线*//*取值范围：ONLINE/OFFLINE*//*默认值: ONLINE*/
	char status_guard[0x20];/*布防状态*//*取值范围：ONDUTY/OFFDUTY/ALARM*//*默认值: OFFDUTY*/
	char status_time[0x20];/*设备日期和时间*//*格式：xxxx-xx-xxTxx:xx:xx*//*默认值: 2012-12-18T16:23:32*/
}GbDevStatus;

typedef struct _rtpsend_info
{
	int fps;/*FPS*//*默认值: 25*/
	FILE* fp;/*指向历史回放流文件的文件指针*//*默认值: NULL*/
	bool mQuit;/*退流状态*//*默认值: true*/
	bool mPlay;/*播放控制*//*默认值: false*/
	float scale;/*播放速度*//*默认值: 1*/
	bool usefifo;/*环形队列使用*//*默认值: false*/
	int channel;/*通道号*//*默认值: 0/1/2/3/4/5*/
	int mSockFd;/*SDP网络句柄*//*默认值: 0*/
	//pthread_t pid;/*推流线程号*//*默认值: 0*/
	std::thread pid_thread;/*推流线程号*///
	time_t endTime;/*会话结束时间*//*默认值: 0*/
	time_t startTime;/*会话开始时间*//*默认值: 0*/
	uint32_t u32Ssrc;/*SSRC*//*默认值: 0200000002*/
	int sdpProtocol;/*SDP流传输模式*//*默认值: UDP*/
	int rtpLocalPort;/*SDP本地端口*//*默认值: 30000*/
	int rtpServerPort;/*SDP服务端口*//*默认值: 50000*/
	pthread_mutex_t mutex;/*推流线程互斥体*//*默认值: NULL*/
	char recordDir[0x40];/*通道录像存储目录*//*默认值: device_dir/video0/1/2/3/4/5*/
	char sessionName[0x10];
    char rtpServerIp[0x10];
	char rtplocalIp[0x10];
	char RtpProtocol[0x10];
	char callidNumber[0x40];
	char callidHost[0x20];
}RtpSendPs;

typedef struct _sdp_track {
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
    int  port;
	int  protocol;
    float scale;
	time_t startTime;
	time_t endTime;
	uint32_t ssrc;
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


class SipConfig
{
    public:
    SipConfig() {}
    ~SipConfig() {}

    
    int64_t getCurTimestamp();
    int SetSipClientDefaultInfo();
    int SetRtpRecordFileDir(char * dir);
    int GetStreamDuration(char * StreamPath);
    int FindChannelByDeviceId(char * deviceId);
    int ParseFileNameTime(char *path, time_t *ParseResult);
    int ParseSDPInfo(char *sdpToken, SdpTrack *m_sdptrack);
    int UpdateRtpSendCfgBySDPInfo(int channel, SdpTrack m_sdptrack);
    char *MyStrptime(const char *buf, const char *format, struct tm *tm);
    int FindVideoByTime(char *dirName, int channel, time_t startTime, time_t endTime, int *total);

	
	bool GetSipClientStatus();
	bool GetSipClientRegister();
	int GetRtpStreamFps(int *fps);
	int SetRtpStreamFps(int fps);
	int GetRtpRecordFileDir(char * dir);
	int GetRtpFifoSize(int *width, int *height);
	int SetRtpFifoSize(int width, int height);
	int GetRtpStreamResolution(int *width, int *height);
	int SetRtpStreamResolution(int width, int height);

	int SendStreamData(int channel, unsigned char *buffer,int size, int encode_type);

	bool GetRtpStreamStatus(int channel);

    GbDevInfo g_devinfo;
    GbDevStatus g_devsta;
    SipClientStatus gclntsta;
    RtpSendPs g_rtpsendPs[MAX_CHN];
    StreamSize ringsize={640,360};
    StreamSize input_size={1280,720};
};



struct ClientInfo {
    std::string ip_address;//ip
    std::string direction;
	std::string path;
	std::string direction_swith;
};

class Config : public SipConfig{
public:
    Config();
    ~Config();
    static string Get(const std::string& section, const std::string& key, const std::string& default_value = "");
	static bool SetToFile(const std::string& section, const std::string& key,const std::string& value, const std::string& filename = "./config.ini");
    void FileInit(string filename);

    static int GetCmdLen(unsigned char *buf, int bufLen);
    static void Show(unsigned char *buf, char *prompt);

	static int getNextNalu(FILE* inpf, unsigned char* buf);

	static int getNalupos2(uint8_t* data, int plen, int* prefix, uint8_t *nalu);

	static void ringmalloc_fifochn(int chn,int size);
	static void ringreset_fifochn(int chn);
	static void ringfree_fifochn(int chn);
	static int ringget_fifochn(int chn, struct ringbuf *getinfo);
	static int ringput_fifochn(int chn, unsigned char *buffer,int size,int encode_type);

	static uint16_t package_get_sn();
	static string GetTime_Path();

	static bool isCar;
	static std::map<int, ClientInfo> client_map;

private:
    static std::map<std::string, std::map<std::string, std::string>> sections;
	static std::vector<std::string> section_order;
	static struct ringbuf mult_ringfifo[NCHNMAX][NMAX];
	static struct ringbuf ringfifo[NMAX];
	static struct ringinfo g_ringinfo[NCHNMAX];
	static std::atomic<uint16_t> g_package_sn;
    //std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_;
    
};


#endif // CONFIG_H
