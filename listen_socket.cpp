
#include "listen_socket.h"
#include "config.h"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <iostream>
#include <ctime>
#include <sys/types.h>
#include <cmath>
ListeningSocket::ListeningSocket()
{
   //check_time();
    // std::thread jt(&ListeningSocket::jt808_init, this);
    // jt.detach();
}
ListeningSocket::~ListeningSocket()
{
    
}

// 设置服务器并进行监听
bool ListeningSocket::Listen(int port)
{
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("socket failed\n");
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
        printf("bind failed\n");
        return -1;
    }
    listen(server_fd, 50);
    printf("Server listening on port %i\n", port);

    
    return 1;
}

//等待客户端连接
int ListeningSocket::Accept()
{
    struct sockaddr_in addr = {0};
    socklen_t size = sizeof(addr);
    int client =accept(server_fd, reinterpret_cast<sockaddr*>(&addr), &size);
    if (client == -1)
    {
        printf("accept failed\n");
        return -1;
    }

    std::thread recvThread([this, client]() {
        Recv_handler(client);
    });
    recvThread.detach();

    // std::thread sendThread(&ListeningSocket::Send_handler,this);
    // sendThread.detach();

    return SocketBase(client);
}

time_t ListeningSocket::get_ntp_time(const char *host)
{
    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_INET; // 仅使用IPv4
    hints.ai_socktype = SOCK_DGRAM;
    
    int err = getaddrinfo(host, "ntp", &hints, &res);
    if (err != 0) {
        throw std::runtime_error(gai_strerror(err));
    }

    // 创建Socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        throw std::runtime_error("Socket creation failed");
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
        throw std::runtime_error("Send failed");
    }

    // 接收响应
    socklen_t addr_len = res->ai_addrlen;
    int len = recvfrom(sockfd, (char*)&packet, sizeof(packet), 0,
                      res->ai_addr, &addr_len);
    close(sockfd);
    freeaddrinfo(res);

    if (len < sizeof(packet)) {
        throw std::runtime_error("Invalid response");
    }

    // 转换网络字节序到主机字节序
    packet.trans_ts_sec = ntohl(packet.trans_ts_sec);
    
    // 计算UNIX时间戳（减去70年差值,）获取的是0时区的时间,需要转化为东八区
    return (time_t)(packet.trans_ts_sec - NTP_OFFSET +28800);
}

void ListeningSocket::check_time()
{
    try {
        // 尝试多个NTP服务器
        time_t ntp_time = 0;
        for (const auto& server : NTP_SERVERS) {
            try {
                ntp_time = get_ntp_time(server);
                std::cout << "成功从 " << server << " 获取时间: " 
                          << ctime(&ntp_time);
                break;
            } catch (const std::exception& e) {
                std::cerr << server << " 失败: " << e.what() << std::endl;
            }
        }

        if (ntp_time == 0) {
            throw std::runtime_error("所有NTP服务器连接失败");
        }

        // 计算本地时间与网络时间的差值
        time_t local_time = time(nullptr);
        std::cout << "本地时间: " << ctime(&local_time);
        std::cout << "时间差: " << difftime(ntp_time, local_time) << " 秒\n";

        if(difftime(ntp_time, local_time) != 0)
        {
            // 设置系统时间
            struct timeval tv;
            tv.tv_sec = ntp_time;
            tv.tv_usec = 0;
            if (settimeofday(&tv, NULL) != 0) {
                throw std::runtime_error("settimeofday failed (需要root权限)");
            }
            std::cout << "系统时间已更新\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return ;
    }
}

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
            std::lock_guard<std::mutex> lock(sockets_mutex);
            auto it = std::find(sockets.begin(), sockets.end(), sock);
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


int  ListeningSocket::Recv_handler(int socked)
{
    while (true)
    {
    // 接收协议头
        PacketHeader header{};
        if (!receive_full(socked, (char*)&header, sizeof(header))) {
        return -1;
        }

        // 接收数据部分
        std::vector<char> data(header.data_size);
        if (!receive_full(socked, data.data(), header.data_size)) {
            return -1;
        }


        switch (header.type) 
        {
            case 0: {
                std::string text(data.data(), data.size());
                std::cout << "[文本消息] " << text << std::endl;
                SetConfig(text.c_str(),socked);
                send_text(socked,"123");
                break;
            }
            case 1: {
                std::string filename = Config::Get("Path","Tmp");
                filename += Config::client_map[socked].direction_swith;
                filename += header.image_name;
                std::ofstream image_file(filename, std::ios::binary);
                if (image_file.write(data.data(), data.size())) {
                    std::cout << "[图片保存成功] " << filename << std::endl;
                } else {
                    std::cerr << "图片保存失败: " << filename  << "，错误: " << strerror(errno) << std::endl;
                }
                g2d.montage_jpeg();
                break;
            }
             case 2: {
                Gate.processEntry();
                break;
            }
            case 3: {
                Gate.processExit();
                break;
            }
            default:
                std::cerr << "未知数据类型" << std::endl;
        }
    }
}

void ListeningSocket::send_text(int sock, const std::string &text)
{
    PacketHeader header{};
    header.type = 0;
    header.data_size = text.size();
    
    // 发送协议头
    send(sock, (char*)&header, sizeof(header), 0);
    // 发送数据
    send(sock, text.data(), text.size(), 0);
}

void ListeningSocket::SetConfig(string config,int sock)
{
    lock_guard<mutex> lock(sockets_mutex);//不用管是否解锁

    size_t pos = config.find("@");
    string direction = config.substr(0, pos);
    string ip = config.substr(pos + direction.length());
    Config::SetToFile("IPC", direction,ip);
    
    ClientInfo new_client;
    new_client.ip_address = ip;
    new_client.direction = direction;
    new_client.path = Gate.GetIPC_Path(direction);
    new_client.direction_swith = Gate.Swith_IPC(direction);
    Config::client_map[sock] = new_client;
}   

int ListeningSocket::SocketBase(int sock)
{
    return client_fd = sock;
}

void ListeningSocket::jt808_init()
{
    //jt808.jt808_init();
}
