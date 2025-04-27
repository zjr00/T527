#ifndef G2D_H
#define G2D_H
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <filesystem>

#include "config.h"
#include "sunxiMemInterface.h"
#include "G2dApi.h"

#include "jpeglib.h"

#define LOG_TAG "g2d_test"

#include "sdklog.h"
#include <libyuv.h>
#include "ftp_task.h"
class G2d
{

public:
    G2d();
    ~G2d();
    int decode_jpeg(const char* filename,paramStruct_t*pops);
    void rgb_to_yuv420(unsigned char* rgb, unsigned char* yuv, int width, int height);
    void montage_jpeg();

    void Create_Path();
private:
    bool is_image_file(const std::string& filename);
    int allocPicMem(paramStruct_t*pops, int size);
    void NV21_TO_RGB24(unsigned char *data, unsigned char *rgb, int width, int height);
    int freePicMem(paramStruct_t*pops);



    int iWidth = 1920, iHeight = 1080 ;
    int iSubWidth = 640, iSubHeight = 360;
    paramStruct_t m_DispMemOps;
    paramStruct_t m_DispMemOps0;
    paramStruct_t m_DispMemOps1;
    paramStruct_t m_DispMemOps2;
    paramStruct_t m_DispMemOps3;
    paramStruct_t m_DispMemOps4;
    paramStruct_t m_DispMemOps5;
    paramStruct_t m_DispMemOps6;
    paramStruct_t m_DispMemOps7;
    paramStruct_t m_DispMemOps8;
    paramStruct_t m_DispMemOpsArray[9] = {
        m_DispMemOps0,
        m_DispMemOps1,
        m_DispMemOps2,
        m_DispMemOps3,
        m_DispMemOps4,
        m_DispMemOps5,
        m_DispMemOps6,
        m_DispMemOps7,
        m_DispMemOps8
    };
    std::vector<std::string> image_paths;
    string pcompPicPath0;
    struct timeval t1, t2;
    FtpTask ftp;
};


#endif