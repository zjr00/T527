#include "radar.h"
#include "config.h"

/*功能：雷达*/
Radar::Radar()
{
   
}

Radar::~Radar()
{
    close(fd);
}

bool Radar::Radar_Init()
{
    Config::initialize();
    Config::OPen_485Com();

    fd = Config::Get_fd();
    write_OPen();
    return true;
}

// 开启获取雷达报警
void Radar::write_OPen()
{
    unsigned char Openbuffer[] = {0xDF, 0x07, 0x04, 0x12, 0x11, 0x24, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x00};//开启
    writeData(Openbuffer,sizeof(Openbuffer));
}

//关闭获取雷达报警
void Radar::write_Close()
{
    unsigned char Closebuffer[] = {0xDF, 0x07, 0x04, 0x12, 0x11, 0x24, 0x00, 0x00, 0x00, 0x00, 0x4B, 0x00};//关闭
    writeData(Closebuffer,sizeof(Closebuffer));
}


//读取雷达报警开关状态
void Radar::write_Obtain()
{
    unsigned char Obtainbuffer[] = {0xDF, 0x07, 0x03, 0x11, 0x11, 0x24, 0x00, 0x00, 0x00, 0x00, 0x49, 0x00};//获取是否开启回复
    writeData(Obtainbuffer,sizeof(Obtainbuffer));
}

int Radar::writeData(unsigned char *buffer,size_t bufferSize)
{
    // 清空输出缓冲区
    tcflush(fd, TCOFLUSH);
    Config::write_gpio_value(1);
    // 发送数据
    ssize_t bytesWritten = write(fd, buffer, bufferSize);

    printf("发送的报文：\n");
    for (int  i = 0; i < bufferSize; i++)
    {   
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0)
        { // 每行打印16个字节
            printf("\n");
        }
    }
    
    if (bytesWritten < 0) {
        cerr << "发送错误: " << strerror(errno) << endl;
        return -1;
    } else if (bytesWritten != static_cast<ssize_t>(bufferSize)) {
        cerr << "部分数据发送: " << bytesWritten << "/" << bufferSize << " 字节" << endl;
        return bytesWritten;
    } else {
        cout << "成功发送 " << bytesWritten << " 字节数据" << endl;
        
        // 等待所有数据发送完成
        if (tcdrain(fd) < 0) {
            cerr << "等待发送完成错误: " << strerror(errno) << endl;
        }
        
        return bytesWritten;
    }
}

int Radar::GetRadardData(char *buffer,size_t bufferSize)
{
    Config::write_gpio_value(0);
    int strRead = read(fd, buffer, bufferSize);

    return strRead;
}