#ifndef ICCARD_H
#define ICCARD_H

class IcCard
{
public:
    IcCard();
    ~IcCard();
private:
    void Connect();
    void ReadIcCardProc();
    void ServerThread();
    void OnTest(int car);
};

#endif