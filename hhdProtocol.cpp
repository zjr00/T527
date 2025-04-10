#include "hhdProtocol.h"
#include <string.h>
#include "config.h"
HddProtocol::HddProtocol()
{
    
}

HddProtocol::~HddProtocol()
{

}

 /*****************************************************************************
//	描述：		施封加密报文
//	输入参数：	
//cmd	存储加密报文
//lockNum	锁蓝牙id
*****************************************************************************/
bool HddProtocol::getSealLock(unsigned char *cmd,const char * lockNum)
{
    if (NULL == cmd || NULL == lockNum)
	{
		return false;
	}
    Aes_ECB *aes_Ecb = new Aes_ECB();
    int cmdflag = 0;
    unsigned char cmdData[128]={0};//储存需要发送给阅读器的数据
    unsigned char lockData[64]={0};//存储需要发送给锁的数据
    unsigned char aesData[64]={0};//存储加密后的数据
    unsigned char lockId[7] = { 0 };

    int dataflag = 0; 
    cmdData[cmdflag++] = 0xAA;
    cmdData[cmdflag++] = 0x55;

    cmdData[cmdflag++] = 0x00;
    cmdData[cmdflag++] = 0x0D;//连接方式+蓝牙设备号的长度

    cmdData[cmdflag++] = 0x00;//连接方式---蓝牙
   

    memcpy(&cmdData[cmdflag], lockNum, 12);//蓝牙设备号
    cmdflag+=12;

/***********************开始写入数据内容，数据内容是需要让阅读器发送给锁的数据*******************/
    lockData[dataflag++] = 0x7e;//帧头标识符
    lockData[dataflag++] = 0x03;
    lockData[dataflag++] = 0x10;//0310，通用指令
    lockData[dataflag++] = 0x00;
    lockData[dataflag++] = 0x04;//00 04参数长度
    
    bool bRet = GetLockIdByLockNum(lockId, lockNum);
    memcpy(&lockData[dataflag], lockId, 6);
	dataflag += 6;

    lockData[dataflag++] = 0x00;
    lockData[dataflag++] = 0x01;//00 01 流水号。可以自己填，也可以没发送一次指令就++;

    //如果要加密，在流水号后面添加时间防伪码，之后将时间防伪码和下面的数据一起加密
    lockData[dataflag++] = 0x01;//参数个数
    lockData[dataflag++] = 0x24;//发送的指令。
    lockData[dataflag++] = 0x01;//参数长度
    lockData[dataflag++] = 0x01;//01是施封
    hhd_bcc(lockData+1, dataflag,&lockData[dataflag]);
    dataflag += 1;
    lockData[dataflag++] = 0x7e;//帧尾标识符
    aes_Ecb->Cropping(lockData,aesData);

///////////////////////////////////end

    
   
    cmdData[cmdflag++] = 0x00;
    int aesDataLen = Config::GetCmdLen(aesData, sizeof(aesData));
    cmdData[cmdflag++] = aesDataLen;//需要发送给锁的加密数据长度
    int c=cmdflag -1;

    //需要发送给索的信息
    memcpy(&cmdData[cmdflag], aesData, aesDataLen);
    cmdflag += aesDataLen;

    int len = EscapeData(cmdData,cmd,cmdflag); //转义
    cmd[c] = aesDataLen + len-cmdflag;
    hhd_bcc(cmd+2, len, &cmd[len]);//添加校验码
    return true;
}

/*****************************************************************************
//	描述：		解封加密报文
//	输入参数：	
//cmd	存储加密报文
//lockNum	锁蓝牙id
*****************************************************************************/
bool HddProtocol::getUnSealLock(unsigned char *cmd,const char *lockNum)
{
    if (NULL == cmd || NULL == lockNum)
	{
		return false;
	}
    Aes_ECB *aes_Ecb = new Aes_ECB();
    
    int cmdflag = 0;
     unsigned char cmdData[128]={0};//储存需要发送给阅读器的数据
    unsigned char lockData[64]={0};//存储需要发送给锁的数据
    unsigned char aesData[64]={0};//存储加密后的数据

    int dataflag = 0;
    cmdData[cmdflag++] = 0xAA;
    cmdData[cmdflag++] = 0x55;

    cmdData[cmdflag++] = 0x00;
    cmdData[cmdflag++] = 0x0D;//连接方式+蓝牙设备号的长度

    cmdData[cmdflag++] = 0x00;//连接方式---蓝牙

    memcpy(&cmdData[cmdflag], lockNum, 12);//蓝牙设备号
    cmdflag+=12;
    

/***********************开始写入数据内容，数据内容是需要让阅读器发送给锁的数据*******************/
    lockData[dataflag++] = 0x7e;//帧头标识符
    lockData[dataflag++] = 0x03;
    lockData[dataflag++] = 0x10;//0310，通用指令
    lockData[dataflag++] = 0x00;
    lockData[dataflag++] = 0x04;//00 04参数长度
    unsigned char lockId[7] = { 0 };

    //锁号，先默认303030303030
    // for (size_t i = 0; i < 6; i++)
    // {
    //     lockData[dataflag++] = 0x30;;
    // }
    // bool bRet = GetLockIdByLockNum(lockId, lockNum);

    bool bRet = GetLockIdByLockNum(lockId, lockNum);
    memcpy(&lockData[dataflag], lockId, 6);
	dataflag += 6;

    lockData[dataflag++] = 0x00;
    lockData[dataflag++] = 0x01;//00 01 流水号。可以自己填，也可以没发送一次指令就++;

    //如果要加密，在流水号后面添加时间防伪码，之后将时间防伪码和下面的数据一起加密
    lockData[dataflag++] = 0x01;//参数个数
    lockData[dataflag++] = 0x24;//发送的指令。
    lockData[dataflag++] = 0x01;//参数长度
    lockData[dataflag++] = 0x00;//00是解封
    hhd_bcc(lockData, dataflag,&lockData[dataflag]);
    dataflag += 1;
    lockData[dataflag++] = 0x7e;//帧尾标识符

    printf("原始数据\n");
    for (int i = 0; i < 64; i++) {
        printf("%02X ", lockData[i]);
        if ((i + 1) % 16 == 0) {  // 每行打印16个字节
            printf("\n");
        }
    }
    cout<<"-------------------------"<<endl;
   
    aes_Ecb->Cropping(lockData,aesData);//加密数据

///////////////////////////////////end

    //蓝牙设备号
    cmdData[cmdflag++] = 0x00;
    int aesDataLen = Config::GetCmdLen(aesData, sizeof(aesData));
    cmdData[cmdflag++] = aesDataLen;//需要发送给锁的加密数据长度

    //拷贝需要发送给锁的信息
    memcpy(&cmdData[cmdflag], aesData, aesDataLen);
    cmdflag += aesDataLen;
    
    EscapeData(cmdData,cmd,cmdflag); //转义
    hhd_bcc(cmd+2, cmdflag, &cmd[cmdflag]);//校验
   
    return true;
}

//将锁id转化为数组
bool HddProtocol::GetLockIdByLockNum(unsigned char *lockId, const char *lockNum)
{
    if(NULL == lockId || NULL == lockNum)
	{
		return false;
	}
    char part1[7] ={0};

     for (size_t i = 0; i < 12; i += 2) {
        char pairStr[3] = {0};
        strncpy(pairStr, lockNum + i, 2);
        lockId[i / 2] = (uint16_t)strtol(pairStr, NULL, 16);  // 将字符串转换为16位整数
    }
	return true;
}

//crc16校验
void HddProtocol::Crc16Ccitt(const unsigned char *data, int iLen, unsigned char *checkData)
{
    unsigned int crc = 0xFFFF;	//initial value
	unsigned int polynomial = 0x1021;	//0001 0000 0010 0001 (0, 5, 12)
	for(int i = 0; i < iLen; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			char bit = ((data[i] >> (7 - j) & 1) == 1);
			char c15 = ((crc >> 15 & 1) == 1);
			crc <<= 1;
			if(c15 ^ bit)
				crc ^= polynomial;
		}
	}
	crc &= 0xFFFF;
	checkData[0] = crc >> 8;
	checkData[1] = crc & 0x00FF;
}

//bbc校验
void HddProtocol::hhd_bcc(unsigned char *data, int len,unsigned char *checkData)
{
    unsigned char value = 0;
    int i = 0;

    for(i = 0; i < len; i++)
    {
        value ^=  data[i];
    }
   checkData[0] = value;
}

/*****************************************************************************
//	描述：		转义数据
//	输入参数：
//data -- 需要转义的报文
//cmd	保存转义后的报文
//len  报文长度
*****************************************************************************/
int HddProtocol::EscapeData(unsigned char *data, unsigned char *cmd,int len)
{
    if (data == NULL || cmd == NULL) {
        printf("Invalid buffer pointer!\n");
        return -1;
    }

    int i = 0;
    int j = 0;
    for (i = 0; i < len -2 ; i++)
    {
        if (data[i] == 0x7e && i!=19)
        {
            cmd[j++] = 0x7D;
            cmd[j++] = 0x02;
        }
        else if (data[i] == 0x7D) {
            cmd[j++] = 0x7D;
            cmd[j++] = 0x01;
        } else {
            cmd[j++] = data[i];
        }
    }

    memcpy(&cmd[j], &data[len -2], 2);
    return j + 2;  //返回转义后的数据长度
}

/*****************************************************************************
//	描述：		还原转义数据
//	输入参数：
//data -- 需要还原转义的报文
//cmd	保存还原转义后的报文
//len  报文长度
*****************************************************************************/
void HddProtocol::UnEscapeData(unsigned char *data, unsigned char *cmd, int len)
{
    int i = 0;
    int j = 0;
    for (i = 0; i < len -3 ; i++)
    {
        if (data[i] == 0x7d)
        {
           if (data[i + 1] == 0x02) 
            {
                cmd[j++] = 0x7E;
                i++;
            } else if (data[i + 1] == 0x01) {
                cmd[j++] = 0x7D;
                i++;
            } 
        }
        else 
        {
            cmd[j++] = data[i];
        }
    }

    memcpy(&cmd[j], &data[len -3], 4);
}
