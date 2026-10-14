#include "hhdProtocol.h"
#include "config.h"


/*功能：蓝牙阅读器组包*/


HddProtocol::HddProtocol()
{
   
}

HddProtocol::~HddProtocol()
{
    
}

int HddProtocol::Hdd_Init()
{
    L666_Aes = Config::Get("Reader","L666ASE");
    
    return 0;
}

 /*****************************************************************************
//	描述：		施封加密报文
//	输入参数：	
//cmd	存储加密报文
//lockNum	锁蓝牙id
*****************************************************************************/
int HddProtocol::getSealLock(unsigned char *cmd,const char * lockNum)
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
    

///////////////////////////////////end

    
   
    cmdData[cmdflag++] = 0x00;
    aes_Ecb->Cropping(lockData,aesData);//加密
    int aesDataLen = Config::GetCmdLen(aesData, sizeof(aesData));
    cmdData[cmdflag++] = aesDataLen;//需要发送给锁的加密数据长度
    //cmdData[cmdflag++] = dataflag;
    int c=cmdflag -1;

    //需要发送给索的信息
    memcpy(&cmdData[cmdflag], aesData, aesDataLen);
    cmdflag += aesDataLen;

    // memcpy(&cmdData[cmdflag], lockData, dataflag);
    // cmdflag += dataflag;   

    int len = EscapeData(cmdData,cmd,cmdflag); //转义
    cmd[c] = aesDataLen + len-cmdflag;
    //cmd[c] = dataflag + len-cmdflag;
    hhd_bcc(cmd+2, len, &cmd[len]);//添加校验码
    len+=1;
    return len;
}

/*****************************************************************************
//	描述：		解封加密报文
//	输入参数：	
//cmd	存储加密报文
//lockNum	锁蓝牙id
*****************************************************************************/
int HddProtocol::getUnSealLock(unsigned char *cmd,const char *lockNum)
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
   
    cmdflag+=1;
    return cmdflag;
}

int HddProtocol::L666_getSealLock(unsigned char *cmd, const char *lockNum)
{
    if (NULL == cmd || NULL == lockNum)
	{
		return false;
	}

    unsigned char cmdData[128]={0};//储存需要发送给阅读器的数据
    int cmdflag,L_666; 

    cmdflag = Bluetooth_message(cmdData,lockNum);
    L_666 = L666_message(cmdData,0x31,"12345678",cmdflag+1);
    cmdData[cmdflag] = L_666-cmdflag-1;


    memcpy(cmd, cmdData, L_666);
    hhd_bcc(cmd+2, L_666, &cmd[L_666]);//校验
    L_666+=1;
    return L_666;
}

int HddProtocol::L666_getUnSealLock(unsigned char *cmd, const char *lockNum)
{
     if (NULL == cmd || NULL == lockNum)
	{
		return false;
	}

    unsigned char cmdData[128]={0};//储存需要发送给阅读器的数据
    int cmdflag,L_666; 

    cmdflag = Bluetooth_message(cmdData,lockNum);
    L_666 = L666_message(cmdData,0x30,"12345678",cmdflag+1);
    cmdData[cmdflag] = L_666-cmdflag-1;


    memcpy(cmd, cmdData, L_666);
    hhd_bcc(cmd+2, L_666, &cmd[L_666]);//校验
    L_666+=1;
    return L_666;
}

uint32_t HddProtocol::COMM_NTOHL(uint32_t data)
{
    int i;
    uint32_t tmp = 0;
    for(i = 0;i<4;i++)
    {
        tmp <<= 8;
        tmp |= (data>>8*i)&0xff;
    }
    return tmp;
}

/// @brief 组L666报文
/// @param cmdData 存储报文数组
/// @param parameter 参数id
/// @param pass 蓝牙密码
/// @param len 数组下标位置
/// @return 
int HddProtocol::L666_message(unsigned char *cmdData, uint8_t parameter, const char *pass, int len)
{

/****************************固定报文**********************************/
    //帧头+固定
    cmdData[len++] = 0xF1;
    cmdData[len++] = 0x1F;
    cmdData[len++] = 0xFF;
    cmdData[len++] = 0xee;

    //命令
    cmdData[len++] = 0x24;
    cmdData[len++] = 0x01;
/****************************end***************************************/

/****************************需要加密的报文******************************/


    unsigned char buffer_crc[32] = {0}; //计算crc校验
    unsigned char buffer_Aes[42] = {0}; //计算AES加密  
    int offset = 0;
    int AesLent= 0;



    // 获取当前时间点
    chrono::system_clock::time_point now = chrono::system_clock::now();
    // 转换为 time_t 类型（自1970年1月1日以来的秒数）
    time_t now_time_t = chrono::system_clock::to_time_t(now);
    // 转换为 tm 结构（包含年、月、日、时、分、秒等信息）
    tm* now_tm = localtime(&now_time_t);
    char chTime[13] = {0};
    char time_buffer[15] = {0};


 

    memcpy(buffer_crc + offset, pass, 8); //蓝牙密码
    offset += 8;

    buffer_crc[offset] = parameter;
    offset += 1;

    sprintf(time_buffer,"%04d%02d%02d%02d%02d%02d",now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);
    aes.Str2Hex(time_buffer, &buffer_crc[offset]); //时间
    offset+=7;
    
    char padding[15] ={0};
    int paddingNeeded =16 -((offset+12)%16); //需要填充的字节数
    if (paddingNeeded != 16) {
        memset(padding, 0, paddingNeeded);
        memcpy(buffer_crc + offset, padding, paddingNeeded);
    }
    
    offset +=paddingNeeded;
    unsigned char crc_result[2] = {0};
    Crc16Ccitt(buffer_crc,offset,crc_result);
   // Config::Show(buffer_crc,"校验:",offset);
/****************************end***************************************/

/****************************加密后的报文******************************/
    cmdData[len++] = 0x00;
    cmdData[len++] = offset+12;


    memcpy(buffer_Aes + AesLent, crc_result, 2); //crc校验
    AesLent += 2;

    sprintf(chTime, "%02d%02d%02d%02d%02d%02d", (now_tm->tm_year + 1900) % 100, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);
    aes.Str2Hex(chTime, &buffer_Aes[AesLent]); //增量
    AesLent += 6;

    //随机数
    buffer_Aes[AesLent++] = 0x31;
    buffer_Aes[AesLent++] = 0x32;
    buffer_Aes[AesLent++] = 0x33;
    buffer_Aes[AesLent++] = 0x34;
    
    memcpy(buffer_Aes+AesLent,buffer_crc,offset);
    AesLent += offset;
    
    Config::Show(cmdData,"加密前:",len);

  
    //aes.aesEncrypt((uint8_t *)buffer_Aes,AesLent,(uint8_t *)L666_Aes.c_str(),&cmdData[len]);
    len+=AesLent;
/****************************end***************************************/

/****************************效验和 帧尾******************************/
    
    cmdData[len++] = SunCheck(cmdData+21,len-21);
    // Config::Show(cmdData+20,"效验和:",len-20);
    cmdData[len++] =0xF2;
    cmdData[len++] =0x2F;
    return len;
}

/// @brief 蓝牙阅读器组包
/// @param cmdData 存储数组
/// @param lockNum 锁号
/// @return 
int HddProtocol::Bluetooth_message(unsigned char *cmdData, const char *lockNum)
{
    int cmdflag = 0; 

    cmdData[cmdflag++] = 0xAA;
    cmdData[cmdflag++] = 0x55;

    cmdData[cmdflag++] = 0x00;
    cmdData[cmdflag++] = 0x0D;//连接方式+蓝牙设备号的长度

    cmdData[cmdflag++] = 0x00;//连接方式---蓝牙

    memcpy(&cmdData[cmdflag], lockNum, 12);//蓝牙设备
    //copy_and_uppercase(&cmdData[cmdflag], lockNum, 12);
    cmdflag+=12;
    cmdData[cmdflag++] = 0x00;
    return cmdflag;
}

void HddProtocol::copy_and_uppercase(unsigned char *dest, const char *src, size_t dest_size)
{
    if (dest == nullptr || src == nullptr) return;
    
    size_t i = 0;
    // 复制并转换字符
    while (i < dest_size - 1 && src[i] != '\0') {
        if (src[i] >= 'a' && src[i] <= 'z') {
            dest[i] = src[i] - 32;  // 转换为大写
        } else {
            dest[i] = src[i];
        }
        i++;
    }
}

// 将锁id转化为数组
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

unsigned char HddProtocol::SunCheck(unsigned char *uBuff, unsigned char uBuffLen)
{
    //Config::Show(uBuff,"校验",uBuffLen);
    unsigned char i,uSum=0;
	for(i=0;i<uBuffLen;i++)
	{
		uSum = uSum + uBuff[i];
	}
	uSum = (~uSum) + 1;
    if(uSum > 0xf0)
		uSum -= 16;

	return uSum;
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
