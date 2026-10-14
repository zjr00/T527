#ifndef HDDPROTOCOL_H
#define HDDPROTOCOL_H

#include "Aes_ECB.h"
#include <string>
#include <vector>
#pragma pack(push, 1)  
typedef struct L666_msg_config
{
    uint32_t frame_header; //帧头+保留字段
    uint16_t command; //命令
    uint16_t     L666lenth; //内容长度
    /////////下面的是内容
    uint16_t crc16; //crc校验
    unsigned char time_increment[6]; //增量
    uint32_t number; //随机数
    unsigned char bluetooth_pass[8]; //蓝牙密码
    uint8_t parameter; //参数
    unsigned char time[7]; // 时间
    // 填充字段
    unsigned char padding[15];
    ///////内容结束
    uint16_t frame_end;
}L666_msg_config_t;



typedef struct Bluetooth_msg_config {
    uint16_t    pack_head_flg;          //报文起始标志
    uint16_t     bluetooth_lenth;                 //蓝牙长度
    uint8_t     method;                    //连接方式
    unsigned char bluetooth[12];				//蓝牙号
    L666_msg_config_t  lockData;//存储需要发送给锁的数据
    uint8_t  bbc;        //校验 除了起始的所有数据
} Bluetooth_msg_config_t;
#pragma pack(pop)


class HddProtocol {
public:
    HddProtocol();
    ~HddProtocol();
    int Hdd_Init();
    //拼接---施封加密报文
    int getSealLock(unsigned char * cmd,const char * lockNum);

    //拼接---解封加密报文
    int getUnSealLock(unsigned char * cmd,const char * lockNum); 

    //拼接---L666施封加密报文
    int L666_getSealLock(unsigned char * cmd,const char * lockNum); 

    //拼接---L666解封加密报文
    int L666_getUnSealLock(unsigned char * cmd,const char * lockNum); 


    void hhd_bcc(unsigned char *data, int len, unsigned char *checkData);
    void UnEscapeData(unsigned char* data, unsigned char *cmd,int len);//转义还原
private:
    int  L666_message(unsigned char * cmdData,uint8_t parameter,const char * pass,int len);//L666组包
    int  Bluetooth_message(unsigned char * cmdData,const char * lockNum);//蓝牙阅读器组包

    void copy_and_uppercase(unsigned char* dest, const char* src, size_t dest_size);

    bool GetLockIdByLockNum(unsigned char *lockId, const char *lockNum);
    unsigned char SunCheck(unsigned char *uBuff, unsigned char uBuffLen);
    void Crc16Ccitt(const unsigned char *data, int iLen, unsigned char *checkData);
    int EscapeData(unsigned char* data, unsigned char *cmd,int len);//转义
    

    uint32_t COMM_NTOHL(uint32_t data);

    std::string deviceName;
    std::string sealType;
    Aes_ECB aes;

    std::string L666_Aes;

};

#endif