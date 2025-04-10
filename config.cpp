/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-06-21 11:34:55
 * @LastEditors: zjr00 28044466060@qq.com
 * @LastEditTime: 2025-04-10 11:41:00
 * @FilePath: \AOA\config.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "config.h"
#include <thread>
#include <atomic>
#include <ctime>
#include <sstream>
#include <iomanip>

std::map<std::string, std::map<std::string, std::string>> Config::sections;
std::vector<std::string> Config::section_order;
struct ringbuf Config::mult_ringfifo[NCHNMAX][NMAX];
struct ringbuf Config::ringfifo[NMAX];
struct ringinfo Config::g_ringinfo[NCHNMAX];
std::atomic<uint16_t> Config::g_package_sn{0};
bool Config::isCar =false;
std::map<int, ClientInfo> Config::client_map;
int64_t SipConfig::getCurTimestamp()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long long millis = (long long)(time.tv_sec) * 1000 + (long long)(time.tv_usec) / 1000;
    return millis;
}

int SipConfig::SetSipClientDefaultInfo()
{
    int i;
    gclntsta.hb_sn = 0;
    gclntsta.alarm_sn = 0;
    //GbDevInfo
	strcpy(g_devinfo.server_domain,"3402000000");
	strcpy(g_devinfo.server_id,"34020000002000000001");
	strcpy(g_devinfo.server_ip,"192.168.10.1");
	g_devinfo.server_port = 5060;
    g_devinfo.rtp_server_port = 50000;
    
    strcpy(g_devinfo.ipc_ua,"camera");
	strcpy(g_devinfo.ipc_id,"34020000001320000001");
    strcpy(g_devinfo.ipc_ip,"192.168.1.1");
    strcpy(g_devinfo.ipc_name,"HiPC");
    g_devinfo.ipc_port = 5060;
    g_devinfo.rtp_local_port = 30000;
	strcpy(g_devinfo.ipc_usrname,"34020000001320000001");
	strcpy(g_devinfo.ipc_pwd,"12345678");
	
    g_devinfo.sdp_protocol = GB28181_TRANS_TYPE_UDP;
    g_devinfo.sip_protocol = GB28181_TRANS_TYPE_UDP;

	strcpy(g_devinfo.ipc_media_id,"34020000001320000001");
	strcpy(g_devinfo.ipc_warn_id,"34020000001340000001"); 
	
	g_devinfo.reg_expires = 3600;
	g_devinfo.hb_period= 60;

    for(i=0;i<MAX_CHN;i++)
    {
        sprintf(g_devinfo.device_name[i],"%s%d","IPC",i);
        sprintf(g_devinfo.device_id[i],"%d",i);
    }
    
	strcpy(g_devinfo.device_manufacturer,"Hikvision");
	strcpy(g_devinfo.device_model,"YF1109");
	strcpy(g_devinfo.device_firmware,"V0.0.1");
	strcpy(g_devinfo.device_encode,"OFF");
	strcpy(g_devinfo.device_record,"ON");
    strcpy(g_devinfo.device_record_dir,"/media");

    //GbDevStatus
	strcpy(g_devsta.status_on,"ON");
	strcpy(g_devsta.status_ok,"OK");
	strcpy(g_devsta.status_online,"ONLINE");
	strcpy(g_devsta.status_guard,"OFFDUTY");
	strcpy(g_devsta.status_time,"2024-05-25T13:12:20");

    //RtpSendPs
    for(int i=0;i<MAX_CHN;i++)
    {
        g_rtpsendPs[i].fps = 25;
        g_rtpsendPs[i].fp = NULL;
        g_rtpsendPs[i].mQuit = true;
        g_rtpsendPs[i].mPlay = false;
        g_rtpsendPs[i].usefifo = false;
        g_rtpsendPs[i].scale = 1;
        g_rtpsendPs[i].channel = i;
        g_rtpsendPs[i].mSockFd = 0;
        //g_rtpsendPs[i].pid = 0;
        g_rtpsendPs[i].endTime = 0;
        g_rtpsendPs[i].startTime = 0;
        g_rtpsendPs[i].u32Ssrc = 0200000002;
        if(g_devinfo.sdp_protocol == GB28181_TRANS_TYPE_UDP)
            strcpy(g_rtpsendPs[i].RtpProtocol,"RTP/AVP");
        else if(g_devinfo.sdp_protocol == GB28181_TRANS_TYPE_TCP)
            strcpy(g_rtpsendPs[i].RtpProtocol, "TCP/RTP/AVP");
        g_rtpsendPs[i].sdpProtocol = g_devinfo.sdp_protocol;
        g_rtpsendPs[i].rtpLocalPort = g_devinfo.rtp_local_port+i;
        g_rtpsendPs[i].rtpServerPort = g_devinfo.rtp_server_port+1;
        pthread_mutex_init(&g_rtpsendPs[i].mutex,NULL);
        sprintf(g_rtpsendPs[i].recordDir,"%s/video%d",g_devinfo.device_record_dir,i);        
    }
	return 0;
}

int SipConfig::SetRtpRecordFileDir(char *dir)
{
    int i;
    struct stat st;
    if(stat(dir, &st)==-1)
    {
        LOGE("dir %s not exsis",dir);
        return -1;
    }
    sprintf(g_devinfo.device_record_dir,"%s",dir);
    for(i=0; i < MAX_CHN; i++)
    {
        sprintf(g_rtpsendPs[i].recordDir,"%s/video%d",dir,i);   
        if(stat(g_rtpsendPs[i].recordDir, &st)==-1)
        {
            if (mkdir(g_rtpsendPs[i].recordDir, 0755) == -1) 
            {
                LOGE("Cannot create directory %s\n",g_rtpsendPs[i].recordDir);
                return -1;
            }
        } 
    }
    return 0;
}

int SipConfig::GetStreamDuration(char *StreamPath)
{
    time_t startTime, endTime;
    struct tm timeInfo;
    struct stat st;
    if(ParseFileNameTime(StreamPath,&startTime) < 0)
    {
        LOGE("parse file name time err\n");
        return -1;
    }
    if (stat(StreamPath, &st) == -1) {
        LOGE("Get file stat err %s\n",StreamPath);
        return -1;
    }
    endTime = st.st_mtime;//st_ctime
    LOGD("Stream start time: %ld\tend time: %ld\n",startTime,endTime);
    return (endTime-startTime);
}

/*****************************************************************************
//	描述：	通过获取到的摄像头对应deviceId来判断通道
//	输入参数：
//deviceId 摄像头对应deviceId
*****************************************************************************/
int SipConfig::FindChannelByDeviceId(char *deviceId)
{
    int chn = 0;
    for(;chn<MAX_CHN;chn++)
    {
        if(0 == strcmp(deviceId, g_devinfo.device_id[chn]))
            break;
    }
    return chn >= MAX_CHN ? -1 : chn;
}

int SipConfig::ParseFileNameTime(char *path, time_t *ParseResult)
{
    struct tm timeInfo;
    char timeStr[strlen(TIMESTRING)+1];
    const char *timeStrStart = strrchr(path, '/');
    if(timeStrStart!=NULL)
		timeStrStart+=1;
	else
		timeStrStart=path;
    const char *timeStrEnd = strchr(timeStrStart, '.');
    if (timeStrEnd != NULL) {
        size_t timeStrLen = timeStrEnd - timeStrStart;
        if (timeStrLen == strlen(TIMESTRING)) {
            strncpy(timeStr, timeStrStart, timeStrLen);
            timeStr[timeStrLen] = '\0';
        } else {
            LOGE("Time string format length is incorrect. filenale: %s\tformat len: %d\n",path,timeStrLen);
            return -1;
        }
    } else {
        LOGE("Time string end '.' not found\n");
        return -1;
    }
    if(MyStrptime(timeStr, TIMEFORMAT, &timeInfo) == NULL)
    {
        LOGE("Parse time err\n");
        return -1;
    }
    timeInfo.tm_isdst = -1;
    *ParseResult = mktime(&timeInfo);
    return 0;
}

/*****************************************************************************
//	描述：	解析获取到的sdp信息
//	输入参数：
//sdpToken sdp信息
//m_sdptrack 存储解析成功后的数据结构体
*****************************************************************************/
int SipConfig::ParseSDPInfo(char *sdpToken, SdpTrack *m_sdptrack)
{
    memset((void *)m_sdptrack, 0, sizeof(SdpTrack));
    LOGD("-----------sdp parse start-----------");
    // 下面解析SDP的方式只适合一个媒体流，所有如果同时包含音频和视频流，需要注意！！！
    // 采用自定义的方式解析sdp
    while(sdpToken != NULL)
    {
        LOGI("token line:%s",sdpToken);
        if (!strncmp(sdpToken, "v=", strlen("v="))) {
            // example: v=0
            if (sscanf(sdpToken, "v=%s",  (char *)m_sdptrack->sdpVersion) != 1)//将从sdpToken 解析出来的对应数据存储到sdpVersion 结构体中
            {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else {
                LOGD("parse line success:%s", sdpToken);
            }
        }
        else if (!strncmp(sdpToken, "o=", strlen("o="))) {
            // example: o=44030200491320000000 0 0 IN IP4 192.168.1.1
            if (sscanf(sdpToken, "o=%s %s %s %s %s %s",  (char *)m_sdptrack->userName, (char *)m_sdptrack->sessionId  , \
                (char *)m_sdptrack->sessionVersion, (char *)m_sdptrack->netWorkType, (char *)m_sdptrack->addressType, \
                (char *)m_sdptrack->address) != 6)
            {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else {
                LOGD("parse line success:%s", sdpToken);
            }
        }
        else if (!strncmp(sdpToken, "s=", strlen("s="))) {
            // example: s=Play
            if (sscanf(sdpToken, "s=%s",  (char *)m_sdptrack->sessionName) != 1) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else {
                LOGD("parse line success:%s", sdpToken);
            }
        }
        else if (!strncmp(sdpToken, "c=", strlen("c="))) {
            // example: c=IN IP4 192.168.1.1
            if (sscanf(sdpToken, "c=%s %s %s", (char *)m_sdptrack->netWorkType, \
                (char *)m_sdptrack->addressType, (char *)m_sdptrack->address) != 3)
            {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else {
                LOGD("parse line success:%s", sdpToken);
            }
        }
        else if (!strncmp(sdpToken, "t=", strlen("t="))) {
            // example: t=0 0
            if (sscanf(sdpToken, "t=%ld %ld", &m_sdptrack->startTime, &m_sdptrack->endTime) != 2) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else {
                LOGD("parse line success:%s", sdpToken);
            }
        }
        else if (!strncmp(sdpToken, "m=", strlen("m="))) {
            // example: m=video 5060 TCP/RTP/AVP 96 97 98 99
            if (sscanf(sdpToken, "m=%s %d %s %[0-9 ]", (char *)m_sdptrack->mediaType, \
                &m_sdptrack->port, (char *)m_sdptrack->transportProtocol, (char *)m_sdptrack->fmtList) != 4)
            {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else {
                char *result = strstr(m_sdptrack->transportProtocol,"TCP");
                if (result)
                    m_sdptrack->protocol = GB28181_TRANS_TYPE_TCP;
                else
                    m_sdptrack->protocol = GB28181_TRANS_TYPE_UDP;

                LOGD("parse line success:%s", sdpToken);
            }
        }
        else if (!strncmp(sdpToken, "y=", strlen("y="))) {
            // example: y=0200000003
            if (sscanf(sdpToken, "y=%d", &m_sdptrack->ssrc) != 1) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else{
                LOGD("parse line success:%s", sdpToken);
            }
        }
        else if (!strncmp(sdpToken, "a=downloadspeed", strlen("a=downloadspeed"))) {
            // example: a=downloadspeed:4
            if (sscanf(sdpToken, "a=downloadspeed:%f", &m_sdptrack->scale) != 1) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            }
            else{
                LOGD("parse line success:%s", sdpToken);
            }
        }
        else {
            LOGI("no parsing rule is added,Skip this line:%s", sdpToken);
        }
        sdpToken = strtok(NULL, "\n");
    }
    LOGD("-----------sdp parse end-----------\n");
    return 0;
}

int SipConfig::UpdateRtpSendCfgBySDPInfo(int channel, SdpTrack m_sdptrack)
{
    strcpy(g_rtpsendPs[channel].rtpServerIp,m_sdptrack.address);
    g_rtpsendPs[channel].rtpServerPort = m_sdptrack.port;
    strcpy(g_rtpsendPs[channel].rtplocalIp,g_devinfo.ipc_ip);
    strcpy(g_rtpsendPs[channel].RtpProtocol,m_sdptrack.transportProtocol);
	g_rtpsendPs[channel].sdpProtocol = m_sdptrack.protocol;
    g_rtpsendPs[channel].u32Ssrc = m_sdptrack.ssrc;
    strcpy(g_rtpsendPs[channel].sessionName,m_sdptrack.sessionName);
    g_rtpsendPs[channel].startTime = m_sdptrack.startTime;
    g_rtpsendPs[channel].endTime = m_sdptrack.endTime;
    g_rtpsendPs[channel].scale = m_sdptrack.scale > 0 ? m_sdptrack.scale : 1;
    return 0;
}

char *SipConfig::MyStrptime(const char *buf, const char *format, tm *tm)
{
     char *endPtr = NULL;
    endPtr = strptime(buf,format,tm);
    if(endPtr != NULL)
        return endPtr ;
    size_t i = 0, j = 0;
    while (buf[i] && format[j]) {
        if (format[j] == '%') {
            switch (format[++j]) {
                case 'Y':
                    sscanf(buf + i, "%4d", &tm->tm_year);
                    tm->tm_year-=1900;
                    i += 4;
                    break;
                case 'm':
                    sscanf(buf + i, "%2d", &tm->tm_mon);
                    i += 2;
                    tm->tm_mon-=1; // tm_mon is 0-based
                    break;
                case 'd':
                    sscanf(buf + i, "%2d", &tm->tm_mday);
                    i += 2;
                    break;
                case 'H':
                    sscanf(buf + i, "%2d", &tm->tm_hour);
                    i += 2;
                    break;
                case 'M':
                    sscanf(buf + i, "%2d", &tm->tm_min);
                    i += 2;
                    break;
                case 'S':
                    sscanf(buf + i, "%2d", &tm->tm_sec);
                    i += 2;
                    break;
                default:
                    // Handle other format specifiers if needed
                    break;
            }
            j++;
        } else {
            if (buf[i] != format[j]) {
                return NULL; // Mismatch
            }
            i++;
            j++;
        }
    }
    if (format[j] != '\0') {
        return NULL; // Format string not fully parsed
    }
    return (char *)(buf+i);
}

int SipConfig::FindVideoByTime(char *dirName, int channel, time_t startTime, time_t endTime, int *total)
{
    DIR *dir;
    struct dirent *entry;
    time_t filetime;
    struct tm timeInfo, startInfo, endInfo;
    *total = 0;

    if (!(dir = opendir(dirName))) {
        fprintf(stderr, "Cannot open directory: %s\n", dirName);
        return -1;
    }
 
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        if(ParseFileNameTime(entry->d_name, &filetime) < 0)
        {
            LOGE("parse file name time err\n");
            continue;
        }
        if (filetime >= startTime && filetime <= endTime) {
            localtime_r(&startTime, &startInfo);
            localtime_r(&endTime, &endInfo);
            LOGD("file timestamp:%ld\t%s was created between %s and %s\n", filetime, entry->d_name, asctime(&startInfo), asctime(&endInfo));
            *total +=1;
        }
    }
    closedir(dir);
    return 0;
}

bool SipConfig::GetSipClientStatus()
{
    return !gclntsta.quit;
}

bool SipConfig::GetSipClientRegister()
{
    return gclntsta.mRegistered;
}

int SipConfig::GetRtpStreamFps(int *fps)
{
    *fps = g_rtpsendPs->fps;
    return 0;
}

int SipConfig::SetRtpStreamFps(int fps)
{
    g_rtpsendPs->fps = fps;
    return 0;
}

int SipConfig::GetRtpRecordFileDir(char *dir)
{
   sprintf(dir, "%s", g_devinfo.device_record_dir);
    return 0;
}

int SipConfig::GetRtpFifoSize(int *width, int *height)
{
    *width = ringsize.width;
    *height = ringsize.height;
    return 0;
}

int SipConfig::SetRtpFifoSize(int width, int height)
{
    ringsize.width = width;
    ringsize.height = height;
    return 0;
}

int SipConfig::GetRtpStreamResolution(int *width, int *height)
{
    *width = input_size.width;
    *height = input_size.height;
    return 0;
}

int SipConfig::SetRtpStreamResolution(int width, int height)
{
    input_size.width = width;
    input_size.height = height;
    return 0;
}

int SipConfig::SendStreamData(int channel, unsigned char *buffer, int size, int encode_type)
{
    if(GetRtpStreamStatus(channel) != true)
        return -1;
    if(Config::ringput_fifochn(channel+REALTIME_FIFO_OFFSET, buffer, size, encode_type)==0)
        Config::ringreset_fifochn(MAX_CHN+REALTIME_FIFO_OFFSET);//buff full
    return 0;
}

bool SipConfig::GetRtpStreamStatus(int channel)
{
    pthread_mutex_lock(&g_rtpsendPs[channel].mutex);
    bool status = g_rtpsendPs[channel].usefifo;
    pthread_mutex_unlock(&g_rtpsendPs[channel].mutex);
    return status;
}

/************************************************************************************
************************************************************************************
************************************************************************************
************************************************************************************/

Config::Config()
{
    FileInit("./config.ini");
}

Config::~Config()
{
    
}


/*****************************************************************************
//	描述：		获取配置文件对应值
//	输入参数：
//section 分组
//key  键
//default_value  返回的默认值----可以不填 
*****************************************************************************/
string Config::Get(const string &section, const string &key, const string &default_value)
{
    auto section_it = Config::sections.find(section);

    if (section_it == sections.end()) {
        return default_value; //返回默认值空
    }

    auto key_it = section_it->second.find(key);
    if (key_it == section_it->second.end()) {
        return default_value;//返回默认值空
    }
    //cout<<"key_it:"<<key_it->second<<endl;
    return key_it->second;
}

bool Config::SetToFile(const std::string &section, const std::string &key, const std::string &value, const std::string &filename)
{
    if (section.empty()) 
    {
        printf("节名不能为空\n");
        return false;
    }

    sections[section][key] = value;


    std::ofstream file(filename);
    if (!file.is_open()) {
        printf("无法打开文件进行写入\n");
        return false;
    }

    for (const auto& section_name : section_order) {
        file << "[" << section_name << "]" << std::endl;
        for (const auto& kv : sections[section_name]) {
            file << kv.first << " = " << kv.second << std::endl;
        }
        file << std::endl;
    }

    file.close();
    return true;
}

/*****************************************************************************
//	描述：		获取配置文件中的配置信息
//	输入参数：
//FileInit 配置文件路径
*****************************************************************************/
void Config::FileInit(string filename)
{
    ifstream file;
    file.open(filename,ios_base::out);
    if (!file.is_open()) {
       cout<<"打开文件失败"<<endl;
       return;
    }
    
    std::string line;
    std::string current_section;

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            if (sections.find(current_section) == sections.end()) {
                section_order.push_back(current_section); // 记录新节的顺序
            }
        } else {
            size_t delimiter_pos = line.find('=');
            if (delimiter_pos != std::string::npos) {
                std::string key = line.substr(0, delimiter_pos);
                std::string value = line.substr(delimiter_pos + 1);

                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t\""));
                value.erase(value.find_last_not_of(" \t\"") + 1);


                sections[current_section][key] = value;
            }
        }
    }

    file.close();
    return;

}


/*****************************************************************************
//	描述：		计算数组有效数据长度
//	输入参数：
//buf -- 需要计算报文
//len  数组大小
*****************************************************************************/
int Config::GetCmdLen(unsigned char *buf, int bufLen)
{
   if (bufLen <= 0)
	{
		return 0;
	}
	int len = 0;
	for (int i = bufLen - 1; i >= 0; i--)
	{
		if ((char)*(buf + i) != (char)0x00)
		{
			len = i + 1;
			break;
		}
	}
    //printf("len = %d\n", len);
	return len;
}


/*****************************************************************************
//	描述：		打印数组数据
//	输入参数：
//buf -- 需要打印的数据
//prompt  提示信息
*****************************************************************************/
void Config::Show(unsigned char *buf, char *prompt)
{

    int len = GetCmdLen(buf, 128);

   
    if (len <= 0) {
        return;
    }

    printf("%s: \n", prompt);
    for (int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
        if ((i + 1) % 16 == 0)
        { // 每行打印16个字节
            printf("\n");
        }
    }
    printf("\n");
}


int findStartCode(unsigned char* buf, int zeros_in_startcode)
{
    int info;
    int i;

    info = 1;
    for (i = 0; i < zeros_in_startcode; i++)
        if (buf[i] != 0)
            info = 0;

    if (buf[i] != 1)
        info = 0;
    return info;
}

/*****************************************************************************
//	描述：		获取h264文件数据
//	输入参数：
//inpf -- 文件名
//buf  存储获取到的文件流
*****************************************************************************/
int Config::getNextNalu(FILE *inpf, unsigned char *buf)
{
    int pos = 0;
    int startCodeFound = 0;
    int info2 = 0;
    int info3 = 0;
    while (!feof(inpf) && (buf[pos++] = fgetc(inpf)) == 0); // fgetc:读取成功时返回读取到的字符，读取到文件末尾或读取失败时返回EOF
    while (!startCodeFound)
    {
        if (feof(inpf))   //feof:其功能是检测流上的文件结束符，如果文件结束，则返回非0值，否则返回0
        {
            return pos - 1;
        }
        buf[pos++] = fgetc(inpf);
        info3 = findStartCode(&buf[pos - 4], 3);//查找3字节的起始码
        startCodeFound = (info3 == 1);
        if (info3 != 1)
            info2 = findStartCode(&buf[pos - 3], 2);//如果没有查到3字节起始码，就查找2字节
        startCodeFound = (info2 == 1 || info3 == 1);
    }
    if (info2)
    {
        fseek(inpf, -3, SEEK_CUR);  //fseek:重定位流(数据流/文件)上的文件内部位置指针 返回值：成功，返回0，失败返回非0值，并设置error的值  SEEK_CUR	1
        return pos - 3;
    }
    if (info3)
    {
        fseek(inpf, -4, SEEK_CUR);
        return pos - 4;
    }
    return 0;
}

int Config::getNalupos2(uint8_t *data, int plen, int *prefix, uint8_t *nalu)
{
    uint8_t* p = NULL;
	int len = plen - 4;
    //遍历查找NALU起始码
	for (int i = 0; i < len; i++) {
		p = data + i;
		if ((*p) == 0x00 && (*(p + 1)) == 0x00 && (*(p + 2)) == 0x00 && (*(p + 3)) == 0x01) {
			*prefix = 4;
            *nalu = *(p + 4);//获取起始码类型
			return i + 4;
		}
		if ((*p) == 0x00 && (*(p + 1)) == 0x00 && (*(p + 2)) == 0x01)
		{
			*prefix = 3;
            *nalu = *(p + 3);
			return i + 3;
		}
	}
	return -1;
}

void Config::ringmalloc_fifochn(int chn, int size)
{
    int i;
    if(chn >= NCHNMAX)
    {
        printf("channel invaled\n");
        return;
    }
    for(i =0; i<NMAX; i++)
    {
        mult_ringfifo[chn][i].buffer = (unsigned char *)malloc(sizeof(unsigned char)*size);
        mult_ringfifo[chn][i].size = 0;
        mult_ringfifo[chn][i].frame_type = 0;
       // printf("FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
    }
    g_ringinfo[chn].iput = 0; /* 环形缓冲区的当前放入位置 */
    g_ringinfo[chn].iget = 0; /* 缓冲区的当前取出位置 */
    g_ringinfo[chn].n = 0; /* 环形缓冲区中的元素总数量 */
}

void Config::ringreset_fifochn(int chn)
{
    if(chn >= NCHNMAX)
    {
        printf("channel invaled\n");
        return;
    }
    g_ringinfo[chn].iput = 0; /* 环形缓冲区的当前放入位置 */
    g_ringinfo[chn].iget = 0; /* 缓冲区的当前取出位置 */
    g_ringinfo[chn].n = 0; /* 环形缓冲区中的元素总数量 */
}

void Config::ringfree_fifochn(int chn)
{
    int i;
    if(chn >= NCHNMAX)
    {
        printf("channel invaled\n");
        return;
    }
    // printf("begin free mem\n");
    for(i =0; i<NMAX; i++)
    {
       // printf("FREE FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
       if(mult_ringfifo[chn][i].buffer)
       {
            free(mult_ringfifo[chn][i].buffer);
            mult_ringfifo[chn][i].buffer=NULL;
       }
        mult_ringfifo[chn][i].size = 0;
    }
}

int addring(int i)
{
    return (i+1) == NMAX ? 0 : i+1;
}



int Config::ringget_fifochn(int chn, ringbuf *getinfo)
{
    int Pos;
    if(chn >= NCHNMAX)
    {
        printf("channel invaled\n");
        return 0;
    }
    if(g_ringinfo[chn].n>0)
    {
        Pos = g_ringinfo[chn].iget;
        g_ringinfo[chn].iget = addring(g_ringinfo[chn].iget);
        g_ringinfo[chn].n--;
        getinfo->buffer = (mult_ringfifo[chn][Pos].buffer);
        getinfo->frame_type = mult_ringfifo[chn][Pos].frame_type;
        getinfo->size = mult_ringfifo[chn][Pos].size;
        //printf("Get FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",Pos,getinfo->size,(int)(getinfo->buffer),getinfo->frame_type);
        return mult_ringfifo[chn][Pos].size;
    }
    else
    {
        //printf("Buffer is empty\n");
        return 0;
    }
}


int Config::ringput_fifochn(int chn, unsigned char *buffer, int size, int encode_type)
{
    if(chn >= NCHNMAX)
    {
        printf("channel invaled\n");
        return -1;
    }
    if(g_ringinfo[chn].n<NMAX)
    {
        memcpy(mult_ringfifo[chn][g_ringinfo[chn].iput].buffer,buffer,size);
        mult_ringfifo[chn][g_ringinfo[chn].iput].size= size;
        mult_ringfifo[chn][g_ringinfo[chn].iput].frame_type = encode_type;
        //printf("Put FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",iput,ringfifo[iput].size,(int)(ringfifo[iput].buffer),ringfifo[iput].frame_type);
        g_ringinfo[chn].iput = addring(g_ringinfo[chn].iput);
        g_ringinfo[chn].n++;
        return size;
    }
    else
    {
        printf("Buffer is full\n");
        return 0;
    }
}

uint16_t Config::package_get_sn()
{
    uint32_t sn = ++g_package_sn;
    while (sn == 0u) {
        sn = ++g_package_sn;
    }
    return sn;
}

string Config::GetTime_Path()
{
    time_t now = time(nullptr);
    tm local_time;
    localtime_r(&now, &local_time);
    ostringstream oss;
    oss <<"./"<< setw(4) << (local_time.tm_year + 1900)<<"/"
        << setw(2) << setfill('0') << (local_time.tm_mon + 1)<<"/"
        << setw(2) << setfill('0') << local_time.tm_mday<<"/";

    return oss.str();
}