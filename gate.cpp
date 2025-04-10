#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include <filesystem>
#include "gate.h"
#include "config.h"

using namespace std;
gate::gate()
{
    frontGateOpen =false; //入口道闸状态
    rearGateOpen=false; //出口道闸状态
    vehicleDetected=false; //识别区域是否有车
    bool created = std::filesystem::create_directories(Config::Get("Path","Tmp"));
    if (created) {
        std::cout << "Directory created: " <<Config::Get("Path","Tmp")<< std::endl;
    }
}

gate::~gate()
{
}

//处理入口车辆进入
void gate::processEntry()
{
    if (vehicleDetected)
    {
        printf("识别区域有车\n");
        return ;
    }
    openFrontGate();
    sleep(3);
    closeFrontGate();
}

//处理出口车辆离开
void gate::processExit()
{
    if (!vehicleDetected)
    {
        printf("识别区域无车，不需要操作\n");
        return ;
    }
    openRearGate();
    sleep(3);
    closeRearGate();
}
void gate::openFrontGate()
{
    printf("开启入口道闸\n");
    NetUnLock();
    frontGateOpen =true;
    vehicleDetected =true;
}

void gate::closeFrontGate()
{
    printf("关闭入口道闸\n");
    rearGateOpen =false;
}

void gate::openRearGate()
{
    printf("开启出口道闸\n");
    rearGateOpen =true;
    vehicleDetected =false;
}



void gate::closeRearGate()
{
    printf("关闭出口道闸\n");
    frontGateOpen =false;
}

void gate::NetUnLock()
{
    reader.TCPConnect("192.168.30.198",8075);

    reader.NetUnsealLock();
    reader.close_sock();
}

string gate::GetIPC_Path(string direction)
{
    string folder_name = Config::GetTime_Path();
    folder_name+= Swith_IPC(direction);

    std::cout << "ipc path : " << folder_name << std::endl;
    //先创建日期文件夹
    bool created = std::filesystem::create_directories(folder_name);
    if (created) {
        std::cout << "Directory created: " << folder_name << std::endl;
    }

    return folder_name;
}

string gate::Swith_IPC(string direction)
{
    if(direction == "左上")
        return "IPC1";
    if(direction == "左下")
        return "IPC2";
    if(direction == "右上")
        return "IPC3";
    if(direction == "右下")
        return"IPC4";
}
