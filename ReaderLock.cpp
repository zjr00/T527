#include "ReaderLock.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "config.h"
#define MAX_CMDBUFF_LEN 128

ReaderLock::ReaderLock()
{
    server_fd =-1;
    m_RecvLen = 0;
    m_bResult =false;
    // 初始化互斥锁和条件变量
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
   
    m_nHaveReq = false;
    
}

ReaderLock::~ReaderLock()
{
    // 销毁互斥锁和条件变量
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void ReaderLock::TCPRecvThread()
{
    unsigned char buffer[1024] = {0};
    unsigned char cmdType;
    int bytes_received =0;
    int c = 0;
    memset(m_recvData, 0,sizeof(m_recvData));
    while (server_fd != -1)
    {
        memset(buffer,0,sizeof(buffer));
        bytes_received =0;
        bytes_received = recv(server_fd, buffer, sizeof(buffer), 0);  
        if (bytes_received > 0)
        {
            // if (m_recvData == NULL || buffer == NULL) {
            //     printf("Invalid buffer pointer!\n");
            //     return;
            // }

            memcpy(m_recvData+m_RecvLen, buffer, bytes_received);//追加方式拷贝进去
            m_RecvLen +=bytes_received;
           
            c = *(m_recvData+18);//接收到返回锁的信息大小
            unsigned char onePack[MAX_CMDBUFF_LEN] = {0};
            if (m_recvData[0] == 0xAA && m_recvData[1] == 0x55)
            {
                if (m_RecvLen >19+c && c!=0x00)//说明接收到一个完整的报文信息
                {
                    
                    memcpy(onePack, m_recvData, m_RecvLen);
                    cmdType = *(onePack + 19);
                    if (cmdType == 0x00)
                    {
                        printf("找不到目标终端\n");
                        break;
                    }
                    else if (cmdType == 0x01)
                    {
                        printf("目标终端连接失败\n");
                        break;
                    }
                    else if (cmdType == 0x02)
                    {
                        printf("目标终端连接后超时无应答\n");
                        break;
                    }
                    else
                    {   
                        NetRecvOnePack(onePack, m_RecvLen);
                        
                        pthread_cond_signal(&cond);    
                    }

                    int packetLength = 19 + m_recvData[18];
                    memmove(m_recvData, m_recvData + packetLength, m_RecvLen - packetLength);
                    m_RecvLen -= packetLength;
                }              
            }
            else
            {
                if (m_RecvLen >2)
                {
                    memmove(m_recvData, m_recvData + 1, m_RecvLen - 1);
                    m_RecvLen--;
                }
            }  
        }
        else if(bytes_received == -1)
        {
            //printf("Connection closed...\n");
            break;
        }

        sleep(0.3);
    }
    
    return;
}



/*****************************************************************************
//	描述：		接收数据解析
//	输入参数：
//onePack -- 接收到的数据
//packLen	接收到的数据长度
*****************************************************************************/
void ReaderLock::NetRecvOnePack(unsigned char *onePack, int packLen)
{
    if (!m_nHaveReq)
		return;

    Config::Show(onePack,"接收到的数据");
    Aes_ECB *aes_Ecb = new Aes_ECB();
    unsigned char lockData[128] = {0};//存储需要解密的数据
    int lockLen = *(onePack + 18); // 加密的数据长度
    unsigned char data[128] = {0};//存储解密后的数据
    unsigned char UnEscape[128] = {0};//存储还原转义的数据

    hdd_protocol->UnEscapeData(onePack,UnEscape,packLen);
    //需要而外填充加密数据的长度
    int paddingNeeded =16 -lockLen%16;
    memcpy(lockData, UnEscape + 32, lockLen+paddingNeeded); // 拷贝锁的加密数据

   
    unsigned char cmdType;
    aes_Ecb->aesDecrypt((uint8_t *)"1234567890123456", lockData, data);//解密数据

    Config::Show(data,"解密后的原始数据");

    cmdType = *(data + 10);
    switch (cmdType)
    {
    case 0x00:
        strcpy(m_retInfo,"成功");
        break;
    case 0x01:
        strcpy(m_retInfo,"失败");
        break;
    case 0x02:
        strcpy(m_retInfo,"锁杆断开施封失败");
        break;
    default:
        break;
    }
    m_bResult =true;
    return ;
}



// 连接阅读器网口
int ReaderLock::TCPConnect(const char *IP, int Port)
{
    
    if (IP == NULL || Port >=65535)
    {
       return -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        printf("socket errr");
        return 0;
    }
    printf("套接字创建成功\n");

    // 2、连接服务器
    struct sockaddr_in saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr(IP);
    saServer.sin_port = htons(Port);
    if (connect(server_fd, (struct sockaddr *)&saServer, sizeof(saServer)) < 0)
    {
        printf("connet %s err\n", IP);
        close_sock();
        return 0;
    }
    else
    {
        printf("connect success\n");
        std::thread RecvThread(&ReaderLock::TCPRecvThread, this);
        RecvThread.detach();
    }
    return 0;
}

void ReaderLock::close_sock()
{
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
    }
}

void ReaderLock::TCPSend(const char *data, int len)
{
    if (send(server_fd, data, len, 0) == -1)
    {
        printf("send error\n");
        return;
    }
}

// 施封
bool ReaderLock::NetSealLock()
{
    memset(m_retInfo, 0, sizeof(m_retInfo));
    unsigned char cmd[MAX_CMDBUFF_LEN] = {0};
    unsigned char aesData[64] = {0};

    hdd_protocol->getSealLock(cmd, "280075725423");
    Config::Show(cmd,"需要发送的数据");

    send(server_fd, cmd, Config::GetCmdLen(cmd, sizeof(cmd)), 0);

    m_nHaveReq = true;
    if (wait_for_response(7000)) {
       
        printf("施封结果: %s\n",m_retInfo);
    } else {
        printf("Timeout! No data received.\n");
    }
    m_RecvLen = 0;   
    m_nHaveReq = false;
    m_bResult=false;
    return true;
}

// 解封
bool ReaderLock::NetUnsealLock()
{
    unsigned char cmd[MAX_CMDBUFF_LEN] = {0};
    hdd_protocol->getUnSealLock(cmd, "280075725423");

    Config::Show(cmd,"需要发送的数据");

    send(server_fd, cmd, Config::GetCmdLen(cmd, sizeof(cmd)), 0);

    m_nHaveReq = true;
    if (wait_for_response(7000)) {
       
        printf("解封结果: %s\n",m_retInfo);
    } else {
        printf("Timeout! No data received.\n");
    }
    m_RecvLen = 0;   
    m_nHaveReq = false;
    m_bResult=false;
    return true;
}

// 写业务数据
bool ReaderLock::NetWriteLock()
{
    unsigned char cmd[MAX_CMDBUFF_LEN] = {0};
    hdd_protocol->getSealLock(cmd, "CNHD0000008528");
    return true;
}

// 读业务数据
bool ReaderLock::NetReadLock()
{
    unsigned char cmd[MAX_CMDBUFF_LEN] = {0};
    hdd_protocol->getSealLock(cmd, "CNHD0000008528");
    return true;
}

bool ReaderLock::wait_for_response(int timeout_ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    pthread_mutex_lock(&mutex);
    
    while (!m_bResult)
    {
        if (pthread_cond_timedwait(&cond, &mutex, &ts) == ETIMEDOUT) {
            
            pthread_mutex_unlock(&mutex);
            return false; // 超时返回
        }
    }
    pthread_mutex_unlock(&mutex);
    return true; // 接收到数据
}
