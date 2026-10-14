
#include "listen_socket.h"
#include "config.h"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <iostream>
#include <ctime>
#include <sys/types.h>
#include <cmath>
#include <csignal>  
#include <zlib.h>
#include <execinfo.h>
#include <cxxabi.h> 


/*功能：主页功能*/
ListeningSocket::ListeningSocket()
{
    
    // thread xstdata(&ListeningSocket::Xstdata, this);
    // xstdata.detach();
    //Restart();
}
ListeningSocket::~ListeningSocket()
{
    // LOG_APP_Log_SetSysModuleInited(APP_LOG_FALSE);
    // LOG_APP_Log_DeInit();
    if (server_fd != -1) {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
        server_fd = -1;
    }

    if (serverfile_fd != -1) {
        shutdown(serverfile_fd, SHUT_RDWR);
        close(serverfile_fd);
        serverfile_fd = -1;
    }
    
    if (server_Video != -1) {
        shutdown(server_Video, SHUT_RDWR);
        close(server_Video);
        server_Video = -1;
    }

    if (client_fd != -1) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        client_fd = -1;
    }

    
    for(int i=0;i<sockets.size();i++)
    {
        close(sockets[i]);
    }

    pthread_mutex_destroy(&mx_array_buffer);
    // pthread_mutex_destroy(&parse_sync.mutex);
    // pthread_cond_destroy(&parse_sync.cond);
    // parse_sync.data_ready = 0;


    for (auto& [sock, ctx] : client_contexts) 
    {
        // 销毁同步对象
        pthread_mutex_destroy(&ctx->mutex);
        pthread_cond_destroy(&ctx->cond);
        ctx->data_ready = 0;
        // 清理缓冲区数据
    }
    

    client_contexts.clear();
    
}



void ListeningSocket::Listen_Init()
{
    //日志
    APP_LOG_S32 nRet = LOG_SUCCESS;
    APP_LOG_ATTR_T tLogAttr;
    memset(&tLogAttr, 0, sizeof(APP_LOG_ATTR_T));
    tLogAttr.nTarget = APP_LOG_TARGET_SYSLOG | APP_LOG_TARGET_APPLOG | APP_LOG_TARGET_STDOUT;
    tLogAttr.nLv = 4;
    sprintf(tLogAttr.szAppName, "%s", "t527");
    nRet = LOG_APP_Log_Init(&tLogAttr);
    if (LOG_SUCCESS != nRet) {
        return;
    }

    DEBUG_LogSetLevel(DEBUG_LOGINFO);
    LOG_APP_SetLogLevel(DEBUG_LOGINFO);
    LOG_APP_Log_SetOutput(stdout);
    LOG_APP_Log_SetSysModuleInited(APP_LOG_TRUE);
    printf("current log level: %d %s\n",LOG_APP_GetLogLevel(),LOG_APP_LogLevelToString(LOG_APP_GetLogLevel()));

    //config.FileInit("./config.ini");
    tmp_path =Config::Get("Path","Tmp");


    //回调
    jt808.packag.addParameterCallback(0x01, 
        bind(&ListeningSocket::onMachineIpReceived_callback, this, 
                  placeholders::_1, placeholders::_2));

    jt808.packag.addParameterCallback(0x02, 
        bind(&ListeningSocket::onIsOrder_callback, this, 
                  placeholders::_1, placeholders::_2));

    gb.sipClient.addParameterCallback(0x01,
        bind(&ListeningSocket::onGbChannel_callback, this, 
                  placeholders::_1,placeholders::_2));
    
    gb.sipClient.addParameterCallback(0x02,
        bind(&ListeningSocket::onGbSendStop_callback, this, 
                  placeholders::_1,placeholders::_2));

    // #if TARGET_MODE == 2

    //     g2d.Create_Path();
    // #endif
    
    // radar.Radar_Init();

    Config::register_signal_handlers();//信号捕捉

    // thread Ra(&ListeningSocket::Read_485, this);
    // Ra.detach();
    check_time();//配置时间

    thread jt(&ListeningSocket::jt808_init, this);
    jt.detach();

    
    // thread gb_thread(&ListeningSocket::gb28181_init, this);
    // gb_thread.detach();

    //备案，注册到gb2818
    
    // Gate.NetUnLock();
}


// 设置服务器并进行监听
bool ListeningSocket::Listen(int port)
{
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOGE("socket failed");
        return -1;
    }
    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,&optval, sizeof(optval));//端口复用
    
    // 初始化地址结构体
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);

    // 绑定socket
    if (bind(server_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) {
        LOGE("bind failed");
        return -1;
    }
    listen(server_fd, 50);
    LOGD("Server listening on port %i",port);
    return 1;
}

//等待客户端连接
int ListeningSocket::Accept()
{
    socklen_t addrlen = sizeof(serveraddr);
    int client =accept(server_fd, reinterpret_cast<sockaddr*>(&serveraddr), &addrlen);
    if (client == -1)
    {
        LOGE("accept failed");
        return -1;
    }

    thread recvThread([this, client]() {
        Recv_handler(client);
    });
    recvThread.detach();

    // thread sendThread([this, client]() {
    //     Send_handler(client);
    // });
    // recvThread.detach();


    return SocketBase(client);
}


//配置时间
time_t ListeningSocket::get_ntp_time(const char *host)
{
    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_INET; // 仅使用IPv4
    hints.ai_socktype = SOCK_DGRAM;
    
    int err = getaddrinfo(host, "ntp", &hints, &res);
    if (err != 0) {
        throw runtime_error(gai_strerror(err));
    }

    // 创建Socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        throw runtime_error("Socket creation failed");
    }


    // 设置超时（2秒）
    struct timeval timeout = {2, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    // 构造NTP请求包
    ntp_packet packet = {};
    packet.li_vn_mode = 0x1B;

    // 发送请求到解析后的地址
    if (sendto(sockfd, (char*)&packet, sizeof(packet), 0,
              res->ai_addr, res->ai_addrlen) < 0) {
        close(sockfd);
        freeaddrinfo(res);
        throw runtime_error("Send failed");
    }

    // 接收响应
    socklen_t addr_len = res->ai_addrlen;
    int len = recvfrom(sockfd, (char*)&packet, sizeof(packet), 0,
                      res->ai_addr, &addr_len);
    close(sockfd);
    freeaddrinfo(res);

    // if (len < sizeof(packet)) {
    //     throw runtime_error("Invalid response");
    // }

    // 转换网络字节序到主机字节序
    packet.trans_ts_sec = ntohl(packet.trans_ts_sec);
    if (packet.trans_ts_sec < 2208988800U) {  // 1970-01-01 的 NTP 时间戳
        throw runtime_error("Invalid NTP timestamp");
    }
    
    // 计算UNIX时间戳（减去70年差值,）获取的是0时区的时间,需要转化为东八区
    return (time_t)(packet.trans_ts_sec - NTP_OFFSET +28800);
}


//配置设备的时间
void ListeningSocket::check_time()
{
    try {
        time_t ntp_time = 0;
        time_t local_time = time(nullptr);
        time_t max_ntp_time = 0;
        string success_server = "";
        
        // 时间差阈值（秒），只有超过这个值才更新
        const int TIME_DIFF_THRESHOLD = 10;  // 10秒
        
        // 遍历所有NTP服务器
        for (const auto& server : NTP_SERVERS) {
            try {
                time_t current_ntp_time = get_ntp_time(server);
                if (current_ntp_time > 1000000000) {
                    LOGI("从 %s 获取到时间: %s", server, ctime(&current_ntp_time));
                    
                    if (current_ntp_time > max_ntp_time) {
                        max_ntp_time = current_ntp_time;
                        success_server = server;
                    }
                    break;
                }
            } catch (const exception& e) {
                LOGE("%s 失败: %s", server, e.what());
            }
        }
        
        if (max_ntp_time == 0) {
            throw runtime_error("所有NTP服务器连接失败或时间无效");
        }
        
        int time_diff = difftime(max_ntp_time, local_time);
        
        LOGI("本地时间: %s，时间差%d", ctime(&local_time),time_diff);
        // 只有当NTP时间大于本地时间且超过阈值时才更新
        if (time_diff > TIME_DIFF_THRESHOLD) {
    
            struct timeval tv;
            tv.tv_sec = max_ntp_time;
            tv.tv_usec = 0;
            
            if (settimeofday(&tv, NULL) != 0) {
                throw runtime_error("settimeofday failed (需要root权限)");
            }
            
        #if TARGET_MODE == 2
                g2d.Create_Path();
        #endif
            
            LOGI("系统时间已更新，新时间: %s", ctime(&max_ntp_time));
        } else if (time_diff > 0) {
            LOGI("时间差 %.0f 秒在允许范围内，无需更新", time_diff);
        } else if (time_diff < 0) {
            LOGI("系统时间超前 %.0f 秒，无需回退", -time_diff);
        } else {
            LOGI("时间已同步");
        }
    } catch (const exception& e) {

        LOGE("错误 %s",e.what());
        //cerr << "错误: " << e.what() << endl;
        return ;
    }
}

//重启程序
void ListeningSocket::Restart()
{
    // ftp.Get_FTPconfig();
    // while (1)
    // {
    //     if(Config::requested)
    //     {
    //         Config::requested =false;
            
            pid_t pid = fork();
            if (pid == 0) {
                // 子进程
                exit(EXIT_FAILURE); // 如果 execv 失败
            } else if (pid > 0) {
                // 父进程
                system("sh ./none.sh");
                exit(EXIT_SUCCESS);
            } else {
                perror("fork 失败");
            }
    //     }
        
    // }
}

void ListeningSocket::RestartThread(string iface)
{
    sleep(10);
    Clear();
    cout << "配置文件已更新，准备重启网络服务和程序..." << endl;
    //重启网络
    string cmd = "ifdown " + iface + " && ifup " + iface;
    int ret = system(cmd.c_str());
    if (ret == 0) {
        cout << "使用 ifdown/ifup 重启接口 " << iface << " 成功" << endl;
    }
   
    Restart();
}

// 检查socket有效性
bool ListeningSocket::is_valid_socket(int sock)
{
    lock_guard<mutex> lock(sockets_mutex);
    return find(sockets.begin(), sockets.end(), sock) != sockets.end();
}

void ListeningSocket::parse_license_plate(char *result,int clnt_sock)
{
    LOGI("车牌索引：%s",result);
    int indices[7];
    string CarNum;  
    int count = sscanf(result, "%d %d %d %d %d %d %d", 
                      &indices[0], &indices[1], &indices[2], 
                      &indices[3], &indices[4], &indices[5], &indices[6]);
    for (int i = 0; i < count; i++) {
        if (indices[i] >= 0 && indices[i] < MAX_CLASS) {
            CarNum += nna_label[indices[i]]; 
        } else {
            printf("[无效索引:%d]", indices[i]);
        }
    }
    LOGI("发送IP：%s,车牌：%s",Config::getClientIP(clnt_sock).c_str(),CarNum.c_str());
    //jt808.pack_jt808_0316(CarNum);
    cpature_picture();

    printf("\n");
}

//协同机ip回调实现
void ListeningSocket::onMachineIpReceived_callback(int tslKey, const string &content)
{
    // cout << "[ListeningSocket] Received machine IP: " << content 
    //           << " (tslKey: 0x" << hex << tslKey << dec << ")" << endl;
    thread updateThread([this,content]() {
        RestartThread(content);//接收客户端信息
    });
    updateThread.detach();
 
}


//平台是否有订单回调
void ListeningSocket::onIsOrder_callback(int tslKey, const string &content)
{
    if (content == "0")
    {
        
    }
    else if (content == "1")
    {
        cpature_picture();
    }
    
}

void ListeningSocket::onGbChannel_callback(int tslKey, int content)
{
    lock_guard<mutex> lock(mtx);
    int sock;
     LOGI("通道号%d\n",content);
    for (auto& pair : ip_to_connection) {
        if (pair.second.unique_id == content) {
            chn = content;
            sock = pair.second.sock_msg;
            is_gb =true; 
        }
    }
    //发送报文指令给630
    hhd_msg_set_command(MSG_CONFIG_28181,sock,1);

}

void ListeningSocket::onGbSendStop_callback(int tslKey, int content)
{
    lock_guard<mutex> lock(mtx);
    int sock;
    LOGI("停止推流通道号%d\n",content);
    for (auto& pair : ip_to_connection) {
        if (pair.second.unique_id == content) {
            is_gb =false;
            sock = pair.second.sock_msg;
        }
    }
    //发送报文指令给630
    hhd_msg_set_command(MSG_CONFIG_28181,sock,0);
}

void ListeningSocket::Clear()
{
    if (server_fd != -1) {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
        server_fd = -1;
    }


    if (serverfile_fd != -1) {
        shutdown(serverfile_fd, SHUT_RDWR);
        close(serverfile_fd);
        serverfile_fd = -1;
    }

    if (server_Video != -1) {
        shutdown(server_Video, SHUT_RDWR);
        close(server_Video);
        server_Video = -1;
    }


    if (client_fd != -1) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        client_fd = -1;
    }
    for(int i=0;i<sockets.size();i++)
    {
        close(sockets[i]);
    }
}

//////////////////////////////////////////////////////////////////////////////////////
//发送心跳
int ListeningSocket::hhd_heart_beat_resp(int sock)
{   
    LOGI("设备在线个数：%d\n",ip_to_connection.size());

    mcucom_msg_heart_beat_resp_t msg;
    memset(&msg,0,sizeof(mcucom_msg_heart_beat_resp_t));
    msg.src_sn = 1;//设置源流水号
    msg.reserve=0;
    uint8_t data[350];
    msg_cmd_package_t cmd_pkt;
    cmd_pkt.type = MCUCOM_MSG_HEART_BEAT;
	memset(data, 0, sizeof(data));
    memcpy(cmd_pkt.cmd_data,static_cast<const void*>(&msg),sizeof(mcucom_msg_heart_beat_resp_t));
    cmd_pkt.cmd_data_len = sizeof(mcucom_msg_heart_beat_resp_t);
   
	return hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_HEART_BEAT, \
								(void *)&cmd_pkt,  \
								cmd_pkt.cmd_data_len+4,\
								NULL, 0,sock);

}


//发送心跳
void ListeningSocket::heart_handler(int sock)
{
    while (true)
    {
        if (!is_valid_socket(sock))
        {
            printf("退出\n");
            break;
        }
        if (hhd_heart_beat_resp(sock)<0)
        {
            break;
        }
        sleep(15);
    }
    
}


//捕获照片
int ListeningSocket::cpature_picture()
{
    uint8_t data[350];
    msg_cmd_package_t cmd_pkt;

    memset(data, 0, sizeof(data));
    cmd_pkt.type = MSG_REMOTE_CAPTRUE;

    sprintf(cmd_pkt.cmd_data, "show.hhdlink.online:7271,/t32/z,test,hhd@123.com,%ld.jpg",time(NULL));
    cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    LOGI("测试拍照指令 %s\n", cmd_pkt.cmd_data);
    g_mcu_id = 0; // 发送方ID nrf5340为0，T31为1

    LOGI("照片套接字大小：%d\n",cpature_sock.size());
    for (int  i = 0; i < cpature_sock.size(); i++)
    {
        hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD,
                                                    (void *)&cmd_pkt,
                                                    cmd_pkt.cmd_data_len + 2 + 2,
                                                    NULL, 0,cpature_sock[i]);
    }
    
    
    printf("\n");
    return 0;
}

void ListeningSocket::remove_socket(int sock)
{
    LOGD("sockets 大小：%d,sock=%d\n",sockets.size(),sock);
    auto it = find(sockets.begin(), sockets.end(), sock);
    if (it != sockets.end()) {
        int index = distance(sockets.begin(), it);
        close(sockets[index]);
        sockets.erase(it);
        sock =-1;
    }
    else {
        ++it;
    }


    // auto it2 = find(cpature_sock.begin(), cpature_sock.end(), sock);
    // if (it2 != cpature_sock.end()) {
    //     cpature_sock.erase(it2);
    //     LOGI("清理断开连接的客户端套接字: %d", sock);
    // }

   
    std::string client_ip =Config::getClientIP(sock) ;
    auto conn_it = ip_to_connection.find(client_ip);
    if (conn_it != ip_to_connection.end()) {
        // 清除对应的socket
        LOGI("清理断开连接的客户端套接字: %d", sock);
        if (conn_it->second.sock_msg == sock) conn_it->second.sock_msg = -1;
        if (conn_it->second.sock_file == sock) conn_it->second.sock_file = -1;
        if (conn_it->second.sock_video == sock) conn_it->second.sock_video = -1;
        
        // 如果所有socket都已断开，删除整个连接信息
        if (conn_it->second.sock_msg == -1 && 
            conn_it->second.sock_file == -1 && 
            conn_it->second.sock_video == -1) {
            ip_to_connection.erase(conn_it);
        }
    }
}

//接收报文线程
void ListeningSocket::serial_recv(int serial_fd,shared_ptr<ParseSync_t> ctx)
{
    ssize_t nbytes;
    char buffer[ONE_FRAME_MAX_LEN];
    LOGD("serial_recv start");
    while(1)
	{
		while ((nbytes = read(serial_fd, buffer, sizeof(buffer))) > 0) {
            if(!arraybuf_add(&serial_buf, (uint8_t *)buffer, nbytes)) {
                LOGD("serial failed to add data to recvbuf");
            }
            if(ctx->data_ready == 0) {
                pthread_mutex_lock(&ctx->mutex);
                while(!ctx->data_ready)
                    ctx->data_ready = 1;
                pthread_cond_signal(&ctx->cond);
                pthread_mutex_unlock(&ctx->mutex);
            }
		} 
        // 接收文件结束标志
        char transfer_flag;
        if(recv(serial_fd, &transfer_flag, 1, MSG_PEEK) <= 0) {
            LOGE("%s 连接断开",Config::getClientIP(serial_fd).c_str());
            remove_socket(serial_fd);
            break; // 连接断开
        }


        if(transfer_flag == 0) {
            LOGD("[Info] Client finished sending files");
            recv(serial_fd, &transfer_flag, 1, 0); // 消耗标志位
            break;
        }
        //BLOCK READ,not sleep
	}
    LOGD("serial_recv exit");
    return ;

}

//解析报文线程
void ListeningSocket::serial_parse(int serial_fd,shared_ptr<ParseSync_t> ctx)
{
    int ret;
    struct timespec ts;
    
    while(1)
    {
        pthread_mutex_lock(&ctx->mutex);
        while(!ctx->data_ready) {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 60;
            ret = pthread_cond_timedwait(&ctx->cond, &ctx->mutex, &ts);
            // if (ret == ETIMEDOUT)
            //     printf("Consumer: Wait serial data timed out\n");
        }
        ctx->flag_waitdat = 0;
        while( arraybuf_get_length(&serial_buf) > 0 && !ctx->flag_waitdat) {
            if(ctx->flag_parsing == 0) {
                if(serial_buf.buffer[0]!=((serial_head_flg>>8)&0xFF) || serial_buf.buffer[1]!=(serial_head_flg&0xFF)) {
                    arraybuf_del(&serial_buf, 1);
                    continue ;						
                }
                ctx->flag_parsing = 1;
            }
            ret = protocol_parse(serial_buf.buffer, serial_buf.length,serial_fd);
            if(ret < 0) {//数据太少了, 等待数据下一次到来
                LOGD("\nThe buffer data is insufficient. Continue to wait for the data");
                ctx->flag_waitdat = 1;
            } else {//解析失败或者成功
                arraybuf_del(&serial_buf, 1);//删掉一个继续寻找协议头
			    ctx->flag_parsing = 0; 
			    LOGD("Delete a byte and search for the starting point anew");
            }
        }
        while(ctx->data_ready)
            ctx->data_ready = 0;
        pthread_mutex_unlock(&ctx->mutex);
    }
    LOGD("serial_recv exit");
    return ;
}


int ListeningSocket::protocol_parse(uint8_t *data, int len,int sock)
{
    int ret;
    ret = parse_msg_head(data,len);
    if(ret == -2) {//数据太少了, 等待数据下一次到来
        return -1;
    }
    if(ret == -1 || ret == 0 ) {//解析失败或者成功
        if(ret == 0) {//解析命令成功
            parse_msg_data(data, len,sock);
        }
    }
    return 0;
}

int ListeningSocket::get_remain_frames(uint8_t *date, int nbytes)
{
    int i;
    int count = 0;
    for(i=1;i<nbytes;i++) {
        if(date[i-1]==((MCU_PACKAGE_HEAD_FLG>>8)&0xFF) && date[i]==(MCU_PACKAGE_HEAD_FLG&0xFF))
            count++;
    }
    return count;
}

// 0 表示数据包校验成功
//-1 表示数据包不符合协议, 需要删除一个字节 重新寻找起始
//-2 表示 数据太短, 不用删除数据 , 继续等待,
int ListeningSocket::parse_msg_head(uint8_t *data, int nbytes)
{
    int head_len = sizeof(mcucom_package_head_t);
    if(nbytes < head_len) {
        printf("The length is too short. Wait for data\n");		
        return -2;	//数据确实太少, 继续等待数据
    }

    mcucom_package_t *p =  (mcucom_package_t *)data;
    mcucom_package_head_t *head = &p->head;
    msg_cmd_package_t *cmd=(msg_cmd_package_t *)(p->data);

    LOGI("!!!!!!!!!!!!!!!!!!!!head->data_len = %d\n",head->data_len);
    //对数据包长度进行限制
    if(head->data_len > ONE_FRAME_MAX_LEN) {
        printf("The packet length %u is too long. Find the start again\n", head->data_len);
        return -1;			//删掉一个字节重新寻找协议的起始
    }

    LOGI("head->data_len = %d\n",head->data_len);
    //判断剩余长度
    if(nbytes - head_len >= head->data_len) {//缓冲区长度与数据包长度相等或者长
        //判断 cmd_data 
        uint8_t check_cmd_len = head->data_len -2 -2 ;//
        if(check_cmd_len != cmd->cmd_data_len && head->msg_type == MCUCOM_MSG_CMD) {
            printf("the packet length %u and cmd length %u do not match, find the start again\n", head->data_len, cmd->cmd_data_len);
            return -1;			//删掉一个字节重新寻找协议的起始
        }
        if(bspal_mcucom_package_crc((mcucom_package_t *)data)!=head->crc) {
            printf("----------Caution---------- Data verification failure The crc comparison is inconsistent\n");
            return -1;
        }
    } else {//如果缓冲区长度比数据包长度小, 
        if(get_remain_frames(data, nbytes) > 1) {
            printf("----------Caution---------- There is a problem with receiving data here. One frame is received incorrectly\n");
            return -1;
        } else {
            return -2; 			//数据确实太少, 继续等待数据
        }
    }
    printf("解析命令成功");
    printf("  Package Header Information:");
    printf("  pack_head_flg: 0x%04x\n", head->pack_head_flg);
    printf("  data_len: %hu\n", head->data_len);
    printf("  crc: 0x%02X\n", head->crc);
    printf("  mcu_id: 0x%02X\n", head->mcu_id);
    printf("  sn: 0x%02X\n", head->sn);
    printf("  src_sn: 0x%04X\n", head->src_sn);
    printf("  msg_type: 0x%04X\n", head->msg_type);
    printf("\n");
    // for(int i=0;i<nbytes;i++) {
    //     // if((i&0x1F) == 0)
    //     //     printf("\n");
    //     printf("%02X ",data[i]);
    // }
    // printf("\n");
    return  0;
}

int ListeningSocket::parse_msg_data(uint8_t *data, int nbytes,int sock)
{
    // int head_len = sizeof(mcucom_package_head_t);
    mcucom_package_t *p =  (mcucom_package_t *)data;
    mcucom_package_head_t *head = &p->head;
    LOGD("process msg type:[%02X]",head->msg_type);
    switch(head->msg_type)
    {
        case MCUCOM_MSG_CMD_RESP:
            printf("MCUCOM_MSG_CMD_RESP\n");
            
            break;
        case MCUCOM_MSG_HEART_BEAT:
            printf("MCUCOM_MSG_HEART_BEAT\n");
            break;
        case MCUCOM_MSG_HEART_BEAT_RESP:  
            printf("MCUCOM_MSG_HEART_BEAT_RESP\n");//接收到心跳应答
            process_heart_beat_resp( data, nbytes,sock);
            break;
        case MSG_CONFIG_28181_RESP:  
            printf("gb28181应答\n");
        default:
            printf("CMD not support\n");
    }
    return 0;
}


//解析心跳应答
int ListeningSocket::process_heart_beat_resp(uint8_t *data, int nbytes,int sock)
{
    mcucom_package_t *p =  (mcucom_package_t *)data;
    msg_cmd_package_t *pt = (msg_cmd_package_t *)p->data;

    mcucom_msg_heart_beat_resp_t * hb_recv = (mcucom_msg_heart_beat_resp_t *)(pt->cmd_data);
    //LOGI("摄像机IP：%s,方位：%s,状态：%d",Config::getClientIP(sock).c_str(),get_direction_string(hb_recv->data.direction),hb_recv->data.camera_status);
    LOGI("摄像机IP：%s,方位：%d,状态：%d",Config::getClientIP(sock).c_str(),(hb_recv->data.direction),hb_recv->data.camera_status);
    jt808.packag.set_camera_status((hb_recv->data.direction),hb_recv->data.camera_status);
    //m_socket_DirectionMap[hb_recv->data.direction] = sock;
    string key = "IPC"+ hb_recv->data.direction;
    Config::SetToFile("IPC",key,Config::getClientIP(sock));

    lock_guard<mutex> lock(mtx);
    auto it_ip = ip_to_connection.find(Config::getClientIP(sock));
    if (it_ip != ip_to_connection.end()) {
        it_ip->second.unique_id = hb_recv->data.direction;
        
    }

    auto it = find(cpature_sock.begin(), cpature_sock.end(), sock);
    
    // 判断是否为拍照客户端
    bool is_capture_client = (hb_recv->data.direction != 5 && hb_recv->data.direction != 10);
    
    if (is_capture_client) {
        if (it == cpature_sock.end()) {
            cpature_sock.push_back(sock);
            //LOGI("添加拍照客户端套接字: %d, 当前总数: %zu", sock, cpature_sock.size());
        }
    } else {
        if (it != cpature_sock.end()) {
            cpature_sock.erase(it);
            //LOGI("移除非拍照客户端套接字: %d, 当前总数: %zu", sock, cpature_sock.size());
        }
    }
    return 0;
}

size_t ListeningSocket::arraybuf_get_length(StaticArrayBuffer *buff)
{
   size_t length;
    pthread_mutex_lock(&mx_array_buffer);
    length = buff->length;
    pthread_mutex_unlock(&mx_array_buffer);
    return length;
}

bool ListeningSocket::arraybuf_del(StaticArrayBuffer *buff, size_t n)
{
    bool result;
    pthread_mutex_lock(&mx_array_buffer);
    if (n > buff->length) {
        result = false;
    } else {
        memmove(buff->buffer, buff->buffer + n, buff->length - n);
        buff->length -= n;
        result = true;
    }
    pthread_mutex_unlock(&mx_array_buffer);
    return result;
}

bool ListeningSocket::arraybuf_add(StaticArrayBuffer *buff, const uint8_t *data, size_t len)
{
    if (arraybuf_get_remain(buff) < len) 
        return false;
    pthread_mutex_lock(&mx_array_buffer);
    memcpy(buff->buffer + buff->length, data, len);
    buff->length += len;
    pthread_mutex_unlock(&mx_array_buffer);
    return true;
}

size_t ListeningSocket::arraybuf_get_remain(StaticArrayBuffer *buff)
{
    size_t length;
    pthread_mutex_lock(&mx_array_buffer);
    length = ARRAY_BUFFER_CAPACITY - buff->length;
    pthread_mutex_unlock(&mx_array_buffer);
    return length;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 接收完整数据包
bool  ListeningSocket::receive_full(int sock, char* buffer, size_t expected_size) {
    size_t total_received = 0;
    while (total_received < expected_size) {
        ssize_t received = recv(sock, 
                               buffer + total_received, 
                               expected_size - total_received, 
                               0);
        if (received <= 0)
        {
            lock_guard<mutex> lock(sockets_mutex);
            auto it = find(sockets.begin(), sockets.end(), sock);
            if (it != sockets.end()) {
                sockets.erase(it);
                Config::client_map.erase(sock);
            }
            return false;
        }
        
        total_received += received;
    }
    return true;

}

//接收文件线程
int  ListeningSocket::Recv_handler(int socked)
{
    while (true)
    {

        if(process_file_transfer(socked) < 0) {
            LOGD("[Warning] File transfer failed, waiting for next file");
        }
        // 接收文件结束标志
        char transfer_flag;
         if(recv(socked, &transfer_flag, 1, MSG_PEEK) <= 0) {
            LOGE("%s 连接断开",Config::getClientIP(socked).c_str());
            remove_socket(socked);
            break;
        }


        if(transfer_flag == 0) {
            LOGD("[Info] Client finished sending files");
            recv(socked, &transfer_flag, 1, 0); // 消耗标志位
            break;
        }
    }

    return 0;
}

//发送给630摄像机指令
int ListeningSocket::Send_handler(int socked)
{
    while (true)
    {       
        if (!is_valid_socket(socked))
        {
            printf("退出\n");
           break;
        }
        

        // if (hhd_msg_set_ai(socked,1) < 0)
        // {
        //     break;
        // }

         if (hhd_test(socked) < 0)
        {
            break;
        }

        sleep(30);
    }
    return 0;
}


//接收630返回的指令响应
int ListeningSocket::netctrl_recv(int socked)
{
    serial_head_flg = MCU_PACKAGE_HEAD_FLG;
    // pthread_mutex_init(&parse_sync.mutex, NULL);
    // pthread_cond_init(&parse_sync.cond, NULL);
    // parse_sync.data_ready = 0;
    // serial_buf.length = 0;

    auto ctx = make_shared<ParseSync_t>();
    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond, NULL);
    ctx->data_ready = 0;
    ctx->flag_parsing = 0;
    ctx->flag_waitdat = 0;

    {
        lock_guard<mutex> lock(contexts_mutex);
        client_contexts[socked] = ctx;
    }



    thread serial([this, socked,ctx]() {
        serial_recv(socked,ctx);
    });
    serial.detach();

    thread serial_([this, socked,ctx]() {
        serial_parse(socked,ctx);
    });
    serial_.detach();
    return 0;
}

int ListeningSocket::Receive_h264_video(int socked)
{
    //FILE * fp = fopen("test.h264","w+");
    unsigned char recv_buffer[1024*1024];
   


    while (1)
    {
        ssize_t received = recv(socked, recv_buffer, sizeof(recv_buffer), 0);
        if (received>0)
        {
            // for (auto& pair : ip_to_connection) {
            //     if (pair.second.sock_video == socked) {
            //         chn = pair.second.unique_id;
            //     }
            // }
    
            //fwrite(recv_buffer,received,1,fp);
            // 处理接收到的数据
            gb.processReceivedData(chn,recv_buffer, received);
            usleep(39*1000);
        }
    }
    return 0;
}


void ListeningSocket::Xstdata()
{
    //Public::Init();
}


//接收485雷达数据
void ListeningSocket::Read_485()
{
    char recvBuf[256];
    int size = 0;
    
    while (1)
    {
        size = 0;
        memset(recvBuf, 0, sizeof(recvBuf));
        size =radar.GetRadardData(recvBuf,sizeof(recvBuf));

        if(size > 0)
        {
            for (int  i = 0; i < size; i++)
            {   
                if(recvBuf[i] == 0x70 &&           
                recvBuf[i+1] == 0x04 && recvBuf[i+4] == 0x01) 
                {
                    printf("触发雷达\n");
                }
                else if(recvBuf[i] == 0x70 &&           
                recvBuf[i+1] == 0x04 && recvBuf[i+4] == 0x02) 
                {
                    printf("解除雷达\n");
                }
            }
        }

    }
}

//接收文件
int ListeningSocket::process_file_transfer(int clnt_sock)
{

    FileHeader header;
    // 接收文件头
    if(recv(clnt_sock, &header, sizeof(FileHeader), MSG_WAITALL) != sizeof(FileHeader)) {
        return -1; // 非致命错误，允许继续处理下一个文件
    }

 
    if(header.magic != MAGIC_NUMBER) {
        printf("[Error] Invalid protocol header (0x%08X)\n", header.magic);
        return -1;
    }
    stringstream ss;
    ss << tmp_path << "/0" << header.axis<<"_"<<header.filename;
    string full_filename = ss.str();

    //hhd_msg_set_ai(clnt_sock,0); //停止ai

    LOGI("[%s] Receiving file: %s (Size: %.2f KB) axis %d file_status %d sn %ld", Config::getClientIP(clnt_sock).c_str(),
           full_filename.c_str(), (double)header.file_size/(1024),header.axis,header.file_status,header.sn);

    
    
    //LOGI("车牌：%s",header.result);

    bool is_license_plate = false;
    if (strstr(header.result, "0 0 0 0 0 0 0") == NULL) {

        // 分析结果字符串，判断是否为有效车牌
        is_license_plate = true;
        parse_license_plate(header.result, clnt_sock);

    } 

    FileHeader header_ack;
    header_ack.magic = MAGIC_NUMBER,
    header_ack.file_status = 1;
    header_ack.sn = header.sn+1;
    strcpy(header_ack.filename, full_filename.c_str());
    header_ack.file_size = header.file_size;
    header_ack.crc32 = header.crc32;
    send(clnt_sock,&header_ack,sizeof(header_ack),0);
    // 接收文件内容
    if(header.file_status==0){
        return 0;
    }
    FILE *fp = fopen(full_filename.c_str(), "wb");
    if(fp == NULL) {
        LOGE("[Error] Failed to create file %s", full_filename.c_str());
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    size_t total_bytes = 0;
    uint32_t calculated_crc = crc32(0L, Z_NULL, 0);
    time_t start_time = time(NULL);

    while(total_bytes < header.file_size) {
        size_t remaining = header.file_size - total_bytes;
        size_t chunk_size = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;
        
        bytes_received = recv(clnt_sock, buffer, chunk_size, MSG_WAITALL);
        if(bytes_received <= 0) break;

        size_t written = fwrite(buffer, 1, bytes_received, fp);
        if(written != bytes_received) {
            fclose(fp);
            return -1;
        }
        calculated_crc = crc32(calculated_crc, (Bytef*)buffer, bytes_received);
        total_bytes += bytes_received;
        
        printf("\r[Progress] %.2f/%.2f KB (%.1f%%) @ %.1f KB/s", 
              (double)total_bytes/(1024),
              (double)header.file_size/(1024),
              ((double)total_bytes/header.file_size)*100,
              (double)total_bytes/(1024)/difftime(time(NULL), start_time));
        fflush(stdout);
    }

    // 校验结果
    double elapsed = difftime(time(NULL), start_time);
    LOGD("\n[Status] Transfer completed in %.1f seconds (%.1f KB/s)", 
          elapsed, (double)total_bytes/(1024)/elapsed);

    if(calculated_crc == header.crc32) {
        LOGI("[Success] File integrity verified (CRC32: 0x%08X)", calculated_crc);
        #if TARGET_MODE == 2
        if (!is_license_plate)
        {
           g2d.mark_client_completed(clnt_sock);
        }
        
        #endif
       
    } else {
        LOGE("[Error] CRC mismatch! (Expected: 0x%08X, Actual: 0x%08X)", 
              header.crc32, calculated_crc);
    }

    fclose(fp);
    return 0;
}


// 发送报文测试
int  ListeningSocket::hhd_test(int sock)
{
    uint8_t data[350];
	msg_cmd_package_t cmd_pkt;
    int ret =0;

    memset(data, 0, sizeof(data));
    cmd_pkt.type = MSG_SET_AI;
    sprintf(cmd_pkt.cmd_data, "%d",1);
    cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    g_mcu_id = g_mcu_id;   //发送方ID nrf5340为0，T31为1
    ret= hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
                        (void *)&cmd_pkt,  \
                        cmd_pkt.cmd_data_len +2+2,\
                        NULL, 0,sock);

	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_REMOTE_CAPTRUE;
	// strcpy(cmd_pkt.cmd_data   , "121.15.156.83:7271,/home/photograph,test,hhd@123.com,P1_0001_240308131812_0000.jpeg");
    // cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
	// printf("拍照测试指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");

    // memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_SET_TIME;
	// strcpy(cmd_pkt.cmd_data, "20250930150000");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("测试录像指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");


	
	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_VIDEO_ON;
	// strcpy(cmd_pkt.cmd_data, "0");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("测试录像指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");


    

	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_OPEN_RTMP_MODE;
	// strcpy(cmd_pkt.cmd_data, "1,rtmp://129.211.172.129:1935/live/535073201769");
    // //strcpy(cmd_pkt.cmd_data, "1,rtmp://119.23.107.23:1930/live");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("开启推流指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");
    // //serial_parse_do(data,cmd_pkt.cmd_data_len);
	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_OPEN_RTMP_MODE;
	// strcpy(cmd_pkt.cmd_data, "1,rtmp://sdzt.hhdlink.online:1935/live/535073983069");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("开启推流指令2 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");
	

	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_OPEN_RTMP_MODE;
	// strcpy(cmd_pkt.cmd_data, "1,sdzt.hhdlink.online:1935/live/535073983069");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("开启推流指令3 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");

	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_OPEN_RTMP_MODE;
	// strcpy(cmd_pkt.cmd_data, "1,rtmp://36.109.104.43:1935/live/535073983069");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("开启推流指令3 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);

    // printf("\n");


	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_OPEN_RTMP_MODE;
	// strcpy(cmd_pkt.cmd_data, "1,rtmp://sdzt.hhdlink.online:1935/live");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("开启推流指令3 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);

    // printf("\n");

	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_CLOSE_RTMP_MODE;
	// strcpy(cmd_pkt.cmd_data, "0");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("关闭推流指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");




	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_OTA;
	// strcpy(cmd_pkt.cmd_data, "121.15.156.83:7271,/home/t31,test,hhd@123.com,3,t31x_nor_v0_60_240315.img");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("测试OTA指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");

	
	


	// memset(data, 0, sizeof(data));
	// cmd_pkt.type = MSG_VERSION;
	// strcpy(cmd_pkt.cmd_data, "0");
	// cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("版本查询指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
	// 							(void *)&cmd_pkt,  \
	// 							cmd_pkt.cmd_data_len +2+2,\
	// 							NULL, 0, sock);
    // printf("\n");



    // memset(data, 0, sizeof(data));
    // cmd_pkt.type = MSG_SET_IP;
    // strcpy(cmd_pkt.cmd_data, "192.168.30.214,192.168.30.254,ETH,STATIC");
    // cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("设置IP指令 %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
    //                                 (void *)&cmd_pkt,  \
    //                                 cmd_pkt.cmd_data_len +2+2,\
    //                                 NULL, 0,sock);
    // printf("\n");

    // memset(data, 0, sizeof(data));
    // cmd_pkt.type = MSG_SET_COORDINATE;
    // strcpy(cmd_pkt.cmd_data, "1.1");
    // cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("MSG_SET_COORDINATE %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
    //             (void *)&cmd_pkt,  \
    //             cmd_pkt.cmd_data_len +2+2,\
    //             NULL, 0,sock);
    // printf("\n");
        


    // memset(data, 0, sizeof(data));
    // cmd_pkt.type = MSG_SET_SOCK_IP_PORT;
    // strcpy(cmd_pkt.cmd_data, "192.168.30.171,8080");
    // cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    // printf("MSG_SET_SOCK_IP_PORT %s\n", cmd_pkt.cmd_data);
    // g_mcu_id = 0;   //发送方ID nrf5340为0，T31为1
    // hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
    //                             (void *)&cmd_pkt,  \
    //                             cmd_pkt.cmd_data_len +2+2,\
    //                             NULL, 0,sock);
    printf("\n");
    return ret;
}

int ListeningSocket::hhd_msg_set_command(int msg, int sock,int flag)
{
    uint8_t data[350];
	msg_cmd_package_t cmd_pkt;
    int ret =0;

    memset(data, 0, sizeof(data));
    cmd_pkt.type = msg;
    sprintf(cmd_pkt.cmd_data, "%d",flag);
    cmd_pkt.cmd_data_len = strlen(cmd_pkt.cmd_data);
    g_mcu_id = g_mcu_id;   //发送方ID nrf5340为0，T31为1
    ret= hhd_mcucom_package_send((uint8_t *)data, MCUCOM_MSG_CMD, \
                        (void *)&cmd_pkt,  \
                        cmd_pkt.cmd_data_len +2+2,\
                        NULL, 0,sock);
    return ret;
}

//发送报文
int ListeningSocket::hhd_mcucom_package_send(uint8_t *buff, mcucom_msg_type_t msg_type, void *send_buff, uint16_t size, 
                        mcucom_package_head_t *req_head, uint32_t timeout_ms,int sock)
{
    int ret = 0;
    try
    {
        int send_len;
        static uint16_t local_sn = 0;
        // uint16_t send_len = 0;

        mcucom_package_head_t * head = NULL;
        mcucom_package_t      * package = NULL;

        package = (mcucom_package_t *) buff;
        head    = &package->head;

        memset(head, 0, sizeof(*head));
        head->pack_head_flg = 0x55AAu;
        head->msg_type  = msg_type;
        head->src_sn    = 0;            // 响应报文的流水号
        head->sn        = local_sn++;   // 当前报文流水号（每发送一包自增）
        head->data_len  = size;
        head->mcu_id = g_mcu_id;

        memcpy(package->data, send_buff, size);

        send_len = sizeof(mcucom_package_head_t) + size;

        head->crc = bspal_mcucom_package_crc(package);

        LOGI( "%s Send package: [head: %04x DATA_LEN: %04x CRC: %02x MCU_ID: %02x SN: %04x SRC_SN: %04x TYPE: %02x SEND_LEN: %04x"
                            ,Config::getClientIP(sock).c_str(),head->pack_head_flg, head->data_len, head->crc, head->mcu_id, head->sn, head->src_sn, head->msg_type, send_len);
        // int j = 0;
        // for(j = 0; j < send_len; j++)
        // {
        //     if(j % 16 == 0)
        //     {
        //         printf("\n");
        //     }
        //     else if(j % 8 == 0)
        //     {
        //         printf("  ");
        //     }
        //     printf("%02x ", buff[j]);
        // }
        // printf("\n");
        ret =write(sock, buff, send_len);
        if (ret <= 0)
        {   
            if (errno == EPIPE || errno == ECONNRESET) {
                LOGI("连接已关闭");// 连接已关闭，无需处理
            }
            else
            {
                LOGE("发送报文失败");
            }
               
            remove_socket(sock);
        }

    }
    catch(const exception& e)
    {
        cerr <<"hhd_mcucom_package_send"<< e.what() << '\n';
    }
    
    
    return ret;
}

//解析报文crc
uint8_t ListeningSocket::bspal_mcucom_package_crc(mcucom_package_t *package)
{
    uint8_t * start = (uint8_t *)&package->head.crc + sizeof(package->head.crc);
    uint16_t len = package->head.data_len;
    len += sizeof(package->head.mcu_id);
    len += sizeof(package->head.sn);
    len += sizeof(package->head.src_sn);
    len += sizeof(package->head.msg_type);
    
    LOGI("len = %d\n",len);

    return Config::hhd_crc8(start, len);
}


const char*ListeningSocket::get_direction_string(uint8_t direction)
{
    switch (direction) {
        case 0: return "上方";
        case 1: return "左上";
        case 2: return "中上";
        case 3: return "右上";
        case 4: return "左中";
        case 5: return "中心";
        case 6: return "右中";
        case 7: return "左下";
        case 8: return "中下";
        case 9: return "右下";
        case 10: return "额外位置";
        default: return "未知方向";
    }
}

//开启两个服务器
bool ListeningSocket::Listen_2Server(int port1, int port2,int port3)
{
    /******************************************************/
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOGE("socket failed");
        return -1;
    }
    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,&optval, sizeof(optval));//端口复用
    
    // 初始化地址结构体
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port1);

    // 绑定socket
    if (bind(server_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) {
        LOGE("bind failed");
        return -1;
    }
    listen(server_fd, 50);


    /*****************第二个server*************************************/
    if ((serverfile_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOGE("第二个server failed");
        return -1;
    }


    setsockopt(serverfile_fd, SOL_SOCKET, SO_REUSEADDR,&optval, sizeof(optval));//端口复用
    
     // 初始化地址结构体
    file_address.sin_family = AF_INET;
    file_address.sin_addr.s_addr = INADDR_ANY;
    file_address.sin_port = htons(port2);

    // 绑定socket
    if (bind(serverfile_fd, (struct sockaddr *)&file_address, sizeof(file_address))) {
        LOGE("bind failed");
        return -1;
    }
    listen(serverfile_fd, 50);


    /*****************第三个server*************************************/
    if ((server_Video = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOGE("第二个server failed");
        return -1;
    }


    setsockopt(server_Video, SOL_SOCKET, SO_REUSEADDR,&optval, sizeof(optval));//端口复用
    
     // 初始化地址结构体
    video_address.sin_family = AF_INET;
    video_address.sin_addr.s_addr = INADDR_ANY;
    video_address.sin_port = htons(port3);

    // 绑定socket
    if (bind(server_Video, (struct sockaddr *)&video_address, sizeof(video_address))) {
        LOGE("bind failed");
        return -1;
    }
    listen(server_Video, 50);

    LOGI("Server message on port %i", port1);
    LOGI("Server file_fd on port %i", port2);
    LOGI("Server video on port %i", port3);
    return 1;
}

//传输文件客户端
int ListeningSocket::Accept_clientfile()
{
   
    socklen_t addrlen = sizeof(file_address);
    int clientfile_sock =accept(serverfile_fd, reinterpret_cast<sockaddr*>(&file_address), &addrlen);
    if (clientfile_sock == -1)
    {
        LOGE("clientfile_sock accept failed");
        return -1;
    }
   

    return clientfile_sock;
}

//传输文件客户端线程
void ListeningSocket::clientfile_ptrhread(int sock)
{
    int sock_copy;
     for (size_t i = 0; i < sockets.size(); i++) {
        if (sockets[i] == sock) {
            sock_copy =sockets[i];
            thread recvThread([this, sock_copy]() {
                Recv_handler(sock_copy);
            });
            recvThread.detach();
            printf("clientfile_ptrhread 创建线程成功\n");
            break;
        }
    }
}

//传输视频数据客户端
int ListeningSocket::Accept_clientVideo()
{
    socklen_t addrlen = sizeof(video_address);
    int clientvideo_sock =accept(server_Video, reinterpret_cast<sockaddr*>(&video_address), &addrlen);
    if (clientvideo_sock == -1)
    {
        LOGE("clientvideo_sock accept failed\n");
        return -1;
    }
    printf("clientvideo_sock succeed\n");

    return clientvideo_sock;
}

//接受传输视频数据线程
void ListeningSocket::clientVideo_ptrhread(int sock)
{
    int sock_copy;
    for (size_t i = 0; i < sockets.size(); i++) {
        if (sockets[i] == sock) {
            sock_copy =sockets[i];
            thread recvThread([this, sock_copy]() {
               Receive_h264_video(sock_copy);
            });
            recvThread.detach();
            printf("clientVideo_ptrhread 创建线程成功\n");
            break;
        }
    }
}

void ListeningSocket::addSocket(int sock, const string &client_ip, int server_type)
{
    lock_guard<mutex> lock(mtx);
        
    // 检查是否已存在该IP的连接信息
    if (ip_to_connection.find(client_ip) == ip_to_connection.end()) {
        LOGI("客户端 New connection: %s",client_ip.c_str());
        ip_to_connection[client_ip] = ClientConnection();
        ip_to_connection[client_ip].client_ip = client_ip;
    }
    
    ClientConnection& conn = ip_to_connection[client_ip];
    
    // 根据服务器类型存储对应的socket
    switch(server_type) {
        case 1: // 报文服务器
            conn.sock_msg = sock;
            break;
        case 2: // 文件服务器
            conn.sock_file = sock;
            break;
        case 3: // 视频服务器
            conn.sock_video = sock;
            LOGI("客户端 New connection: %s,sock=%d\n",client_ip.c_str(),conn.sock_video);
            break;
    }

    #if TARGET_MODE == 2
        g2d.set_scok_size(ip_to_connection.size());
    #endif
}

//报文客户端连接
int ListeningSocket::Accept_client()
{
    socklen_t addrlen = sizeof(serveraddr);
    int client_sock =accept(server_fd, reinterpret_cast<sockaddr*>(&serveraddr), &addrlen);
    if (client_sock == -1)
    {
        LOGE("client_sock accept failed");
        return -1;
    }
    return client_sock;
}

//报文客户端线程
void ListeningSocket::client_ptrhread(int sock)
{
    int sock_copy;
    for (size_t i = 0; i < sockets.size(); i++) {
        if (sockets[i] == sock) {
            sock_copy = sockets[i];
            thread sendThread([this, sock_copy]() {
                Send_handler(sock_copy);
            });
            sendThread.detach();

            thread recvThread([this, sock_copy]() {
                netctrl_recv(sock_copy);//接收数据报文线程
            });
            recvThread.detach();


            thread heartThread([this, sock_copy]() {
                heart_handler(sock_copy);//发送心跳线程
            });
            heartThread.detach();
            printf("client_ptrhread 创建线程成功\n");
            break;
        }
    }
}

void ListeningSocket::SetConfig(string config, int sock)
{
}   

int ListeningSocket::SocketBase(int sock)
{
    return client_fd = sock;
}

void ListeningSocket::jt808_init()
{
   
    jt808.jt808_init();
}

void ListeningSocket::gb28181_init()
{
    gb.gb28181_client_start();
}
