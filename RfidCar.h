#ifndef RFIDCAR_H
#define RFIDCAR_H

#include <thread>

class RfidCar
{
public:
    RfidCar();
    ~RfidCar();

private:
    void ReadRfidCarNumProc();
    void Rfid_Connect();
    void readRFID();
    bool ifConnectReader = 0;//读写器是否连接，1---连接,0---没有连接
};

#endif