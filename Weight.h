#ifndef WEIGHT_H
#define WEIGHT_H
#include <string>

class Weight
{
public:
    Weight();
    ~Weight();
private:
    void CommMscomm();
    void GetCurWeight();
    void OnTime();

    int m_fd = -1;
};


#endif