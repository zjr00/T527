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

/*功能：蓝牙阅读器------》锁*/

ReaderLock::ReaderLock()
{

}

ReaderLock::~ReaderLock()
{
    // 销毁互斥锁和条件变量
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

int ReaderLock::ReaderInit()
{
    server_fd =-1;
    m_RecvLen = 0;
    m_bResult =false;
    // 初始化互斥锁和条件变量
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
   
    m_nHaveReq = false;

    IP = Config::Get("Reader","ReaderIP");
    Port = atoi(Config::Get("Reader","ReaderPort").c_str());
    
    hdd_protocol.Hdd_Init();
    return 0;
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
                    //strcpy(m_retInfo,"0x01");
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

    Config::Show(onePack,"接收到的数据",packLen);
    Aes_ECB *aes_Ecb = new Aes_ECB();
    unsigned char lockData[128] = {0};//存储需要解密的数据
    int lockLen = *(onePack + 23); // 加密的数据长度
    unsigned char data[128] = {0};//存储解密后的数据
    unsigned char UnEscape[128] = {0};//存储还原转义的数据

    hdd_protocol.UnEscapeData(onePack,UnEscape,packLen);
    //需要而外填充加密数据的长度
    int paddingNeeded =16 -lockLen%16;
    //printf("lockLen = %d,paddingNeeded=%d\n",lockLen,paddingNeeded);
    memcpy(lockData, UnEscape + 32, lockLen+paddingNeeded); // 拷贝锁的加密数据
    
    Config::Show(lockData,"需要解密的数据",lockLen+paddingNeeded);
    unsigned char cmdType;
    aes_Ecb->aesDecrypt((uint8_t *)aes_Ecb->Aes_pass.c_str(), lockData, data);//解密数据

    //Config::Show(data,"解密后的原始数据");
    printf("解密后的原始数据\n");
    for (int i = 0; i < lockLen; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) {  // 每行打印16个字节
            printf("\n");
        }
    }
    cout<<"-------------------------"<<endl;
    cmdType = *(data + 10);
    switch (cmdType)
    {
    case 0x00:
        m_retInfo = 0x00;
        //strcpy(m_retInfo,"0x00");
        break;
    case 0x01:
    case 0x02:
        m_retInfo = 0x01;
        //strcpy(m_retInfo,"0x01");
        break;
    default:
        break;
    }
    m_bResult =true;
    return ;
}



// 连接阅读器网口
int ReaderLock::TCPConnect()
{
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
    saServer.sin_addr.s_addr = inet_addr(IP.c_str());
    saServer.sin_port = htons(Port);
    if (connect(server_fd, (struct sockaddr *)&saServer, sizeof(saServer)) < 0)
    {
        printf("connet ReaderLock  err\n");
        close_sock();
        return -1;
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
char ReaderLock::NetSealLock(const char * lockNum)
{
    //memset(m_retInfo, 0, sizeof(m_retInfo));
    unsigned char cmd[MAX_CMDBUFF_LEN] = {0};
    unsigned char aesData[64] = {0};
    int len =hdd_protocol.getSealLock(cmd, lockNum);                           
    //hdd_protocol.getSealLock(cmd, "487066731950");
    //hdd_protocol.getSealLock(cmd, "487066828913");
    Config::Show(cmd,"NetSealLock",len);

    if(!send(server_fd, cmd, Config::GetCmdLen(cmd, sizeof(cmd)), 0))
    {
        LOGE("发送给阅读器数据失败\n");
    }

    m_nHaveReq = true;
    if (wait_for_response(7000)) {
       
        printf("母锁施封结果: %d\n",m_retInfo);
    } else {
        LOGE("Timeout! No data received.\n");
    }
    m_RecvLen = 0;   
    m_nHaveReq = false;
    m_bResult=false;
    return m_retInfo;
}

// 解封
char ReaderLock::NetUnSealLock(const char * lockNum)
{
    unsigned char cmd[MAX_CMDBUFF_LEN] = {0};
    int len =hdd_protocol.getUnSealLock(cmd, lockNum);
    //hdd_protocol.getUnSealLock(cmd, "487066731950");
    //hdd_protocol.getUnSealLock(cmd, "487066828913");

    Config::Show(cmd,"NetUnSealLock",len);

    send(server_fd, cmd, Config::GetCmdLen(cmd, sizeof(cmd)), 0);

    m_nHaveReq = true;
    if (wait_for_response(7000)) {
       
        LOGI("母锁解封结果: %d\n",m_retInfo);
    } else {
        LOGE("Timeout! No data received.\n");
    }
    m_RecvLen = 0;   
    m_nHaveReq = false;
    m_bResult=false;
    return m_retInfo;
}

//L -666 施封
char ReaderLock::NetL666_SealLock(const char *lockNum)
{
    unsigned char cmd[MAX_CMDBUFF_LEN];
    memset(cmd, 0, MAX_CMDBUFF_LEN);
    int len = hdd_protocol.L666_getSealLock(cmd, lockNum);
    //hdd_protocol.getUnSealLock(cmd, "487066731950");
    //hdd_protocol.getUnSealLock(cmd, "487066828913");

    Config::Show(cmd,"NetL666_SealLock",len);

    send(server_fd, cmd, Config::GetCmdLen(cmd, sizeof(cmd)), 0);

    m_nHaveReq = true;
    if (wait_for_response(7000)) {
       
        LOGI("子锁施封结果: %d\n",m_retInfo);
    } else {
        LOGE("Timeout! No data received.\n");
    }
    m_RecvLen = 0;   
    m_nHaveReq = false;
    m_bResult=false;
    return m_retInfo;
}

// L -666解封
char ReaderLock::NetL666_UnSealLock(const char *lockNum)
{
    unsigned char cmd[MAX_CMDBUFF_LEN] = {0};
    int len = hdd_protocol.L666_getUnSealLock(cmd, lockNum);

    Config::Show(cmd,"NetL666_UnSealLock",len);

    send(server_fd, cmd, Config::GetCmdLen(cmd, sizeof(cmd)), 0);

    m_nHaveReq = true;
    if (wait_for_response(7000)) {
       
        LOGI("子锁解封结果: %d\n",m_retInfo);
    } else {
        LOGE("Timeout! No data received.\n");
    }
    m_RecvLen = 0;   
    m_nHaveReq = false;
    m_bResult=false;
    return m_retInfo;
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
