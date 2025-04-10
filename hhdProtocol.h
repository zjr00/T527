#ifndef HDDPROTOCOL_H
#define HDDPROTOCOL_H

#include "Aes_ECB.h"
#include <iostream>
#include <string>
#include <vector>


class HddProtocol {
public:
    HddProtocol();
    ~HddProtocol();
    //拼接---施封加密报文
    bool getSealLock(unsigned char * cmd,const char * lockNum);

    //拼接---解封加密报文
    bool getUnSealLock(unsigned char * cmd,const char * lockNum);


    void hhd_bcc(unsigned char *data, int len, unsigned char *checkData);
    void UnEscapeData(unsigned char* data, unsigned char *cmd,int len);//转义还原
private:
    bool GetLockIdByLockNum(unsigned char *lockId, const char *lockNum);
    void Crc16Ccitt(const unsigned char *data, int iLen, unsigned char *checkData);
    int EscapeData(unsigned char* data, unsigned char *cmd,int len);//转义
    
    std::string deviceName;
    std::string sealType;
    
    
};

#endif