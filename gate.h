#ifndef GATE_H
#define GATE_H
#include "ReaderLock.h"

class gate
{
public:
    gate();
    ~gate();

    void processEntry();//处理车辆进入
    void processExit();//处理车辆离开

    string GetIPC_Path(string direction);
    string Swith_IPC(string direction);
private:
    void NetUnLock();
    void openFrontGate();//开启入口道闸
    void closeFrontGate();//关闭入口道闸
    void openRearGate();//开启出口道闸
    void closeRearGate();//关闭出口道闸

    ReaderLock reader;
    bool frontGateOpen; //入口道闸状态
    bool rearGateOpen; //出口道闸状态
    bool vehicleDetected; //识别区域是否有车

};


#endif