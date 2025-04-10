#include "Aes_ECB.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include "config.h"

Aes_ECB::Aes_ECB()
{
}

Aes_ECB::~Aes_ECB()
{
}

int Aes_ECB::loadStateArray(uint8_t (*state)[4], const uint8_t *in)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            state[j][i] = *in++;
        }
    }
    return 0;
}

int Aes_ECB::storeStateArray(uint8_t (*state)[4], uint8_t *out)
{
    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
		{
            *out++ = state[j][i];
        }
    }
    return 0;
}

//密钥扩展
int Aes_ECB::keyExpansion(const uint8_t *key, uint32_t keyLen, AesKey *aesKey)
{
    uint32_t *w = aesKey->eK;  //加密密钥 
    uint32_t *v = aesKey->dK;  //解密密钥 

    //扩展密钥长度44=4*(10+1)个字,原始密钥128位，4个32位字，Nb*(Nr+1) 

    /* W[0-3],前4个字为原始密钥 */
    for (int i = 0; i < 4; ++i)
	{
        LOAD32H(w[i], key + 4*i);
    }

    /* W[4-43] */
    //temp=w[i-1];tmp=SubWord(RotWord(temp))xor Rcon[i/4] xor w[i-Nk]
    for (int i = 0; i < 10; ++i)
	{
        w[4] = w[0] ^ MIX(w[3]) ^ rcon[i];
        w[5] = w[1] ^ w[4];
        w[6] = w[2] ^ w[5];
        w[7] = w[3] ^ w[6];
        w += 4;
    }

    w = aesKey->eK+44 - 4;
    //解密密钥矩阵为加密密钥矩阵的倒序，方便使用，把ek的11个矩阵倒序排列分配给dk作为解密密钥
    //即dk[0-3]=ek[41-44], dk[4-7]=ek[37-40]... dk[41-44]=ek[0-3]
    for (int j = 0; j < 11; ++j)
	{
        for (int i = 0; i < 4; ++i)
		{
            v[i] = w[i];
        }
        w -= 4;
        v += 4;
    }

    return 0;
}

//将一个字符串作为十六进制串转化为一个字节数组，字节间可用空格分隔，
//返回转换后的字节数组长度，同时字节数组长度自动设置
int Aes_ECB::Str2Hex(char *strSrc, unsigned char *hexData)
{
   int t, t1;
	int rlen = 0, len = strlen(strSrc);
	//data.SetSize(len/2);
	for(int i = 0;i < len;)
	{
		char l, h = strSrc[i];
		if( h == ' ')
		{
			i++;
			continue;
		}
		i++;
		if(i >= len)
			break;
		l = strSrc[i];
		t = HexChar(h);
		t1 = HexChar(l);
		if((t == 16)||(t1 == 16))
			break;
		else 
			t = t * 16 + t1;
		i++;
		hexData[rlen] = (char)t;
		rlen++;
	}
	return rlen;
}


//转换为16进制字符
char Aes_ECB::HexChar(char c)
{
   if((c>='0')&&(c<='9'))
		return c-0x30;
	else if((c>='A')&&(c<='F'))
		return c-'A'+10;
	else if((c>='a')&&(c<='f'))
		return c-'a'+10;
	else 
		return 0x10;
}

// 轮密钥加
int Aes_ECB::addRoundKey(uint8_t (*state)[4], const uint32_t *key)
{
    uint8_t k[4][4];

    /* i: row, j: col */
    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
        {
            k[i][j] = (uint8_t) BYTE(key[j], 3 - i);  /* 把 uint32 key[4] 先转换为矩阵 uint8 k[4][4] */
            state[i][j] ^= k[i][j];
        }
    }

    return 0;
}

//字节替换
int Aes_ECB::subBytes(uint8_t (*state)[4])
{
    /* i: row, j: col */
    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
		{
            state[i][j] = S[state[i][j]]; //直接使用原始字节作为S盒数据下标
        }
    }

    return 0;
}

//逆字节替换
int Aes_ECB::invSubBytes(uint8_t (*state)[4])
{
    /* i: row, j: col */
    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
		{
            state[i][j] = inv_S[state[i][j]];
        }
    }
    return 0;
}

//行移位
int Aes_ECB::shiftRows(uint8_t (*state)[4])
{
    uint32_t block[4] = {0};

    /* i: row */
    for (int i = 0; i < 4; ++i)
	{
    //便于行循环移位，先把一行4字节拼成uint_32结构，移位后再转成独立的4个字节uint8_t
        LOAD32H(block[i], state[i]);
        block[i] = ROF32(block[i], 8*i);   //block[i]循环左移8*i位，如第0行左移0位 
        STORE32H(block[i], state[i]);
    }
    return 0;
}

//逆行移位
int Aes_ECB::invShiftRows(uint8_t (*state)[4])
{
    uint32_t block[4] = {0};

    /* i: row */
    for (int i = 0; i < 4; ++i) {
        LOAD32H(block[i], state[i]);
        block[i] = ROR32(block[i], 8*i);
        STORE32H(block[i], state[i]);
    }

    return 0;
}

/* Galois Field (256) Multiplication of two Bytes */
// 两字节的伽罗华域乘法运算
uint8_t Aes_ECB::GMul(uint8_t u, uint8_t v)
{
    uint8_t p = 0;

    for (int i = 0; i < 8; ++i)
	{
        if (u & 0x01)
		{
            p ^= v;
        }

        int flag = (v & 0x80);
        v <<= 1;
        if (flag)
		{
            v ^= 0x1B;
        }

        u >>= 1;
    }

    return p;
}

// 列混合
int Aes_ECB::mixColumns(uint8_t (*state)[4])
{
    uint8_t tmp[4][4];
    uint8_t M[4][4] = {{0x02, 0x03, 0x01, 0x01},
                       {0x01, 0x02, 0x03, 0x01},
                       {0x01, 0x01, 0x02, 0x03},
                       {0x03, 0x01, 0x01, 0x02}};

    /* copy state[4][4] to tmp[4][4] */
    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
		{
            tmp[i][j] = state[i][j];
        }
    }

    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
		{  //伽罗华域加法和乘法
            state[i][j] = GMul(M[i][0], tmp[0][j]) ^ GMul(M[i][1], tmp[1][j])
                        ^ GMul(M[i][2], tmp[2][j]) ^ GMul(M[i][3], tmp[3][j]);
        }
    }

    return 0;
}

// 逆列混合
int Aes_ECB::invMixColumns(uint8_t (*state)[4])
{
    uint8_t tmp[4][4];
    uint8_t M[4][4] = {{0x0E, 0x0B, 0x0D, 0x09},
                       {0x09, 0x0E, 0x0B, 0x0D},
                       {0x0D, 0x09, 0x0E, 0x0B},
                       {0x0B, 0x0D, 0x09, 0x0E}};  //使用列混合矩阵的逆矩阵

    /* copy state[4][4] to tmp[4][4] */
    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
		{
            tmp[i][j] = state[i][j];
        }
    }

    for (int i = 0; i < 4; ++i)
	{
        for (int j = 0; j < 4; ++j)
		{
            state[i][j] = GMul(M[i][0], tmp[0][j]) ^ GMul(M[i][1], tmp[1][j])
                          ^ GMul(M[i][2], tmp[2][j]) ^ GMul(M[i][3], tmp[3][j]);
        }
    }

    return 0;
}



/*****************************************************************************
//	描述：		加密数据
//	输入参数：	
//input -- 明文
//key	初始密钥
//ct	保存加密后的报文
*****************************************************************************/
int Aes_ECB::aesEncrypt(const uint8_t *input, uint8_t *key, uint8_t *ct)
{
    if (NULL == key || NULL == input)
	{
        printf("Aes_ECB NULL input || key\n");
        return -1;
    }

    AesKey aesKey;
    uint8_t *pos = ct;
    const uint32_t *rk = aesKey.eK;  //加密密钥指针
    uint8_t out[BLOCKSIZE] = {0};
    uint8_t actualKey[16] = {0};
    uint8_t state[4][4] = {0};

    memcpy(actualKey, key, 16);
    keyExpansion(actualKey, 16, &aesKey);  // 密钥扩展

	// 使用ECB模式循环加密多个分组长度的数据
    for (int i = 0; i < 16; i += BLOCKSIZE)
	{
		// 把16字节的明文转换为4x4状态矩阵来进行处理
        loadStateArray(state, input);
        // 轮密钥加
        addRoundKey(state, rk);

        for (int j = 1; j < 10; ++j)
		{
            rk += 4;
            subBytes(state);   // 字节替换
            shiftRows(state);  // 行移位
            mixColumns(state); // 列混合
            addRoundKey(state, rk); // 轮密钥加
        }

        subBytes(state);    // 字节替换
        shiftRows(state);  // 行移位
        // 此处不进行列混合
        addRoundKey(state, rk+4); // 轮密钥加
		
		// 把4x4状态矩阵转换为uint8_t一维数组输出保存
        storeStateArray(state, pos);

        pos += BLOCKSIZE;  // 加密数据内存指针移动到下一个分组
        input += BLOCKSIZE;   // 明文数据指针移动到下一个分组
        rk = aesKey.eK;    // 恢复rk指针到秘钥初始位置
    }
    return 0;
}


/*****************************************************************************
//	描述：		解密数据
//	输入参数：	
//key	初始密钥
//ct    加密数据
//pt	保存解密后的报文
*****************************************************************************/
int Aes_ECB::aesDecrypt(const uint8_t *key, const uint8_t *ct, uint8_t *pt)
{
    printf("开始解密\n");
    AesKey aesKey;
    uint8_t *pos = pt;
    const uint32_t *rk = aesKey.dK;  //解密密钥指针
    uint8_t out[BLOCKSIZE] = {0};
    uint8_t actualKey[16] = {0};
    uint8_t state[4][4] = {0};

    if (NULL == key || NULL == ct || NULL == pt)
	{
        printf("param err.\n");
        return -1;
    }


    memcpy(actualKey, key, 16);
    keyExpansion(actualKey, 16, &aesKey);  //密钥扩展，同加密

    for (int i = 0; i < 16; i += BLOCKSIZE)
	{
        // 把16字节的密文转换为4x4状态矩阵来进行处理
        loadStateArray(state, ct);
        // 轮密钥加，同加密
        addRoundKey(state, rk);

        for (int j = 1; j < 10; ++j)
		{
            rk += 4;
            invShiftRows(state);    // 逆行移位
            invSubBytes(state);     // 逆字节替换，这两步顺序可以颠倒
            addRoundKey(state, rk); // 轮密钥加，同加密
            invMixColumns(state);   // 逆列混合
        }

        invShiftRows(state);  // 逆行移位
        invSubBytes(state);   // 逆字节替换
        // 此处没有逆列混合
        addRoundKey(state, rk+4);  // 轮密钥加，同加密

        storeStateArray(state, pos);  // 保存明文数据
        pos += BLOCKSIZE;  // 输出数据内存指针移位分组长度
        ct += BLOCKSIZE;   // 输入数据内存指针移位分组长度
        rk = aesKey.dK;    // 恢复rk指针到秘钥初始位置
    }
    return 0;
}

/*****************************************************************************
//	描述：		裁剪出需要加密的数据
//	输入参数：
//lockData -- 需要裁剪的报文
//savaData	保存加密后的报文
*****************************************************************************/
void Aes_ECB::Cropping(unsigned char *lockData, unsigned char *savaData)
{
     if (NULL == lockData || NULL == savaData)
	{
		return ;
	}
    uint8_t data[128] = {0};
    uint8_t tmp[128] = {0};
    memcpy(savaData,lockData,13);//拷贝需要加密之前的数据
    uint8_t meter = *(lockData + 4);//需要加密的大小

    // 获取当前时间点
    chrono::system_clock::time_point now = chrono::system_clock::now();
    // 转换为 time_t 类型（自1970年1月1日以来的秒数）
    time_t now_time_t = chrono::system_clock::to_time_t(now);
    // 转换为 tm 结构（包含年、月、日、时、分、秒等信息）
    tm* now_tm = localtime(&now_time_t);
    char chTime[6] = {0};
    sprintf(chTime, "%02d%02d%02d%02d%02d%02d", (now_tm->tm_year + 1900) % 100, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);
    
    Str2Hex(chTime, tmp);
    // 加入时间到明文数据中
    strncat((char *)tmp, (char *)lockData + 13,meter);

    Config::Show(tmp,"明文数据");

    //判断是否为16的倍数，不是则填充0x00到十六的倍数，是则增加16个0x00
    int paddingNeeded =16 -(meter + 6)%16;
    strncat((char*)tmp, supply,paddingNeeded);

    //加密
  
    aesEncrypt(tmp, (uint8_t *)"1234567890123456", data);

    Config::Show(data,"加密数据");
    //将加密后的完整报文填入
    savaData[3] = 0x80;//修改消息属性 0x00--》0x80 0x80表示加密
    savaData[4] = meter+6;//需要加密的原本数据+时间防伪码

    memcpy(&savaData[13], data, Config::GetCmdLen(data, sizeof(data)));
    memcpy(&savaData[13 + Config::GetCmdLen(data, sizeof(data))],(char *)lockData + 13+meter,2);//先拷贝需要加密之前的数据
    return ;
}