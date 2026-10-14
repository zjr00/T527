/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-06-21 11:34:55
 * @LastEditors: zjr00 28044466060@qq.com
 * @LastEditTime: 2025-04-10 11:41:00
 * @FilePath: \AOA\config.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "config.h"
#include <thread>
#include <atomic>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <csignal>


/*功能：配置文件*/
map<string, map<string, string>> Config::sections;
vector<string> Config::section_order;

struct ringinfo Config::g_ringinfo[NCHNMAX];
atomic<uint16_t> Config::g_package_sn{0};
bool Config::isCar =false;
map<int, ClientInfo> Config::client_map;
bool Config::requested =false;

struct ringbuf Config::mult_ringfifo[NCHNMAX][NMAX];
struct ringbuf Config::ringfifo[NMAX];
int Config::fd = -1;
string Config::dir_path = "";
string Config::val_path = "";

Config::Config()
{
    FileInit("./config.ini");
}

Config::~Config()
{
    
}


/*****************************************************************************
//	描述：		获取配置文件对应值
//	输入参数：
//section 分组
//key  键
//default_value  返回的默认值----可以不填 
*****************************************************************************/
string Config::Get(const string &section, const string &key, const string &default_value)
{
    auto section_it = Config::sections.find(section);

    if (section_it == sections.end()) {
        return default_value; //返回默认值空
    }

    auto key_it = section_it->second.find(key);
    if (key_it == section_it->second.end()) {
        return default_value;//返回默认值空
    }
    //cout<<"key_it:"<<key_it->second<<endl;
    return key_it->second;
}


/// @brief 修改配置文件
/// @param section 修改对象
/// @param key 修改键
/// @param value 修改值
/// @param filename 修改的文件
/// @return 
bool Config::SetToFile(const string &section, const string &key, const string &value, const string &filename)
{
    if (section.empty()) 
    {
        LOGE("节名不能为空\n");
        return false;
    }

    sections[section][key] = value;


    ofstream file(filename);
    if (!file.is_open()) {
        LOGE("无法打开文件进行写入\n");
        return false;
    }

    for (const auto& section_name : section_order) {
        file << "[" << section_name << "]" << endl;
        for (const auto& kv : sections[section_name]) {
            file << kv.first << " = " << kv.second << endl;
        }
        file << endl;
    }

    file.close();
    return true;
}

/*****************************************************************************
//	描述：		获取配置文件中的配置信息
//	输入参数：
//FileInit 配置文件路径
*****************************************************************************/
void Config::FileInit(string filename)
{
    
    ifstream file;
    LOGI("获取配置信息\n");
    file.open(filename,ios_base::out);
    if (!file.is_open()) {
        LOGE("打开文件失败\n");
        return;
    }
    
    string line;
    string current_section;

    while (getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            if (sections.find(current_section) == sections.end()) {
                section_order.push_back(current_section); // 记录新节的顺序
            }
        } else {
            size_t delimiter_pos = line.find('=');
            if (delimiter_pos != string::npos) {
                string key = line.substr(0, delimiter_pos);
                string value = line.substr(delimiter_pos + 1);

                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t\""));
                value.erase(value.find_last_not_of(" \t\"") + 1);


                sections[current_section][key] = value;
            }
        }
    }

    file.close();
    return;

}


/*****************************************************************************
//	描述：		计算数组有效数据长度
//	输入参数：
//buf -- 需要计算报文
//len  数组大小
*****************************************************************************/
int Config::GetCmdLen(unsigned char *buf, int bufLen)
{
   if (bufLen <= 0)
	{
		return 0;
	}
	int len = 0;
	for (int i = bufLen - 1; i >= 0; i--)
	{
		if ((char)*(buf + i) != (char)0x00)
		{
			len = i + 1;
			break;
		}
	}
	return len;
}


/*****************************************************************************
//	描述：		打印数组数据
//	输入参数：
//buf -- 需要打印的数据
//prompt  提示信息
*****************************************************************************/
void Config::Show(unsigned char *buf, const char *prompt,size_t buf_len)
{

    int len = GetCmdLen(buf, buf_len);
    if (len <= 0) {
        return;
    }

    printf("%s:", prompt);
    for (int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
        if ((i + 1) % 16 == 0)
        { // 每行打印16个字节
            printf("\n");
        }
    }
    printf("\n");
}


int findStartCode(unsigned char* buf, int zeros_in_startcode)
{
    int info;
    int i;

    info = 1;
    for (i = 0; i < zeros_in_startcode; i++)
        if (buf[i] != 0)
            info = 0;

    if (buf[i] != 1)
        info = 0;
    return info;
}

/*****************************************************************************
//	描述：		获取h264文件数据
//	输入参数：
//inpf -- 文件名
//buf  存储获取到的文件流
*****************************************************************************/
int Config::getNextNalu(FILE *inpf, unsigned char *buf)
{
    int pos = 0;
    int startCodeFound = 0;
    int info2 = 0;
    int info3 = 0;
    while (!feof(inpf) && (buf[pos++] = fgetc(inpf)) == 0); // fgetc:读取成功时返回读取到的字符，读取到文件末尾或读取失败时返回EOF
    while (!startCodeFound)
    {
        if (feof(inpf))   //feof:其功能是检测流上的文件结束符，如果文件结束，则返回非0值，否则返回0
        {
            return pos - 1;
        }
        buf[pos++] = fgetc(inpf);
        info3 = findStartCode(&buf[pos - 4], 3);//查找3字节的起始码
        startCodeFound = (info3 == 1);
        if (info3 != 1)
            info2 = findStartCode(&buf[pos - 3], 2);//如果没有查到3字节起始码，就查找2字节
        startCodeFound = (info2 == 1 || info3 == 1);
    }
    if (info2)
    {
        fseek(inpf, -3, SEEK_CUR);  //fseek:重定位流(数据流/文件)上的文件内部位置指针 返回值：成功，返回0，失败返回非0值，并设置error的值  SEEK_CUR	1
        return pos - 3;
    }
    if (info3)
    {
        fseek(inpf, -4, SEEK_CUR);
        return pos - 4;
    }
    return 0;
}

int Config::getNalupos2(uint8_t *data, int plen, int *prefix, uint8_t *nalu)
{
    uint8_t* p = NULL;
	int len = plen - 4;
    //遍历查找NALU起始码
	for (int i = 0; i < len; i++) {
		p = data + i;
		if ((*p) == 0x00 && (*(p + 1)) == 0x00 && (*(p + 2)) == 0x00 && (*(p + 3)) == 0x01) {
			*prefix = 4;
            *nalu = *(p + 4);//获取起始码类型
			return i + 4;
		}
		if ((*p) == 0x00 && (*(p + 1)) == 0x00 && (*(p + 2)) == 0x01)
		{
			*prefix = 3;
            *nalu = *(p + 3);
			return i + 3;
		}
	}
	return -1;
}

/**
 * @brief 初始化指定通道的环形缓冲区
 * @param chn 通道编号（0 ~ NCHNMAX-1）
 * @param size 每个缓冲区的大小
 * 
 * 功能：为指定通道分配 NMAX 个缓冲区，每个缓冲区大小为 size 字节
 */
void Config::ringmalloc_fifochn(int chn, int size)
{
    int i;
    if(chn >= NCHNMAX) {
        fprintf(stderr, "channel invaled\n");
        return;
    }
    for(i =0; i<NMAX; i++) {
        mult_ringfifo[chn][i].buffer = (unsigned char *)malloc(sizeof(unsigned char)*size);
        mult_ringfifo[chn][i].size = 0;
        mult_ringfifo[chn][i].frame_type = 0;
       // printf("FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
    }
    g_ringinfo[chn].iput = 0; /* 环形缓冲区的当前放入位置 */
    g_ringinfo[chn].iget = 0; /* 缓冲区的当前取出位置 */
    g_ringinfo[chn].n = 0; /* 环形缓冲区中的元素总数量 */
#ifdef FIFO_USE_MUTEX
    pthread_mutex_init(&g_ringinfo[chn].fifoMutex, NULL);
#endif
}

/**
 * @brief 重置指定通道的环形缓冲区（不清除数据，只重置索引）
 * @param chn 通道编号（0 ~ NCHNMAX-1）
 * 
 * 功能：将指定通道的环形缓冲区读写指针重置为初始状态
 */
void Config::ringreset_fifochn(int chn)
{
    if(chn >= NCHNMAX) {
        fprintf(stderr, "channel invaled\n");
        return;
    }
#ifdef FIFO_USE_MUTEX
    pthread_mutex_lock(&g_ringinfo[chn].fifoMutex);
#endif
    g_ringinfo[chn].iput = 0; /* 环形缓冲区的当前放入位置 */
    g_ringinfo[chn].iget = 0; /* 缓冲区的当前取出位置 */
    g_ringinfo[chn].n = 0; /* 环形缓冲区中的元素总数量 */
#ifdef FIFO_USE_MUTEX
    pthread_mutex_unlock(&g_ringinfo[chn].fifoMutex);
#endif
}


/**
 * @brief 释放指定通道的环形缓冲区内存
 * @param chn 通道编号（0 ~ NCHNMAX-1）
 * 
 * 功能：释放指定通道所有缓冲区占用的内存，并将指针置为 NULL
 * 调用此函数后，该通道的缓冲区将无法使用，需要重新调用 ringmalloc_fifochn
 */
void Config::ringfree_fifochn(int chn)
{
    int i;
    if(chn >= NCHNMAX) {
        fprintf(stderr, "channel invaled\n");
        return;
    }
    // printf("begin free mem\n");
    for(i =0; i<NMAX; i++) {
       // printf("FREE FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
       if(mult_ringfifo[chn][i].buffer) {
            free(mult_ringfifo[chn][i].buffer);
            mult_ringfifo[chn][i].buffer=NULL;
        }
        mult_ringfifo[chn][i].size = 0;
    }
#ifdef FIFO_USE_MUTEX
    pthread_mutex_destroy(&g_ringinfo[chn].fifoMutex);
#endif
}

/**
 * @brief 计算环形缓冲区的下一个索引位置
 * @param i 当前索引
 * @return 下一个索引位置（当 i == NMAX-1 时返回 0，否则返回 i+1）
 * 
 * 功能：实现环形缓冲区的索引递增，到达末尾后自动回到开头
 * 
 */
int addring(int i)
{
    return (i+1) == NMAX ? 0 : i+1;
}



/**
 * @brief 从指定通道的环形缓冲区中取出一个数据帧
 * @param chn 通道编号
 * @param getinfo 输出参数，用于存储取出的数据帧信息（缓冲区指针、大小、类型）
 * @return 成功返回数据大小（字节数），失败返回 0
 * 
 * 功能：从环形缓冲区中取出最早存入的数据帧（FIFO）
 * 
 */
int Config::ringget_fifochn(int chn, ringbuf *getinfo)
{
    int Pos;
    if(chn >= NCHNMAX) {
        fprintf(stderr, "channel invaled\n");
        return 0;
    }
    if(g_ringinfo[chn].n>0) {
    #ifdef FIFO_USE_MUTEX
        pthread_mutex_lock(&g_ringinfo[chn].fifoMutex);
    #endif
        Pos = g_ringinfo[chn].iget;
        g_ringinfo[chn].iget = addring(g_ringinfo[chn].iget);
        g_ringinfo[chn].n--;
        getinfo->buffer = (mult_ringfifo[chn][Pos].buffer);
        getinfo->frame_type = mult_ringfifo[chn][Pos].frame_type;
        getinfo->size = mult_ringfifo[chn][Pos].size;
    #ifdef FIFO_USE_MUTEX
        pthread_mutex_unlock(&g_ringinfo[chn].fifoMutex);
    #endif
        //printf("Get FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",Pos,getinfo->size,(int)(getinfo->buffer),getinfo->frame_type);
        return mult_ringfifo[chn][Pos].size;
    } else {
        //printf("Buffer is empty\n");
        return 0;
    }
}



/**
 * @brief 向指定通道的环形缓冲区中存入一个数据帧
 * @param chn 通道编号
 * @param buffer 要存入的数据缓冲区指针
 * @param size 数据大小（字节数）
 * @param encode_type 帧类型/编码类型（用于区分不同类型的数据）
 * @return 成功返回存入的数据大小，失败返回 0 或 -1
 * 
 * 功能：将数据帧存入环形缓冲区的写入位置

 * 注意：此函数会拷贝数据到内部缓冲区，调用者可以释放自己的 buffer
 */
int Config::ringput_fifochn(int chn, unsigned char *buffer, int size, int encode_type)
{
   if(chn >= NCHNMAX) {
        fprintf(stderr, "channel invaled\n");
        return -1;
    }
    if(g_ringinfo[chn].n<NMAX) {
    #ifdef FIFO_USE_MUTEX
        pthread_mutex_lock(&g_ringinfo[chn].fifoMutex);
    #endif
        memcpy(mult_ringfifo[chn][g_ringinfo[chn].iput].buffer,buffer,size);
        mult_ringfifo[chn][g_ringinfo[chn].iput].size= size;
        mult_ringfifo[chn][g_ringinfo[chn].iput].frame_type = encode_type;
        //printf("Put FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",iput,ringfifo[iput].size,(int)(ringfifo[iput].buffer),ringfifo[iput].frame_type);
        g_ringinfo[chn].iput = addring(g_ringinfo[chn].iput);
        g_ringinfo[chn].n++;
    #ifdef FIFO_USE_MUTEX
        pthread_mutex_unlock(&g_ringinfo[chn].fifoMutex);
    #endif
        return size;
    } else {
        fprintf(stderr, "Buffer is full\n");
        return 0;
    }
}

//获取报文流水号
uint16_t Config::package_get_sn()
{
    uint32_t sn = ++g_package_sn;
    while (sn == 0u) {
        sn = ++g_package_sn;
    }
    return sn;
}

string Config::GetTime_Path()
{
    time_t now = time(nullptr);
    tm local_time;
    localtime_r(&now, &local_time);
    ostringstream oss;
    oss <<"./"<< setw(4) << (local_time.tm_year + 1900)<<"/"
        << setw(2) << setfill('0') << (local_time.tm_mon + 1)<<"/"
        << setw(2) << setfill('0') << local_time.tm_mday<<"/";

    return oss.str();
}

uint8_t Config::hhd_crc8(const void * data, uint16_t data_len)
{   
    LOGI("data_len=%d\n",data_len);
    if (!data) {
        LOGE("数据为空!!!\n");
        return -1;
    }

    // if (data_len > 256) {
    //     LOGE("长度大小不对!!!\n");
    //     return -1;
    // }
    uint8_t crc;
    uint8_t i;
    crc = 0u;
    const uint8_t * data_temp = (uint8_t *)data;
    while (data_len--) {
        crc ^= *data_temp++;
        for(i = 0; i < 8; i++) {
            if(crc & 0x01) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool Config::check_gpio_exists()
{
    return access(dir_path.c_str(), F_OK) == 0;
}   



bool Config::initialize()
{   
    dir_path = string(GPIO_BASE) + "gpio" + to_string(GPIO_NUM) + "/direction";
    val_path = string(GPIO_BASE) + "gpio" + to_string(GPIO_NUM) + "/value";
    ofstream export_file("/sys/class/gpio/export");
    if (!export_file.is_open()) {
        LOGE("无法打开export文件");
        return false;
    }
    export_file << GPIO_NUM;
    export_file.close();
    
    
    LOGI("成功设置GPIO %d",GPIO_NUM);

    return true;
}


bool Config::OPen_485Com()
{
    // 打开串口设备
    fd = open(ttyAs, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        LOGE("打开串口失败");
        return false;
    }
    // 配置为阻塞模式
    fcntl(fd, F_SETFL, 0);

    // 配置串口参数
    struct termios options;
    if(tcgetattr(fd, &options) != 0) {
        LOGE("Error getting terminal attributes");
        close(fd);
        return false;
    }

    // 设置串口参数
    options.c_cflag |= (CLOCAL|CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    cfsetispeed(&options, Baud);
    cfsetospeed(&options, Baud);
    options.c_cflag &= ~CSTOPB;
    options.c_cc[VTIME] = 5;
    options.c_cc[VMIN] = 2;

    // 应用配置
    tcflush(fd, TCIOFLUSH);
    if(tcsetattr(fd, TCSANOW, &options) != 0) {
        LOGE("Error setting terminal attributes");
        close(fd);
        return false;
    }

    // 清空缓冲区
    tcflush(fd, TCIOFLUSH);
    LOGD("打开串口成功");
    return true;
}

void Config::write_gpio_value(int value)
{

    ifstream val_read_file(val_path);
    if (val_read_file.is_open()) {
        int current_value;
        val_read_file >> current_value;
        val_read_file.close();
        
        if (current_value == value) {
            return; // 值已经正确，无需操作
        }
    }



    ofstream dir_file(dir_path);
    if (!dir_file.is_open()) {
        LOGE("无法打开direction文件 %s",strerror(errno));
        return ;
    }
    dir_file << "out";

    ofstream val_file(val_path);
    if (!val_file.is_open()) {
        LOGE("无法打开value文件 %s",strerror(errno));
        return ;
    }
    val_file << value;

    LOGI("成功设置GPIO %d 值为 %d",GPIO_NUM,value);

    dir_file.close();
    val_file.close();
}

int Config::Get_fd()
{
    return fd;
}

string Config::getClientIP(int socket_fd)
{
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    
    if (getpeername(socket_fd, (struct sockaddr *)&addr, &addr_size) == 0)
    {
        char client_ip[15];
        inet_ntop(AF_INET, &addr.sin_addr, client_ip, 15);
        return string(client_ip);
    }
    
    return "Unknown";
}

string Config::stringToHexString(const string hex)
{
   stringstream ss;
    // 遍历字符串的每一个字节
    for (unsigned char c : hex) {
       ss << std::hex << std::setfill('0')<< std::setw(2) << static_cast<int>(c);
    }


    return ss.str();
}

void signal_handler(int signum, siginfo_t* info, void* context)
{
    cout << "\n=== 捕获到信号 ===" << endl;
    cout << "信号编号: " << signum << endl;
    cout << "发送者PID: " << (info->si_pid ? info->si_pid : getpid()) << endl;
    
    // 信号描述
    const char* signal_name = strsignal(signum);
    if(signal_name) {
        cout << "信号描述: " << signal_name << endl;
    }
    
    // 特殊信号处理
    switch(signum) {
        case SIGINT:  // Ctrl+C
            cout << "处理: 收到中断信号，准备退出..." << endl;
            exit(0);
            break;
            
        case SIGTERM: // 终止信号
            cout << "处理: 收到终止信号，准备退出..." << endl;
            exit(0);
            break;
            
        case SIGHUP:  // 挂起信号
            cout << "处理: 重新加载配置..." << endl;
            break;
            
        case SIGQUIT: // 退出信号
            cout << "处理: 收到退出信号，生成核心转储并退出..." << endl;
            // 重置为默认处理，以便生成核心转储
            signal(signum, SIG_DFL);
            raise(signum);
            return;
            
        case SIGCHLD: // 子进程状态改变
            // cout << "处理: 子进程状态改变，清理僵尸进程..." << endl;
            // int saved_errno = errno;
            // int status;
            // pid_t pid;
            // while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            //     // 可以用 syslog 或 write 记录（避免使用 cout）
            //     LOGI("子进程 %d 已退出", pid);
            // }
            // errno = saved_errno;
            break;
            
        case SIGCONT: // 继续执行
            cout << "处理: 进程继续执行..." << endl;
            break;
            
        case SIGTSTP: // 终端停止
            cout << "处理: 终端停止信号，进程将挂起..." << endl;
            // 注意：SIGTSTP默认行为是暂停进程，我们需要恢复默认处理
            signal(signum, SIG_DFL);
            raise(signum);
            return;
            break;
        case SIGSEGV: // 段错误
        case SIGILL:  // 非法指令
        case SIGFPE:  // 浮点异常
        case SIGBUS:  // 总线错误
            cout << "处理: 严重错误信号，程序将终止!" << endl;
            cout << "错误地址: " << info->si_addr << endl;
            cout << "错误代码: " << info->si_code << endl;
            exit(EXIT_FAILURE);
            break;
            
        default:
            cout << "处理: 默认信号处理" << endl;
            break;
    }

    cout << "================\n" << endl;
}


void Config::register_signal_handlers()
{
     struct sigaction sa;
    
    // 设置信号处理参数
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;  // 使用扩展的信号处理函数
    sa.sa_flags = SA_SIGINFO | SA_RESTART;  // 使用SA_SIGINFO获取更多信息
    
    // 清空信号掩码
    sigemptyset(&sa.sa_mask);
    
    cout << "开始注册信号处理函数..." << endl;
    
    // 注册2-20号信号
    for(int sig = 2; sig <= 20; sig++) {
        // 跳过不能捕获的信号
        if(sig == SIGKILL || sig == SIGSTOP) {
            cout << "信号 " << sig << " 不能被捕获" << endl;
            continue;
        }
        
        if(sigaction(sig, &sa, NULL) == -1) {
            cerr << "错误: 无法注册信号 " << sig << endl;
        } else {
            cout << "成功注册信号 " << sig << " (" << strsignal(sig) << ")" << endl;
        }
    }
}
