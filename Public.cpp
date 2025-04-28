#include "Public.h"
#include "config.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
int Public::m_SystemDeviceSet = 0;
bool Public::isReadRfidCarNum = false;
bool Public::isReadWeigh = false;
bool Public::isReadIc =false;
string Public::weightStr = "";
int Public::isReadWeighCount =0;
int Public::sSocket=0;
char Public::A1=0x30;
char Public::A2=0x30;
char Public::B1=0x30;
char Public::B2=0x30;

int Public::m_cb_A1=0;
int Public::m_cb_A2=0;
int Public::m_cb_B1=0;
int Public::m_cb_B2=0;

Public::Public()
{
	
}

Public::~Public()
{
}

bool Public::InitClient()
{

   	string serverIP =Config::Get("NetworkDriver", "serverIP");
	string serverPort=Config::Get("NetworkDriver", "serverPort");
    
    m_cb_A1 =atoi(Config::Get("Communication", "box_infrared1").c_str());
	m_cb_A2 =atoi(Config::Get("Communication", "box_infrared2").c_str());
	m_cb_B1 =atoi(Config::Get("Communication", "box_infrared3").c_str()); 
	m_cb_B2 =atoi(Config::Get("Communication", "box_infrared4").c_str());

	sSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (sSocket == -1)
    {
        printf("socket errr");
        return 0;
    }
	
	    // 2、连接服务器
    struct sockaddr_in saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr(serverIP.c_str());
    saServer.sin_port = htons(atoi(serverPort.c_str()));
    if (connect(sSocket, (struct sockaddr *)&saServer, sizeof(saServer)) < 0)
    {
        cout<<"IP: "<<serverIP<<"连接服务器失败" <<endl;
        close(sSocket);
        sSocket = -1;
        return 0;
    }
    else
    {
        printf("connect success\n");
        std::thread RecvThread(Public::ReceiveDataThread);
        RecvThread.detach();
    }
    return 0;
}

int Public::ReceiveDataThread()
{
	unsigned char buffer[256] = {0};
    unsigned char cmdType;
    int bytes_received =0;
    int c = 0;
    while (sSocket != -1)
    {
        memset(buffer,0,sizeof(buffer));
        bytes_received =0;
        bytes_received = recv(sSocket, buffer, sizeof(buffer), 0);  
        if (bytes_received > 0)
        {
            Config::Show(buffer,"红外数据");
            GetCapState(buffer);
        }
        else if(bytes_received == -1)
        {
            break;
        }

        sleep(0.3);
    }
    
    return 0;
}

void Public::Init()
{
	InitClient();
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

void Public::GetCapState(unsigned char * onePack)
{
    Config::Show(onePack,"红外数据2：");
    A1 = *(onePack+10 + (m_cb_A1*2-2));
    A2 = *(onePack+10 + (m_cb_A2*2-2));
    B1 = *(onePack+10 + (m_cb_B1*2-2));
    B2 = *(onePack+10 + (m_cb_B2*2-2));
    cout<<"m_cb_A1:"<<m_cb_A1<<" m_cb_A2:"<<m_cb_A2<<" m_cb_B1:"<<m_cb_B1<<" m_cb_B2:"<<m_cb_B2<<endl;
    cout<<"A1："<<A1<<" A2:"<<A2<<" B1:"<<B1<<" B2:"<<B2<<endl;
}
