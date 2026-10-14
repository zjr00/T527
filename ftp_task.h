#ifndef FTP_TASK_H
#define FTP_TASK_H

#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <thread>
#include <pthread.h>
using namespace std;
class FtpTask {
public:

    FtpTask();
    ~FtpTask();
    void ftp_init();
    bool uploadFile(const string& remoteFile, const string& localFile);//下载
    bool downloadFile(const string& localFile, const string& remoteFile);//上传
    void Get_FTPconfig();

    void upload_file_start(const string localFile_path);

private:
    typedef struct image_header 
    {
        uint32_t ih_time;
        uint32_t reserve1;
        uint32_t reserve2;
        uint32_t reserve3;
        uint32_t ih_dlen;
        uint32_t ih_dcrc;
        uint32_t ih_hlen;
        uint32_t ih_hcrc;
    } image_header_t;

    uint32_t calculateFileCRC(const string& filename, size_t offset, size_t length,const string &dts_name);
    bool validateFirmware(const string& file_path);
    void download_thread();
    void upload_thread();
    void add_ftp_upload_count();
    void del_ftp_upload_count();
    void add_ftp_download_count();
    void del_ftp_download_count();

    void traverse_directory(const string& path);
    
    static int ftp_upload_work_count,ftp_download_work_count; //需要上传 和下载的数量
    pthread_mutex_t mx_ftp_work_count = PTHREAD_MUTEX_INITIALIZER; // 互斥锁
    string media_path; 
    string remot_filename;

    thread down_thread;
    thread up_thread;

    string ftp_ip;
    string ftp_user;
    string ftp_pass;
    string local_path;
    string remot_path; 
    int ftp_port;
};
#endif