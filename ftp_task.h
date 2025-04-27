#ifndef FTP_TASK_H
#define FTP_TASK_H

#include <iostream>
#include <string>
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
    bool downloadFile(const string& remoteFile, const string& localFile);//下载
    bool uploadFile(const string& localFile, const string& remoteFile);//上传
    
    
private:

    void download_thread();
    void upload_thread();
    void Get_FTPconfig();
    

    void add_ftp_upload_count();
    void del_ftp_upload_count();
    void add_ftp_download_count();
    void del_ftp_download_count();

    void traverse_directory(const string& path);
    static int ftp_upload_work_count,ftp_download_work_count; //需要上传 和下载的数量
    pthread_mutex_t mx_ftp_work_count = PTHREAD_MUTEX_INITIALIZER; // 互斥锁
    string media_path; 

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