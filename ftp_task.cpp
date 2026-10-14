#include "ftp_task.h"
#include "config.h"
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <zlib.h>
#include <fstream>
#include <cstring>

/*功能：ftp上传下载*/

using namespace  std;

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

int FtpTask::ftp_upload_work_count =0;
int FtpTask::ftp_download_work_count =0;

FtpTask::FtpTask()
{
	
}

FtpTask::~FtpTask()
{
	pthread_mutex_destroy(&mx_ftp_work_count);
}

void FtpTask::ftp_init()
{
	mx_ftp_work_count = PTHREAD_MUTEX_INITIALIZER; // 互斥锁
	ftp_ip = Config::Get("FTP","FTP_IP");
    ftp_user = Config::Get("FTP","FTP_user");
    ftp_pass = Config::Get("FTP","FTP_pass");
    local_path = Config::Get("FTP","Local_path");
    remot_path = Config::Get("FTP","Remot_path");
	ftp_port = stoi(Config::Get("FTP","FTP_port").c_str());
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

uint32_t FtpTask::calculateFileCRC(const string &filename, size_t offset, size_t length,const string &dts_name)
{
    ifstream file(filename, ios::binary);
    if (!file) {
		LOGE("无法打开文件 %s",filename.c_str());
        return 0;
    }

	ofstream dts(dts_name, ios::binary);
    if (!dts) {
		LOGE("无法打开文件 %s",dts_name.c_str());
        return 0;
    }
    // 如果指定了偏移，则跳过
    if (offset > 0) {
        file.seekg(offset, ios::beg);
    }

    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    uint32_t crc = crc32(0L, Z_NULL, 0);  // 初始CRC值

    size_t total_read = 0;
    while (total_read < length && file) {
        size_t to_read = MIN(BUFFER_SIZE, length - total_read);
        file.read(buffer, to_read);
        size_t bytes_read = file.gcount();
        
        if (bytes_read > 0) {
            crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer), bytes_read);
			dts.write(buffer,bytes_read);
            total_read += bytes_read;
        }
        
        if (bytes_read < to_read) break; // 文件结束或读取错误
    }

    if (total_read != length) {

		LOGE("文件读取不完整: 期望 %d 字节, 实际 %d 字节",length,total_read);
    }

    file.close();
    return crc;
}

bool FtpTask::validateFirmware(const string& file_path)
{
	image_header_t header;

	ifstream file(file_path, std::ios::binary);
    if (!file) {
		LOGE("无法打开固件文件 %s",file_path.c_str());
        return false;
    }

   if (!file.read(reinterpret_cast<char*>(&header), sizeof(header))) 
   	{
		LOGE("读取映像头失败 %s",strerror(errno));
        return false;
    }

	file.close();

	uint32_t header_crc = crc32(0L, Z_NULL, 0);

	unsigned char header_buffer[sizeof(header) - sizeof(header.ih_hcrc)];
    memcpy(header_buffer, &header, sizeof(header_buffer));
    
    header_crc = crc32(header_crc, header_buffer, sizeof(header_buffer));

	if (header_crc != header.ih_hcrc) {
		LOGE("头CRC验证失败: 计算值=0x %02x,期望值=0x %02x",header_crc,header.ih_hcrc);
        // cerr << "头CRC验证失败: 计算值=0x" << hex << header_crc 
        //           << ", 期望值=0x" << header.ih_hcrc << dec << endl;
        return false;
    }

	LOGD("头CRC验证成功: 计算值=0x %02x,期望值=0x %02x",header_crc,header.ih_hcrc);
	cout << "头CRC验证成功: 计算值=0x" << hex << header_crc 
                  << ", 期望值=0x" << header.ih_hcrc << dec << endl;
	
	uint32_t data_crc = calculateFileCRC(file_path, sizeof(header), header.ih_dlen,"server1");
    if (data_crc != header.ih_dcrc) {
		LOGE("头CRC验证失败: 计算值=0x %02x,期望值=0x %02x",data_crc,header.ih_dcrc);
        // cerr << "数据CRC验证失败: 计算值=0x" << hex << data_crc 
        //           << ", 期望值=0x" << header.ih_dcrc << dec << endl;
        return false;
    }

	LOGD("头CRC验证成功: 计算值=0x %02x,期望值=0x %02x",data_crc,header.ih_dcrc);
	cout << "数据CRC验证成功: 计算值=0x" << hex << data_crc 
                  << ", 期望值=0x" << header.ih_dcrc << dec << endl;
	
    return true;
}

void FtpTask::download_thread()
{
	if (!downloadFile("none_server","none_server_new"))
	{
		LOGD("下载更新文件失败");
		return ;
	}

	if(!validateFirmware("none_server_new"))
	{
		LOGE("比较失败");
		return;
	}

	if (chmod("./none_server_new", 0755) != 0) {

		LOGE("设置临时文件权限失败: %s", strerror(errno));
		return;
	}
	//备份当前数据
	string backupPath ="/root/none_server.bak";
	if (rename("/root/none_server", backupPath.c_str()) != 0) {
		LOGE("备份当前程序失败");
		return ;
	}

	if (chmod("/root/server1", 0755) != 0) {
		LOGE("设置权限失败: %s", strerror(errno));
	}

	//3. 移动新文件到正确位置
	if (rename("/root/server1", "/root/none_server") != 0) {
		LOGE("移动新文件失败: %s", strerror(errno));
		// 尝试恢复备份
		rename(backupPath.c_str(), "/root/none_server1");
		return ;
	}
	
	Config::requested =true;
	LOGE("替换程序成功");
	return ;
}


void FtpTask::upload_thread()
{
	uploadFile(remot_filename,media_path);
	//traverse_directory(media_path);
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
				LOGI("需要上传的文件名： %s",local_file.c_str());
				//cout<<"需要上传的文件名："<<local_file<<endl;
				uploadFile(local_file,local_file);
			}
		}
		
	}

	closedir(dir);
    return ;
}

void FtpTask::upload_file_start(const string localFile_path)
{
	media_path = localFile_path;
	remot_filename =Config::Get("W-800","W_chipid")+ "_"+ media_path.substr(media_path.find_last_of('/') + 1);

	up_thread =thread(&FtpTask::upload_thread,this);
	up_thread.detach();
	LOGI("ftp download work count\n");
}

//
void FtpTask::Get_FTPconfig()
{
	down_thread =thread(&FtpTask::download_thread,this);
	down_thread.detach();
}

/*****************************************************************************
//	描述：		上传
//	输入参数：
//localFile	本地保存路径+名
//remoteFile 远程文件名+路径
*****************************************************************************/
bool FtpTask::uploadFile(const string &remoteFile, const string &localFile)
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

	//add_ftp_upload_count();
	LOGI("ftp download work count %d\r",ftp_download_work_count);

    start_time = clock();
	//cout<<"ftp_ip: "<<ftp_ip<<" ftp_user: "<<ftp_user<<" ftp_pass:"<<ftp_pass<<" ftp_port: "<<ftp_port<<endl;
    
	ostringstream commandStream;
	commandStream << "ftpput " << ftp_ip << " -P " << ftp_port << " -u " << ftp_user
				<< " -p " << ftp_pass << " " << remot_path << " " << local_path << " 2>&1";

	command =commandStream.str();
	printf("ftp命令 %s",command.c_str());
    fp = popen(command.c_str(), "r");
	if (fp == NULL) {
		printf("Failed to run command \r\n");
		goto ERR_EXIT;
	}	

	while (fgets(buffer, sizeof(buffer), fp) != NULL) 
	{
		printf("%s\r\n", buffer);
		LOGE("%s 上传失败", remoteFile.c_str());
		pclose(fp);
		goto ERR_EXIT;
	}
	// 记录结束时间
	end_time = clock();


	// 获取文件大小
	if (stat(localFile.c_str(), &file_stat) == -1)
	{
	   LOGE("Failed to get file size \r");
	   pclose(fp);
	   goto ERR_EXIT;
	}

	file_size = file_stat.st_size/1024;
	
	// 计算上传时间
	upload_time =  (end_time-start_time)/10000;
	
	// 计算上传速率
	upload_speed = (upload_time != 0) ? (file_size / upload_time) : file_size;


	LOGI("文件大小： %.2lf KB\r\n", file_size);
	LOGI("下载时间： %.2lf s\r\n", upload_time);
	LOGI("下载速率： %.2lf KBs\r\n", upload_speed);

	pclose(fp);

	del_ftp_download_count();
	LOGI("ftp download work count %d\r\n",ftp_download_work_count);
	return true;

ERR_EXIT:
	del_ftp_download_count();
	LOGI("ftp download work count %d\r\n",ftp_download_work_count);
	return false;
}




/*****************************************************************************
//	描述：		下载
//	输入参数：
//localFile	本地文件路径
//remoteFile	需要上传的文件
*****************************************************************************/
bool FtpTask::downloadFile( const string &localFile, const string &remoteFile)
{
    FILE *fp;
    struct stat file_stat;
	double file_size;
	clock_t  start_time, end_time;
	double upload_time;
	double upload_speed;
    //char command[300] = {0};
    string command;

	add_ftp_download_count();
    

	printf("ftp upload work count %d\r\n",ftp_upload_work_count);
	local_path +=localFile;
	remot_path +=remoteFile;
   
    start_time = clock();

	ostringstream commandStream;
	commandStream << "ftpget " << ftp_ip << " -P " << ftp_port << " -u " << ftp_user
				<< " -p " << ftp_pass << " " << local_path << " " << remot_path << " 2>&1";
	command =commandStream.str();
    
	cout <<"ftp命令: "<< command <<endl;

	LOGI("ftp命令 %s",command.c_str());
	fp = popen(command.c_str(), "r");
	if (fp == NULL) {
		LOGE("Failed to run command \r\n");
		return false;
	}


    // 记录结束时间
	end_time = clock();


	// 获取文件大小
	if (stat(localFile.c_str(), &file_stat) == -1)
	{
	   	LOGE("Failed to get file size \r\n");
		del_ftp_upload_count();
	   	return false;
	}
	file_size = file_stat.st_size/1024;

	
	// 计算上传时间
	upload_time =  (end_time-start_time)/10000;
	
	// 计算上传速率
	upload_speed = (upload_time != 0) ? (file_size / upload_time) : file_size;

	LOGI("file size   : %.2lf KB\r\n", file_size);
	LOGI("upload time : %.2lf s\r\n", upload_time);
	LOGI("upload speed: %.2lf KBs\r\n", upload_speed);

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
	LOGI("ftp upload work count %d\r\n",ftp_upload_work_count);
	return true;
}