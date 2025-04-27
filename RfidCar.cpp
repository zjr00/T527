#include "RfidCar.h"
#include "Public.h"
#include "config.h"
#include <stdio.h>

#include <string.h>
RfidCar::RfidCar()
{
    thread rfidThread(&RfidCar::ReadRfidCarNumProc, this);
    rfidThread.detach();
}

RfidCar::~RfidCar()
{
}

void RfidCar::ReadRfidCarNumProc()
{
    Rfid_Connect();

    while (true) {

		if (ifConnectReader ==1) {
			sleep(1);
			if (Public::isReadRfidCarNum) {
               
				readRFID();
			}
		}	
	}
}


void RfidCar::Rfid_Connect()
{
    string m_strIPAddress, m_strHostIPAddress;
    m_strIPAddress = Config::Get("ECarNum","Ip");
    m_strHostIPAddress = Config::Get("ECarNum", "HostIp");

    int m_Port = stoi(Config::Get("ECarNum","Port").c_str());
	int m_HostPort = stoi(Config::Get("ECarNum","HostPort").c_str());

    cout<<"m_strIPAddress:"<<m_strIPAddress<<" m_strHostIPAddress"<<m_strHostIPAddress<<" m_Port"<<m_Port<<" m_HostPort"<<m_HostPort<<endl;
    ifConnectReader =1;

}


void RfidCar::readRFID()
{
    string str;
    cout<<"开始获取电子车牌"<<endl;  
    Public::AddValueGatherInfo("CAR_EC_NO",str);
}
