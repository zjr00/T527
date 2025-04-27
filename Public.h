#ifndef PUBLIC_H
#define PUBLIC_H

#include <string>
#include <thread>
using namespace std;

class Public
{
public:
    Public();
    ~Public();
    static bool InitClient();
    static int ReceiveDataThread();
    static void Init();
    static void AddValueGatherInfo(string key, string value);
    static void GetCapState(unsigned char * onePack);
    static bool isReadRfidCarNum;
    static bool isReadWeigh;
    static int m_SystemDeviceSet;
    static string weightStr;
    static int isReadWeighCount;
    static bool isReadIc;
private:
    static int sSocket;
    static char A1;
	static char A2;
	static char B1;
	static char B2;

    static int m_cb_A1;
	static int m_cb_A2;
	static int m_cb_B1;
	static int m_cb_B2;
   
};


#endif