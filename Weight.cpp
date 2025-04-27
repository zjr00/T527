#include "Weight.h"
#include "Public.h"
#include "config.h"

#include <stdlib.h> //标准函数库定义
#include <unistd.h> //unix标准函数定义
#include <fcntl.h> //文件控制定义
#include <termios.h> //POSIX终端控制定义
#include <chrono>
#include <iomanip> 


Weight::Weight()
{
    if (Config::Get("SystemDeviceSet","Weigh")== "1") {
		CommMscomm();
	}
    thread weightThread(&Weight::OnTime, this);
    weightThread.detach();
}

Weight::~Weight()
{
    close(m_fd);
}

void Weight::CommMscomm()
{
    
    // 打开串口设备
    m_fd = open("/dev/ttyAS4", O_RDWR|O_NOCTTY|O_NDELAY);
    if (m_fd == -1) {
        std::cerr << "打开串口失败" << std::endl;
        return ;
    }

    // 配置串口参数
    struct termios options;
    if(tcgetattr(m_fd, &options) != 0) {
        std::cerr << "Error getting terminal attributes" << std::endl;
        close(m_fd);
        return ;
    }

    // 设置串口参数
    options.c_cflag |= (CLOCAL|CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag &= ~CSTOPB;
    options.c_cc[VTIME] = 0;
    options.c_cc[VMIN] = 0;

    // 应用配置
    tcflush(m_fd, TCIOFLUSH);
    if(tcsetattr(m_fd, TCSANOW, &options) != 0) {
        std::cerr << "Error setting terminal attributes" << std::endl;
        close(m_fd);
        return ;
    }
    std::cerr << "打开串口成功" << std::endl;
}

void Weight::GetCurWeight()
{
    unsigned char bufferR[256] = {0};
    int strRead = read(m_fd, bufferR, sizeof(bufferR));
    // std::cout << "Received " << strRead << " bytes [";
    // for(int i = 0; i < strRead; ++i) {
    //     std::cout << static_cast<int>(bufferR[i]);
    //     if(i != strRead-1) std::cout << " ";
      
    // }
    // std::cout << "]" << std::endl;
   // 数据有效性验证
    if(strRead < 12) { // 最小有效帧长度检查
        std::cerr << "Invalid data length: " << strRead << std::endl;
        return;
    }

    // 查找有效数据帧
    bool bLegalRecord = false;
    const int FRAME_LENGTH = 12;
    std::vector<unsigned char> validFrame;

    for(int nIndex = 0; nIndex <= strRead - FRAME_LENGTH; ++nIndex) {
        if(bufferR[nIndex] == 0x02 &&           // 起始符 STX
        bufferR[nIndex+1] == 0x2B &&         // 协议标识
        bufferR[nIndex+FRAME_LENGTH-1] == 0x03) // 结束符 ETX
        {
            validFrame.assign(bufferR+nIndex, bufferR+nIndex+FRAME_LENGTH);
            bLegalRecord = true;
            break;
        }
    }

    if(!bLegalRecord) {
        std::cerr << "No valid frame found" << std::endl;
        return;
    }

    // 提取重量数据（假设位置2-7为重量值）
    std::string strCurWeight;
    bool numberStarted = false;
    for(int i = 2; i < 8; ++i) { // 取6位重量数据
        unsigned char c = validFrame[i];
        if(c < 0x30 || c > 0x39) { // 非数字字符过滤
            std::cerr << "Invalid weight character: 0x" 
                    << std::hex << static_cast<int>(c) << std::endl;
            strCurWeight.clear();
            break;
        }
        
        if(!numberStarted && c != 0x30) { // 跳过前导零
            numberStarted = true;
        }
        
        if(numberStarted) {
            strCurWeight += static_cast<char>(c);
        }
    }

    // 空值处理
    if(strCurWeight.empty()) {
        strCurWeight = "0";
    }

    
  // 重量值稳定性检测
    if(!Public::weightStr.empty() && Public::weightStr == strCurWeight) {
        if(++Public::isReadWeighCount >= 3) { // 连续3次相同
            Public::AddValueGatherInfo("GROSS_WT", strCurWeight);
            std::cout << "Stable weight recorded: " << strCurWeight << std::endl;
            Public::isReadWeighCount = 0; // 重置计数器
        }
    } else {
        Public::isReadWeighCount = 0;
        Public::weightStr = strCurWeight;
    }
    
    // // 调试信息输出
    // std::cout << "Current Weight: " << strCurWeight 
    //       << " | Stable Count: " << Public::isReadWeighCount 
    //       << std::endl;
}

void Weight::OnTime()
{
    while (1)
    {
        sleep(1);
        if (Public::isReadWeigh)
        {
            GetCurWeight();
        }
    }
    
}
