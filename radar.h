#ifndef RADAR_H
#define RADAR_H
#include <stdlib.h> //标准函数库定义
#include <unistd.h> //unix标准函数定义
#include <fcntl.h> //文件控制定义
#include <termios.h> //POSIX终端控制定义
#include <chrono>
#include <iomanip> 
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#define ttyAs "/dev/ttyAS4"
#define Baud B115200
#define GPIO_NUM 131
#define GPIO_BASE "/sys/class/gpio/"

using namespace std;

class Radar
{
private:
    int fd;
    string dir_path;
    string val_path;
public:
    Radar();
    ~Radar();

    bool Radar_Init();
    void write_OPen();
    void write_Close();
    void write_Obtain();

    int writeData(unsigned char *buffer,size_t bufferSize);
    int GetRadardData(char *buffer,size_t bufferSize);
};


#endif