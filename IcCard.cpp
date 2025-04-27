#include "IcCard.h"
#include "Public.h"
#include "config.h"
IcCard::IcCard()
{
    if (Config::Get("SystemDeviceSet","IcCard") == "1") {
		Connect();
        thread ICThread(&IcCard::ReadIcCardProc, this);
        ICThread.detach();
	}

}
IcCard::~IcCard()
{
}

void IcCard::Connect()
{
    int port = stoi(Config::Get("IcCard","Port").c_str());

    printf("ICCard server start!\n");
    thread serverThread(&IcCard::ServerThread, this);
    serverThread.detach();
}

void IcCard::ReadIcCardProc()
{
    while (true) {
		sleep(1000);
		if (Public::isReadIc) {
			OnTest(0);
			if (Config::Get("Communication", "DoubleICCard") == "1") 
			{
				OnTest(1);
			}		
		}
	}
	return;
}

void IcCard::ServerThread()
{
}

void IcCard::OnTest(int car)
{
}
