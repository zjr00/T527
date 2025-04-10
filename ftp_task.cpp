#include "ftp_task.h"
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <string.h>
#include <unistd.h>
using namespace  std;

int FtpTask::ftp_upload_work_count =0;
int FtpTask::ftp_download_work_count =0;
std::mutex FtpTask::mutex;
char *FtpTask::media_path = "/media/t31";


/*****************************************************************************
//	描述：		下载
//	输入参数：
//ip	需要下载的ip地址
//port	端口
//user	下载地址的用户名
//password	下载地址的用户名密码
//localFile	下载的文件路径
//remoteFile 下载的文件
*****************************************************************************/
bool FtpTask::downloadFile(std::string ip, int port, std::string user, std::string password, const std::string &remoteFile, const std::string &localFile)
{
    FILE *fp;
    struct stat file_stat;
	double file_size;
	clock_t  start_time, end_time;
	double upload_time;
	double upload_speed;
    string command;
    char buffer[1024];


	mutex.lock();
	ftp_download_work_count+=1;
	printf("ftp download work count %d\r\n",ftp_download_work_count);
	mutex.unlock();


     start_time = clock();

    ostringstream commandStream;
    commandStream << "ftpget " << ip << " -P " << port << " -u " << user
                  << " -p " << password << " " << localFile << " " << remoteFile << " 2>&1";
    command =commandStream.str();
	cout <<"ftp命令: "<< command <<endl;

    fp = popen(command.c_str(), "r");
	if (fp == NULL) {
		printf("Failed to run command \r\n");
		goto ERR_EXIT;
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL) 
	{
		printf("%s\r\n", buffer);
		printf("%s 下载失败\n", remoteFile.c_str());
		pclose(fp);
		goto ERR_EXIT;
	}
	// 记录结束时间
	end_time = clock();


	// 获取文件大小
	if (stat(localFile.c_str(), &file_stat) == -1)
	{
	   printf("Failed to get file size \r\n");
	   pclose(fp);
	   goto ERR_EXIT;
	}

	file_size = file_stat.st_size/1024;
	
	// 计算上传时间
	upload_time =  (end_time-start_time)/10000;
	
	// 计算上传速率
	upload_speed = (upload_time != 0) ? (file_size / upload_time) : file_size;


	printf("文件大小： %.2lf KB\r\n", file_size);
	printf("下载时间： %.2lf s\r\n", upload_time);
	printf("下载速率： %.2lf KBs\r\n", upload_speed);

	pclose(fp);

	mutex.lock();
	ftp_download_work_count-=1;
	printf("ftp download work count %d\r\n",ftp_download_work_count);
	mutex.unlock();
	return 0;

ERR_EXIT:
	mutex.lock();
	ftp_download_work_count-=1;
	printf("ftp download work count %d\r\n",ftp_download_work_count);
	mutex.unlock();
	return -1;
}




/*****************************************************************************
//	描述：		上传
//	输入参数：
//ip	需要上传的ip地址
//port	端口
//user	上传地址的用户名
//password	上传地址的用户名密码
//localFile	上传的路径
//remoteFile	需要上传的文件
*****************************************************************************/
bool FtpTask::uploadFile(std::string ip, int port, std::string user, std::string password, const std::string &localFile, const std::string &remoteFile)
{
    FILE *fp;
    struct stat file_stat;
	double file_size;
	clock_t  start_time, end_time;
	double upload_time;
	double upload_speed;
    //char command[300] = {0};
    string command;
    mutex.lock();
	ftp_upload_work_count+=1;
	printf("ftp upload work count %d\r\n",ftp_upload_work_count);
	mutex.unlock();

   
    start_time = clock();

    ostringstream commandStream;
    commandStream << "ftpput " << ip << " -P " << port << " -u " << user
                  << " -p " << password << " " << remoteFile << " " << localFile << " 2>&1";
    command =commandStream.str();
	cout <<"ftp命令: "<< command <<endl;


	fp = popen(command.c_str(), "r");
	if (fp == NULL) {
		printf("Failed to run command \r\n");
		return -1;
	}


    // 记录结束时间
	end_time = clock();


	// 获取文件大小
	if (stat(localFile.c_str(), &file_stat) == -1)
	{
	   	printf("Failed to get file size \r\n");
		mutex.lock();
		ftp_upload_work_count-=1;
		printf("ftp upload work count %d\r\n",ftp_upload_work_count);
		mutex.unlock();
	   	return -1;
	}
	file_size = file_stat.st_size/1024;

	
	// 计算上传时间
	upload_time =  (end_time-start_time)/10000;
	
	// 计算上传速率
	upload_speed = (upload_time != 0) ? (file_size / upload_time) : file_size;

	printf("file size   : %.2lf KB\r\n", file_size);
	printf("upload time : %.2lf s\r\n", upload_time);
	printf("upload speed: %.2lf KBs\r\n", upload_speed);

	pclose(fp);

    if(strncmp(localFile.c_str(), "./", 2) == 0)
	{
		if((strcmp(FtpTask::media_path, "/media/t31")==0))
		{
			char tmp_file[256];
			char * tmp_file_name = (char *)localFile.c_str()+2;
			sprintf(tmp_file, "%s/%s" , media_path, tmp_file_name);
			if(access(tmp_file, F_OK) == 0)
			{
				remove(tmp_file);
				system("sync");
			}
		}
	}

    // 删除本地文件
    if (remove(localFile.c_str()) == 0) {
        printf("file delete success\r\n");
    } else {
        printf("file delete failed\r\n");
    }

   	mutex.lock();
	ftp_upload_work_count-=1;
	printf("ftp upload work count %d\r\n",ftp_upload_work_count);
	mutex.unlock();
	return 0;
}