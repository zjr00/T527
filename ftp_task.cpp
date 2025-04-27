#include "ftp_task.h"
#include "config.h"
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <string.h>
#include <unistd.h>
using namespace  std;

int FtpTask::ftp_upload_work_count =0;
int FtpTask::ftp_download_work_count =0;

FtpTask::FtpTask()
{
	// down_thread =thread(&FtpTask::download_thread,this);
	// up_thread =thread(&FtpTask::upload_thread,this);
	// down_thread.detach();
	// up_thread.detach();
}

FtpTask::~FtpTask()
{
	
}

void FtpTask::add_ftp_upload_count()
{
	pthread_mutex_lock(&mx_ftp_work_count);
    ftp_upload_work_count+=1;
    pthread_mutex_unlock(&mx_ftp_work_count);
}

void FtpTask::del_ftp_upload_count()
{
	pthread_mutex_lock(&mx_ftp_work_count);
	if(ftp_upload_work_count > 0)
    	ftp_upload_work_count-=1;
    pthread_mutex_unlock(&mx_ftp_work_count);
}


void FtpTask::add_ftp_download_count()
{
    pthread_mutex_lock(&mx_ftp_work_count);
    ftp_download_work_count+=1;
    pthread_mutex_unlock(&mx_ftp_work_count);
}

void FtpTask::del_ftp_download_count()
{
    pthread_mutex_lock(&mx_ftp_work_count);
    if(ftp_download_work_count > 0)
		ftp_download_work_count-=1;
    pthread_mutex_unlock(&mx_ftp_work_count);
}





void FtpTask::download_thread()
{
	//downloadFile("jsft.txt","jsft.txt");
}


void FtpTask::upload_thread()
{
	//media_path = Config::GetTime_Path();
	media_path = "./data";
	traverse_directory(media_path);
	//uploadFile("config.ini","config.ini");
}

void FtpTask::traverse_directory(const string& path)
{
	DIR *dir;
	struct dirent *entry;
	string ext;
	struct stat s;
	
	if ((dir = opendir(path.c_str())) == NULL) 
	{
		perror("opendir");
		dir = NULL;
		return ;
	}

	// 遍历目录条目
	while ((entry = readdir(dir)) != NULL) {
		
		string local_file = path + "/" + entry->d_name;
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
		{
			continue;
		}

		if (lstat(local_file.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) 
        {
            // 递归遍历子目录
            traverse_directory(local_file);
        }
		else
		{
			ext = strrchr(entry->d_name, '.'); // 从右向左查找最后一个`.`字符
			if (!ext.empty() && (ext==".h264" || ext==".h265" || ext==".jpeg" || ext==".jpg" )) 
			{
				
				cout<<"需要上传的文件名："<<local_file<<endl;
			}
		}
		
	}

	closedir(dir);
    return ;
}

void FtpTask::Get_FTPconfig()
{
	ftp_ip = Config::Get("FTP","FTP_IP");
    ftp_user = Config::Get("FTP","FTP_user");
    ftp_pass = Config::Get("FTP","FTP_pass");
    local_path = Config::Get("FTP","Local_path");
    remot_path = Config::Get("FTP","Remot_path");
    ftp_port = stoi(Config::Get("FTP","FTP_port").c_str());
}

/*****************************************************************************
//	描述：		下载
//	输入参数：
//localFile	下载的文件路径
//remoteFile 下载的文件
*****************************************************************************/
bool FtpTask::downloadFile(const string &remoteFile, const string &localFile)
{
    FILE *fp;
    struct stat file_stat;
	double file_size;
	clock_t  start_time, end_time;
	double upload_time;
	double upload_speed;
    string command;
    char buffer[1024];
	local_path +=localFile;
	remot_path +=remoteFile;

	add_ftp_download_count();
	printf("ftp download work count %d\r\n",ftp_download_work_count);


    start_time = clock();

    ostringstream commandStream;
	commandStream << "ftpget " << ftp_ip << " -P " << ftp_port << " -u " << ftp_user
				<< " -p " << ftp_pass << " " << local_path << " " << remot_path << " 2>&1";
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

	del_ftp_download_count();
	printf("ftp download work count %d\r\n",ftp_download_work_count);
	return 0;

ERR_EXIT:
	del_ftp_download_count();
	printf("ftp download work count %d\r\n",ftp_download_work_count);
	return -1;
}




/*****************************************************************************
//	描述：		上传
//	输入参数：
//localFile	上传的路径
//remoteFile	需要上传的文件
*****************************************************************************/
bool FtpTask::uploadFile( const string &localFile, const string &remoteFile)
{
    FILE *fp;
    struct stat file_stat;
	double file_size;
	clock_t  start_time, end_time;
	double upload_time;
	double upload_speed;
    //char command[300] = {0};
    string command;

    add_ftp_upload_count();

	printf("ftp upload work count %d\r\n",ftp_upload_work_count);
	local_path +=localFile;
	remot_path +=remoteFile;
   
    start_time = clock();

	ostringstream commandStream;
	commandStream << "ftpget " << ftp_ip << " -P " << ftp_port << " -u " << ftp_user
				<< " -p " << ftp_pass << " " << local_path << " " << remot_path << " 2>&1";
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
		del_ftp_upload_count();
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

    // if(strncmp(localFile.c_str(), "./", 2) == 0)
	// {
	// 	if((strcmp(FtpTask::media_path, "/media/t31")==0))
	// 	{
	// 		char tmp_file[256];
	// 		char * tmp_file_name = (char *)localFile.c_str()+2;
	// 		sprintf(tmp_file, "%s/%s" , media_path, tmp_file_name);
	// 		if(access(tmp_file, F_OK) == 0)
	// 		{
	// 			remove(tmp_file);
	// 			system("sync");
	// 		}
	// 	}
	// }

    // // 删除本地文件
    // if (remove(localFile.c_str()) == 0) {
    //     printf("file delete success\r\n");
    // } else {
    //     printf("file delete failed\r\n");
    // }
	del_ftp_upload_count();
	printf("ftp upload work count %d\r\n",ftp_upload_work_count);
	return 0;
}