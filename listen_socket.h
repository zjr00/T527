#ifndef LISTEN_SOCKET_H_
#define LISTEN_SOCKET_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <map>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <sys/time.h>
#include <condition_variable>
#include "jt808.h"
#include "gate.h"

#include "config.h"
#include "base_json.h"
#include "RfidCar.h"
#include "Weight.h"
#include "Public.h"

#include "radar.h"
#include "./gb28181/gb28181.h"

#if TARGET_MODE == 2
#include "g2d.h"
#endif


#define MAGIC_NUMBER 0x55AA55AA
#define BUFFER_SIZE 4096
#define MAX_CLASS 68
#define MCU_PACKAGE_HEAD_FLG        0xAA55u
#define ONE_FRAME_MAX_LEN               256
#define ARRAY_BUFFER_CAPACITY           1024



struct PacketHeader {
    int type;          // 数据类型（1字节）
    uint32_t data_size;     // 数据部分大小（4字节）
    char image_name[32];   // 图片文件名（仅jpeg有效）
    char TextMesg[32];
    uint8_t picData[];
};

typedef struct cmd_package 
{
	uint16_t type;
	uint16_t cmd_data_len;
	char cmd_data[256];
}msg_cmd_package_t;



struct ntp_packet {
    uint8_t li_vn_mode;      // 8 bits: li, vn, mode
    uint8_t stratum;         // 层级
    uint8_t poll;            // 轮询间隔
    uint8_t precision;       // 精度
    uint32_t root_delay;     // 根延迟
    uint32_t root_dispersion; // 根离散
    uint32_t ref_id;         // 参考ID
    uint32_t ref_ts_sec;     // 参考时间戳秒
    uint32_t ref_ts_frac;    // 参考时间戳分数秒
    uint32_t orig_ts_sec;    // 原始时间戳秒
    uint32_t orig_ts_frac;   // 原始时间戳分数秒
    uint32_t recv_ts_sec;    // 接收时间戳秒
    uint32_t recv_ts_frac;   // 接收时间戳分数秒
    uint32_t trans_ts_sec;   // 传输时间戳秒
    uint32_t trans_ts_frac;  // 传输时间戳分数秒
};



//协议相关
typedef enum mcucom_msg_type {
    MCUCOM_MSG_UNKNOWN = 0,
    MCUCOM_MSG_HEART_BEAT, //心跳
    MCUCOM_MSG_HEART_BEAT_RESP,
    MCUCOM_MSG_MANAGE,
    MCUCOM_MSG_MANAGE_RESP,
    MCUCOM_MSG_IPC_DATA,
    MCUCOM_MSG_IPC_DATA_RESP,
    MCUCOM_MSG_OTA_DATA,
    MCUCOM_MSG_OTA_DATA_RESP,
    MCUCOM_MSG_STATE_CHANGE_HEART_BEAT,  
    MCUCOM_MSG_STATE_CHANGE_HEART_BEAT_RESP,
    MCUCOM_GPS_DATA,
    MCUCOM_GPS_DATA_RESP,
    MCUCOM_MSG_CMD,         //nrf5340   发送
    MCUCOM_MSG_CMD_RESP,    //T31   响应
    MCUCOM_MSG_MAX_COUNT
} mcucom_msg_type_t;

typedef enum msg_cmd_type {
    MSG_UNKNOWN = 0,
    MSG_REMOTE_CAPTRUE,                 //拍照指令
    MSG_REMOTE_CAPTRUE_RESP,            //拍照指令应答
    MSG_VIDEOTAPE,                      //拍视频
    MSG_VIDEOTAPE_RESP,                 //拍视频指令应答
    MSG_RETRANSMISSION_DOCUMENT,        //照片/视频文件补传指令
    MSG_RETRANSMISSION_DOCUMENT_RESP,   //照片/视频文件补传应答指令
    MSG_OPEN_RTMP_MODE,                 //开启RTMP推流指令
    MSG_OPEN_RTMP_MODE_RESP,            //开启RTMP推流指令应答
    MSG_CLOSE_RTMP_MODE,                //关闭RTMP推流指令
    MSG_CLOSE_RTMP_MODE_RESP,           //关闭RTMP推流指令应答
    MSG_CONFIG_28181,                   //配置28181参数 1 开启推流  0 关闭推流
    MSG_CONFIG_28181_RESP,              //配置28181参数应答
    MSG_DEEP_SLEEP,                     //深度休眠指令
    MSG_DEEP_SLEEP_RESP,              //深度休眠指令应答
    MSG_RESERVED_1,                     //预留指令1
    MSG_RESERVED_1_RESP,                //预留指令1应答
    MSG_RESERVED_2,                     //预留指令2
    MSG_RESERVED_2_RESP,                //预留指令2应答
    MSG_CLEAR_TFCARD_FILES,             //清除所有录像文件/照片/短视频指令
    MSG_CLEAR_TFCARD_FILES_RESP,        //所有录像文件/照片/短视频指令应答
    MSG_HEART_BEAT,                     //心跳指令
    MSG_HEART_BEAT_RESP,                //心跳指令应答
    MSG_OTA,                            //OTA指令
    MSG_OTA_RESP,                       //OTA指令应答
    MSG_VERSION,                        //版本号查询指令
    MSG_VERSION_RESP,                   //版本号查询指令应答
    MSG_SET_TIME,                       //校时指令
    MSG_SET_TIME_RESP,                  //校时指令应答
    MSG_VIDEO_ON,                       //录像开启指令
    MSG_VIDEO_ON_RESP,                  //录像开启指令应答
    MSG_VIDEO_OFF,                      //录像关闭指令
    MSG_VIDEO_OFF_RESP,                 //录像关闭指令应答
    MSG_SET_FLIP,                       //设置摄像头图像翻转
    MSG_SET_FLIP_RESP,                  //设置摄像头图像翻转应答
    MSG_SET_CAR_CODE,                   //设置OSD车牌号
    MSG_SET_CAR_CODE_RESP,              //设置OSD车牌号应答
    MSG_SET_IP,                         //设置IP地址与上网方式 192.168.30.xx,192.168.30.254,ETH,static
    MSG_SET_IP_RESP,                    //设置IP地址应答
    MSG_SET_SOCK_IP_PORT,                //设置Socket端口  192.168.30.xx,8080
    MSG_SET_SOCK_IP_PORT_RESP,           //设置Socket端口应答
    MSG_SET_COORDINATE,                   //设置坐标  x.y
    MSG_SET_COORDINATE_RESP,              //设置坐标应答
    MSG_SET_AI,                   //设置ai开启 1--开，0--关
    MSG_SET_AI_RESP,              //设置ai应答
    MAX_COUNT
} msg_cmd_type_t;


typedef struct mcucom_package_head {
    uint16_t    pack_head_flg;          //! 报文起始标志
    uint16_t    data_len;               //! 数据部分长度，只用来发送确认时长度可以为0
    uint8_t     crc;                    //! mcu_id开始的后面全部数据进行校验
    uint8_t     mcu_id;                 //! 发送方MCU_ID
    uint16_t sn;                     //! 报文流水号
    uint16_t src_sn;                 //! 源请求流水号
    uint16_t  msg_type;        //! 消息类型（管理报文、数据报文、心跳报文）
} mcucom_package_head_t;

typedef struct mcucom_package {
    mcucom_package_head_t head;    //! 报文头
    uint8_t data[0];            //! 数据
} mcucom_package_t;

typedef struct {
    uint32_t magic;         // 协议标识
    uint16_t axis;          //位置
    uint16_t file_status;   //是否发送文件status 
    uint64_t sn;            //流水号 
    uint64_t src_sn;        //src流水号 
    uint64_t file_size;     // 文件字节数 
    uint32_t crc32;         // CRC32校验值 
    uint16_t result_class;  //result_class 
    char     result[64];    //result 
    char     filename[128]; // 原始文件名
} FileHeader;


    typedef struct mcucom_msg_heart_beat {
        uint32_t mcu_io_state;
        uint16_t mcu_adc_value;
        uint16_t mcu_lock_state;    //施封/上锁状态 1 0x31
        uint8_t direction;         //摄像头方位
        uint8_t mcu_gprs_state;
        uint8_t mcu_gprs_signal;
        uint8_t mcu_ble_state;
        uint32_t tf_size_total;     //tf卡的总容量
        uint32_t tf_size_free;      //tf卡的可用容量
        uint32_t remain_file;       //未上传文件数量
        uint32_t time_s;
        uint8_t time_zone;
        uint8_t local_record_status;//保留位（用于结构体4字节对齐）
        uint8_t gb28181_status;     //gb28181 状态
        uint8_t ai_status;          //AI检测状态
        uint8_t alarm_status;       //异物入侵状态
        uint8_t system_status;      //空闲状态
        uint8_t camera_status;      //摄像头状态 1正常 0故障
        uint8_t tf_status;          //内存卡状态 
    } mcucom_msg_heart_beat_t;




typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int data_ready;
    uint8_t flag_waitdat;
    uint8_t flag_parsing;
} ParseSync_t;


typedef struct {
    uint8_t buffer[ARRAY_BUFFER_CAPACITY];
    size_t length;
} StaticArrayBuffer;


typedef struct mcucom_msg_heart_beat_resp {
    uint16_t src_sn;                        //源流水号
    uint16_t reserve;                       //保留
    mcucom_msg_heart_beat_t data;           //心跳内容
} mcucom_msg_heart_beat_resp_t;


static const  char *nna_label[MAX_CLASS] = {
    "京", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉", "黑",
    "苏", "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤",
    "桂", "琼", "川", "贵", "云", "藏", "陕", "甘", "青", "宁",
    "新",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "A", "B", "C", "D", "E", "F", "G", "H", "J", "K",
    "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V",
    "W", "X", "Y", "Z", "I", "O", "-"};



struct ClientConnection {
    string client_ip;           // 客户端IP
    int sock_msg;                    // 报文服务器socket
    int sock_file;                   // 文件服务器socket  
    int sock_video;                  // 视频服务器socket
    int unique_id;                   // 客户端唯一标识符（后续接收）
};


typedef enum {
    TOP=0,
    TOP_LEFT,      // 左上  1
    TOP_CENTER,    // 中上  2
    TOP_RIGHT,     // 右上  3
    MIDDLE_LEFT,   // 左中  4
    CENTER,        // 中心  5
    MIDDLE_RIGHT,  // 右中  6
    BOTTOM_LEFT,   // 左下  7
    BOTTOM_CENTER, // 中下  8
    BOTTOM_RIGHT,   // 右下  9
    EXTRA,        //额外的 10
} Direction;




class ListeningSocket{
public:
    ListeningSocket();
    ~ListeningSocket();


    bool Listen(int port); //设置服务器
    void Listen_Init();
    int Accept();//等待client连接

    int Getsocket() const { return server_fd; }
    int Getsocket2() const { return serverfile_fd; }
    int Getsocket_gb() const { return server_Video; }
    int GetClientSocket() const { return client_fd; }
    bool valid() const { return server_fd != -1; }


    bool Listen_2Server(int port1,int port2,int port3); 
    int Accept_client();//等待client连接
    void client_ptrhread(int sock);
    int Accept_clientfile();
    void clientfile_ptrhread(int sock);
    int  Accept_clientVideo();
    void clientVideo_ptrhread(int sock);

    void addSocket(int sock, const string& client_ip, int server_type);

    void remove_socket(int sock);

    
    vector<int> sockets;
#if TARGET_MODE == 2
    G2d g2d;
#endif
    

protected:
    void SetConfig(string config,int sock);
    int SocketBase(int sock);
    void jt808_init();
    void gb28181_init();
    int Recv_handler(int socked);//接收数据
    int Send_handler(int socked);//发送数据
    int netctrl_recv(int socked);

    int Receive_h264_video(int socked);

    
    void Xstdata();
    void Read_485();
    //void Send_handler();//发送数据
    bool receive_full(int sock, char* buffer, size_t expected_size);
    time_t get_ntp_time(const char* host);
    void check_time();
    void Restart();//重启程序
    void RestartThread(string iface);
    bool is_valid_socket(int sock);
    void parse_license_plate(char* result,int clnt_sock);   
    
    void onMachineIpReceived_callback(int tslKey, const string& content);
    void onIsOrder_callback(int tslKey, const string& content);
    void onGbChannel_callback(int tslKey, int content);
    void onGbSendStop_callback(int tslKey, int content);
    void Clear();
/////////////////////////////////////////接收630响应相关函数////////////////////////////////////

    

    int hhd_heart_beat_resp(int sock);//心跳
    void heart_handler(int sock);
    int cpature_picture();//捕获图片
    

    void serial_recv(int serial_fd,shared_ptr<ParseSync_t> ctx);
    void serial_parse(int serial_fd,shared_ptr<ParseSync_t> ctx);
    int protocol_parse(uint8_t *data,  int len,int sock);
    int get_remain_frames(uint8_t *date, int nbytes);
    int parse_msg_head(uint8_t *data,  int nbytes);
    int parse_msg_data(uint8_t *data,  int nbytes,int sock);
    int process_heart_beat_resp(uint8_t *data, int nbytes,int sock);
    size_t arraybuf_get_length(StaticArrayBuffer* buff);
    bool arraybuf_del(StaticArrayBuffer* buff, size_t n);
    bool arraybuf_add(StaticArrayBuffer* buff, const uint8_t* data, size_t len);
    size_t arraybuf_get_remain (StaticArrayBuffer* buff);

    int process_file_transfer(int clnt_sock);
    int hhd_test(int sock);
    int hhd_msg_set_command(int msg, int sock,int flag);

    int hhd_mcucom_package_send(uint8_t *buff, mcucom_msg_type_t msg_type,
                                void * send_buff, uint16_t size, mcucom_package_head_t * req_head, 
                                uint32_t timeout_ms,int sock);
    uint8_t bspal_mcucom_package_crc(mcucom_package_t * package);
    const char* get_direction_string(uint8_t direction);

    pthread_mutex_t mx_array_buffer=PTHREAD_MUTEX_INITIALIZER;
    uint16_t serial_head_flg = 0xAA55;
    //ParseSync_t parse_sync;
    StaticArrayBuffer serial_buf;
    map<int, int> m_socket_DirectionMap;

    unordered_map<int, shared_ptr<ParseSync_t>> client_contexts;
    mutex contexts_mutex;

 ////////////////////////////////////////////////////////////////////////////////////////////////
    int server_fd,client_fd,serverfile_fd , server_Video;
    struct sockaddr_in serveraddr,file_address,video_address;
    char recvBuff[1024];
    Config config;
    Jt808 jt808;
    
    base_json bj;
    mutex sockets_mutex;
    gate Gate;


    const long NTP_OFFSET = 2208988800UL; // 1900-01-01到1970-01-01的秒数差

    RfidCar rfid;
    Weight weight;
    gb28181 gb;
    
    Radar radar;
    string  tmp_path;
    vector<int> cpature_sock;//需要捕获照片

    unordered_map<string, ClientConnection> ip_to_connection;  // IP -> 连接信息
    mutex mtx;  // 线程安全锁
    bool is_gb = false;

    uint8_t g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    int chn;
    const char* NTP_SERVERS[24] = {
        "1.cn.pool.ntp.org",
        "2.cn.pool.ntp.org",
        "3.cn.pool.ntp.org",
        "0.cn.pool.ntp.org",
        "cn.pool.ntp.org",
        "tw.pool.ntp.org",
        "0.tw.pool.ntp.org",
        "1.tw.pool.ntp.org",
        "2.tw.pool.ntp.org",
        "3.tw.pool.ntp.org",
        "pool.ntp.org",
        "time.windows.com",
        "time.nist.gov",
        "time-nw.nist.gov",
        "asia.pool.ntp.org",
        "europe.pool.ntp.org",
        "oceania.pool.ntp.org",
        "north-america.pool.ntp.org",
        "south-america.pool.ntp.org",
        "africa.pool.ntp.org",
        "ca.pool.ntp.org",
        "uk.pool.ntp.org",
        "us.pool.ntp.org",
        "au.pool.ntp.org",
    };

};

#endif