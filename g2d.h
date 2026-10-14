#ifndef G2D_H
#define G2D_H
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <filesystem>

#include <jpeglib.h>
#include "jpeglib.h"
#include "jerror.h"

#include "config.h"
#include "sunxiMemInterface.h"
#include "G2dApi.h"

#include <libyuv.h>

#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <functional>


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

    void mark_client_completed(int sockfd);
    void set_scok_size(int size);

   
private:
    bool is_image_file(const string& filename);
    int allocPicMem(paramStruct_t*pops, int size);
    void NV21_TO_RGB24(unsigned char *data, unsigned char *rgb, int width, int height);
    void save_yuv_file(unsigned char* yuv_data, int width, int height);
    
    int freePicMem(paramStruct_t*pops);
    int extract_index(const string& filename);

    void timer_thread_func();
    void start_montage_timer();
    void stop_timer_thread();
    void check_all_clients_completed();
    void reset_state();
    
    

    
    int iWidth = 1920, iHeight = 1080;
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
    paramStruct_t* m_DispMemOpsArray[9] = {
        &m_DispMemOps0,
        &m_DispMemOps1, 
        &m_DispMemOps2,
        &m_DispMemOps3,
        &m_DispMemOps4,
        &m_DispMemOps5,
        &m_DispMemOps6,
        &m_DispMemOps7,
        &m_DispMemOps8
    };
    bool is_first =false;
    vector<string> image_paths;
    string pcompPicPath0;
    struct timeval t1, t2;
    std::mutex clients_mutex;
    vector<int> completed_clients;      // 已完成文件传输的客户端
    int sock_size = 0;
    chrono::steady_clock::time_point first_completion_time;
    atomic<bool> montage_triggered{false};
    thread timer_thread;
    condition_variable timer_cv;
    std::mutex timer_mutex;
    atomic<bool> stop_timer{false};
    atomic<bool> is_resetting{false};//判断是否在拼图

    FtpTask ftp;
    
};


#endif