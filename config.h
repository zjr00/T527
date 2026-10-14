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
#include <mutex>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include <stdlib.h> //标准函数库定义
#include <fcntl.h> //文件控制定义
#include <termios.h> //POSIX终端控制定义
#include <chrono>
#include <iomanip> 
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "./third/osip/include/eXosip2/eXosip.h"
#include "./third/osip/include/eXosip2/eX_call.h"
#include "./third/osip/include/eXosip2/eX_setup.h"
#include "./third/osip/include/eXosip2/eX_register.h"
#include "./third/osip/include/eXosip2/eX_message.h"
#include "./third/osip/include/osipparser2/osip_list.h"
#include "./third/osip/include/osipparser2/osip_port.h"
#include "./third/osip/include/osipparser2/osip_body.h"
#include "./third/osip/include/osipparser2/osip_parser.h"
#include "./third/osip/include/osipparser2/sdp_message.h"
#include "./third/osip/include/osipparser2/osip_message.h"
#include "applog.h"




#define FIFO_USE_MUTEX

#ifdef FIFO_USE_MUTEX
    #include <pthread.h>
#endif


using namespace std;

struct ClientInfo {
    string ip_address;//ip
    string direction;
	string path;
	string direction_swith;
};


struct ringinfo {
    int iput; /* 环形缓冲区的当前放入位置 */
    int iget; /* 缓冲区的当前取出位置 */
    int n; /* 环形缓冲区中的元素总数量 */
#ifdef FIFO_USE_MUTEX
    pthread_mutex_t fifoMutex;
#endif
};

// #ifdef FIFO_USE_MUTEX
//     pthread_mutex_t mutex_ring;
// #endif

#define ttyAs "/dev/ttyAS4"
#define Baud B115200
#define GPIO_NUM 131
#define GPIO_BASE "/sys/class/gpio/"

#define NCHNMAX 12
#define NMAX 32




struct ringbuf {
    unsigned char *buffer;
	int frame_type;
    int size;
};

class Config{
public:
    Config();
    ~Config();
    static string Get(const string& section, const string& key, const string& default_value = "");
	static bool SetToFile(const string& section, const string& key,const string& value, const string& filename = "./config.ini");
    void FileInit(string filename);

    static int GetCmdLen(unsigned char *buf, int bufLen);
    static void Show(unsigned char *buf,const char *prompt,size_t buf_len );

	static int getNextNalu(FILE* inpf, unsigned char* buf);

	static int getNalupos2(uint8_t* data, int plen, int* prefix, uint8_t *nalu);

	static void ringmalloc_fifochn(int chn,int size);
	static void ringreset_fifochn(int chn);
	static void ringfree_fifochn(int chn);
	static int ringget_fifochn(int chn, struct ringbuf *getinfo);
	static int ringput_fifochn(int chn, unsigned char *buffer,int size,int encode_type);

	static uint16_t package_get_sn();
	static string GetTime_Path();
	static uint8_t hhd_crc8(const void * data, uint16_t data_len);

	static bool check_gpio_exists();
	static bool initialize();
	static bool OPen_485Com();
	static void write_gpio_value(int value);
	static int  Get_fd();
	static string getClientIP(int socket_fd);

	static string stringToHexString(const string hex);
	


	static void register_signal_handlers();
	static bool isCar;
	static bool requested;
	static map<int, ClientInfo> client_map;

	static int fd;
    static string dir_path;
    static string val_path;

private:
    static map<string, map<string, string>> sections;
	static vector<string> section_order;
	static struct ringbuf mult_ringfifo[NCHNMAX][NMAX];
	static struct ringbuf ringfifo[NMAX];
	static struct ringinfo g_ringinfo[NCHNMAX]; 
	static atomic<uint16_t> g_package_sn;



    //unordered_map<string, unordered_map<string, string>> data_;
    
};


#endif // CONFIG_H
