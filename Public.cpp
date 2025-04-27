#include "Public.h"
#include "config.h"

int Public::m_SystemDeviceSet = 0;
bool Public::isReadRfidCarNum = false;
bool Public::isReadWeigh = false;
bool Public::isReadIc =false;
string Public::weightStr = "";
int Public::isReadWeighCount =0;
Public::Public()
{
	
}

Public::~Public()
{
}

void Public::Init()
{
    if (Config::Get("SystemDeviceSet", "CarNum") == "1") {
		m_SystemDeviceSet |= 0x01;
	}

	if (Config::Get("SystemDeviceSet", "ECarNum") == "1") {
		m_SystemDeviceSet |= 0x02;
	}

	if (Config::Get("SystemDeviceSet","Weigh")== "1") {
		m_SystemDeviceSet |= 0x04;
	}

	// if (Config::Get("SystemDeviceSet","Container") == "1") {
	// 	//m_SystemDeviceSet |= 0x06;
	// }

	if (Config::Get("SystemDeviceSet","Radar") == "1") {
		m_SystemDeviceSet |= 0x08;
	}

	if (Config::Get("SystemDeviceSet","IcCard") == "1") {
		m_SystemDeviceSet |= 0x10;
	}

	cout<<"m_SystemDeviceSet"<<m_SystemDeviceSet<<endl;
	Public::AddValueGatherInfo("Radar","");
}

void Public::AddValueGatherInfo(string key, string value)
{
   
    if (key == "VE_NAME" || key == "Radar") {
		printf("开始\n");
		if (m_SystemDeviceSet & 0x04) //开始读取地磅数据
		{
			isReadWeigh = true;
			weightStr ="";
			isReadWeighCount =0;
		} 

			isReadRfidCarNum = true;

		if (m_SystemDeviceSet & 0x10)  //开始读取ic卡
		{
			
		}
    }
    else if (key == "CAR_EC_NO")//获取到电子车牌
    {
		isReadRfidCarNum = false;
		cout<<"读到电子车牌号: "<<value <<endl;
    }
    else if (key == "DR_IC_NO") //获取到ic卡
    {
	}
	else if (key == "GROSS_WT") //获取到地磅
    {
		isReadWeigh = false;
		weightStr ="";
		isReadWeighCount = 0;

		cout<<"读到地磅重量: "<<value <<endl;
	}
}


