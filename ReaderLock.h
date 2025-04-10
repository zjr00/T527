#ifndef READERLOCK_H
#define READERLOCK_H
#include "hhdProtocol.h"
#include "Aes_ECB.h"
#include <pthread.h>
class ReaderLock {
public:
    ReaderLock();
	virtual ~ReaderLock();
    void TCPRecvThread();//接收数据线程
    int TCPConnect(const char *IP, int Port);//网口连接
	void close_sock();	//关闭套接字

    void TCPSend(const char *data, int len);    //发送数据

    bool NetSealLock();
    bool NetUnsealLock();
    bool NetWriteLock();
    bool NetReadLock();
    void NetRecvOnePack(unsigned char *onePack, int packLen);//接收数据解析
protected:
    // 超时等待接收数据
    bool wait_for_response(int timeout_ms);
   
    int server_fd;
    HddProtocol *hdd_protocol;
    char m_retInfo[256];
    unsigned char m_recvData[1024];
    int m_RecvLen = 0;
    bool m_bResult;
    bool m_nHaveReq;
    bool signaled; // 事件状态
    pthread_mutex_t mutex; // 互斥锁
    pthread_cond_t cond; // 条件变量
    struct timespec timeout;
    
};


#endif