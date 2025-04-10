#ifndef FTP_TASK_H
#define FTP_TASK_H

#include <iostream>
#include <string>
#include <mutex>
#include <memory>
#include <vector>
#include <pthread.h>
class FtpTask {
public:
    static bool downloadFile(std::string ip, int port, std::string user, std::string password,const std::string& remoteFile, const std::string& localFile);//下载
    static bool uploadFile(std::string ip, int port, std::string user, std::string password,const std::string& localFile, const std::string& remoteFile);//上传
    
    
private:
    static int ftp_upload_work_count,ftp_download_work_count; //需要上传 和下载的数量
    static std::mutex mutex; // 互斥锁
    static char *media_path;
};
#endif