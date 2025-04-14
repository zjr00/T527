#ifndef LISTEN_SOCKET_H_
#define LISTEN_SOCKET_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <map>
#include <mutex>
#include <algorithm>
//#include "jt808.h"
#include "gate.h"
#include "g2d.h"
#include "config.h"
#include "base_json.h"

#include <sys/socket.h>
#include <sys/time.h>

struct PacketHeader {
    int type;          // 数据类型（1字节）
    uint32_t data_size;     // 数据部分大小（4字节）
    char image_name[20];   // 图片文件名（仅jpeg有效）
};

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

class ListeningSocket{
public:
    ListeningSocket();
    ~ListeningSocket();

    bool Listen(int port); //设置服务器

    int Accept();//等待client连接

    int Getsocket() const { return server_fd; }
    int GetClientSocket() const { return client_fd; }
    bool valid() const { return server_fd != -1; }


    typedef std::vector<int> SocketArray;
    SocketArray sockets;
protected:
    void SetConfig(string config,int sock);
    int SocketBase(int sock);
    void jt808_init();
    int Recv_handler(int socked);//接收数据
    void send_text(int sock, const std::string& text);
    //void Send_handler();//发送数据
    bool receive_full(int sock, char* buffer, size_t expected_size);
    time_t get_ntp_time(const char* host);
    void check_time();
    int server_fd,client_fd;
    struct sockaddr_in serveraddr;
    char recvBuff[1024];
    //Jt808 jt808;
    gate Gate;
    G2d g2d;
    base_json bj;
  
    std::mutex sockets_mutex;

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

    const int NTP_PORT = 123;
    const long NTP_OFFSET = 2208988800UL; // 1900-01-01到1970-01-01的秒数差

};

#endif