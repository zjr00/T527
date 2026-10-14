#include "Sipconfig.h"
#include <sys/time.h>
#include <cstdlib>     
#include <cstring> 
int SipConfig::g_max_channels =6;
int SipConfig::g_realtime_fifo_offset = 0;
StreamControlFunc SipConfig::g_stream_controller = NULL;


//修改
void SipConfig::register_gb_stream_controller(StreamControlFunc func)
{
    g_stream_controller = func;  
}

//修改
StreamControlFunc SipConfig::get_gb_stream_controller()
{
    return g_stream_controller;
}

int SipConfig::get_fifo_offset()
{
    return g_realtime_fifo_offset;
}

int SipConfig::set_fifo_offset(int ofs)
{
    g_realtime_fifo_offset = ofs;
    return 0;
}

//修改
int64_t SipConfig::getCurTimestamp()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long long millis = (long long)(time.tv_sec) * 1000 + (long long)(time.tv_usec) / 1000;
    return millis;
}

//修改
int SipConfig::SetSipClientDefaultInfo()
{
    g_client.sn = 0;
    g_client.quit = true;
    g_client.allowReg = true;
    g_client.mRegistered = false;
    g_client.mRegisterId = 0;
    g_client.pid = 0;
    //GbDevInfo
    g_sipinfo.timeZone = 8;
    g_sipinfo.loadSpeed = 4.0f;
    g_sipinfo.regExpires = 3600;
    g_sipinfo.heartBeatCount = 3;
	g_sipinfo.heartBeatInterval= 60;
    g_sipinfo.sipProtocol = GB28181_TRANS_TYPE_UDP;
    g_sipinfo.sdpProtocol = GB28181_TRANS_TYPE_UDP;

	strcpy(g_sipinfo.realm,"3402000000");
    strcpy(g_sipinfo.nonce,"1234567890123456");
	strcpy(g_sipinfo.serverId,"34020000002000000001");
	strcpy(g_sipinfo.serverIp,"192.168.10.1");
	g_sipinfo.sipServerPort = 5060;
    g_sipinfo.sipRtpServerPort = 50000;
    
    strcpy(g_sipinfo.userAgent,"HHD camera version:v1.0.0");
	strcpy(g_sipinfo.userid,"34020000001320000001");
    strcpy(g_sipinfo.deviceIp,"192.168.1.1");
    strcpy(g_sipinfo.deviceName,"W680LV");
    g_sipinfo.sipLoaclPort = 5060;
    g_sipinfo.sipRtpLoaclPort = 6000;
	strcpy(g_sipinfo.username,"34020000001320000001");
	strcpy(g_sipinfo.passwd,"12345678");

    g_sipinfo.channelId = (char**)malloc(g_max_channels *sizeof(char*));
    g_sipinfo.channelName = (char**)malloc(g_max_channels *sizeof(char*));

    for(int i=0;i<g_max_channels;i++) {
        g_sipinfo.channelId[i] = (char*)malloc(0x20);
        g_sipinfo.channelName[i] = (char*)malloc(0x20);
        sprintf(g_sipinfo.channelName[i],"%s%d","IPC",i);
        sprintf(g_sipinfo.channelId[i],"%s%d","0000000000000000000",i);
    }
    int divisor = gcd(720, 1280);
    sprintf(g_sipinfo.resolution,"%d/%d",720/divisor,1280/divisor);
	strcpy(g_sipinfo.manufacturer,"ZLian");
	strcpy(g_sipinfo.deviceModel,"YF1109");
	strcpy(g_sipinfo.deviceFirmware,"V1.0.0");
	strcpy(g_sipinfo.deviceEncode,"OFF");
	strcpy(g_sipinfo.deviceRecord,"ON");

    //GbDevStatus
	strcpy(g_devsta.status_on,"OFF");
	strcpy(g_devsta.status_ok,"OK");
	strcpy(g_devsta.status_online,"ONLINE");
	strcpy(g_devsta.status_guard,"OFFDUTY");
	strcpy(g_devsta.status_time,"2024-05-25T13:12:20");

    //RtpSendPs
    g_rtpsendPs = rtp_array_create(g_max_channels);
    if(!g_rtpsendPs) return -1;
    for(int i=0;i<g_max_channels;i++) {
        g_rtpsendPs[i].fps = 25;
        g_rtpsendPs[i].pyloadType = GB_H264;
        g_rtpsendPs[i].fp = NULL;
        g_rtpsendPs[i].mQuit = true;
        g_rtpsendPs[i].usefifo = false;
        g_rtpsendPs[i].mPlay = false;
        g_rtpsendPs[i].mRun = false;
        g_rtpsendPs[i].usefifo = false;
        g_rtpsendPs[i].scale = 1;
        g_rtpsendPs[i].channel = i;
        g_rtpsendPs[i].mSockFd = 0;
        //g_rtpsendPs[i].pid = 0;
        g_rtpsendPs[i].endTime = 0;
        g_rtpsendPs[i].startTime = 0;
        g_rtpsendPs[i].u32Ssrc = 0200000002;
        if(g_sipinfo.sdpProtocol == GB28181_TRANS_TYPE_UDP)
            strcpy(g_rtpsendPs[i].rtpProtocol,"RTP/AVP");
        else if(g_sipinfo.sdpProtocol == GB28181_TRANS_TYPE_TCP)
            strcpy(g_rtpsendPs[i].rtpProtocol, "TCP/RTP/AVP");
        g_rtpsendPs[i].sdpProtocol = g_sipinfo.sdpProtocol;
        g_rtpsendPs[i].rtpLocalPort = g_sipinfo.sipRtpLoaclPort+i;
        g_rtpsendPs[i].rtpServerPort = g_sipinfo.sipRtpServerPort+1;
        sprintf(g_rtpsendPs[i].recordDir,"/media/video%d",i);        
    }
	return 0;
}


//修改
int SipConfig::SetGBDeviceConfig(DeviceConfig_t config)
{
    if(config.status)
        strcpy(g_devsta.status_on,"ON");
    else
        strcpy(g_devsta.status_on,"OFF");
    g_sipinfo.codeStream = config.codeStream;
    strcpy(g_sipinfo.serverIp,config.serverIp);
    g_sipinfo.sipServerPort = config.sipServerPort;
    strcpy(g_sipinfo.serverId,config.serverId);
    strcpy(g_sipinfo.realm,config.domain);
    strcpy(g_sipinfo.deviceEncode,config.encode?"ON":"OFF");
    strcpy(g_sipinfo.passwd,config.passwd);
    g_sipinfo.regExpires = config.regExpires;
    g_sipinfo.heartBeatInterval = config.heartBeatInterval;
    strcpy(g_sipinfo.userid,config.userid);
    strcpy(g_sipinfo.username,config.userid);
    g_sipinfo.sipLoaclPort = config.sipLoaclPort;
    g_sipinfo.alertId = config.alertId;
    LOGD("current config:\nstatus: %s\nserver ip:%s\nserver port: %d\nserver id: %s\n\
domain: %s\nipc id: %s\npassword: %s\nipc port: %d\nreg expires: %d\nheart beat: %d\n",\
        g_devsta.status_on,g_sipinfo.serverIp,g_sipinfo.sipServerPort,g_sipinfo.serverId,g_sipinfo.realm,\
        g_sipinfo.userid,g_sipinfo.passwd,g_sipinfo.sipLoaclPort,g_sipinfo.regExpires,g_sipinfo.heartBeatInterval);
    return 0;
}

//修改
int SipConfig::SetRtpStreamPyloadType(int type)
{
    int i=0;
    for ( i = 0; i < g_max_channels; i++) {
        g_rtpsendPs[i].pyloadType = type;
    }
    return 0;
}

//修改
int SipConfig::SetRtpRecordFileDir(int chn, char *dir)
{
   struct stat st;
    if(chn >= g_max_channels)
        return -1;
    sprintf(g_rtpsendPs[chn].recordDir,"%s",dir);
    if(stat(g_rtpsendPs[chn].recordDir, &st)==-1) {
        LOGW("notice: recorder directory %s not exist",g_rtpsendPs[chn].recordDir);
        // if (mkdir(g_rtpsendPs[chn].recordDir, 0755) == -1) {
        //     LOGE("Cannot create directory %s\n",g_rtpsendPs[chn].recordDir);
        //     return -1;
        // }
    }
    return 0;
}

//修改
int SipConfig::GetStreamDuration(char *StreamPath, time_t *startTime, time_t *endTime, time_t *duration) 
{
    struct stat st;
    if(ParseFileNameToTimestamp(StreamPath,RecordFileTimeFormat,startTime) < 0) {
        LOGE("parse file name time err\n");
        return -1;
    }
    if (stat(StreamPath, &st) == -1) {
        LOGE("Get file stat err %s\n",StreamPath);
        return -1;
    }
    *endTime = st.st_mtime;//st_ctime
    *duration = *endTime - *startTime;
    LOGD("st.st_mtime = %lld st.st_ctime = %lld",(long long)st.st_mtime,(long long)st.st_ctime);
    LOGD("Stream start time: %lld\tend time: %lld\tduration: %lds",(long long)startTime,(long long)endTime,duration);
    return 0;
}

/* Parse dir/path/-**-YYYY-MM-DDTHH-MM-SS.xxx to timestamp */ //修改
int SipConfig::ParseFileNameToTimestamp(char *path, char *timeFormat, time_t *timeStamp)
{
     if(path == NULL) {
        LOGE("File name is null");
        return -1;
    }
    char timeStr[strlen(TIMESTRING)+1];
    const char *base = strrchr(path, '/');
    base = (base != NULL) ? base + 1 : path;
    const char *timeStrEnd = strchr(base, '.');
    if(!timeStrEnd || (timeStrEnd - base) < (long int)strlen(TIMESTRING)) {
        LOGE("Invalid filename structure,time format not match %s,file name: %s\n",TIMESTRING, path);
        return -1;
    }
    const char *timeStrStart = timeStrEnd - strlen(TIMESTRING);
    strncpy(timeStr, timeStrStart, strlen(TIMESTRING));
    timeStr[strlen(TIMESTRING)] = '\0';
    if(ParseTimstrToTimestamp(timeStr,timeFormat,timeStamp) < 0) {
        LOGE("Invalid time string,time string not match %s,file name: %s\n",timeFormat, timeStr);
        return -1;
    }
    return 0;
}

////修改
int SipConfig::ParseTimstrToTimestamp(char *timeStr, char *strFormat, time_t *timeStamp)
{
    char *timeFormat;
    struct tm timeInfo;
    time_t timeOffset;
    time_t adjustedTime;
    if(timeStr == NULL) {
        LOGE("timeStr is null");
        return -1;
    }
    if(strFormat != NULL) {
        timeFormat = strFormat;
    } else {
        LOGD("timeFormat is null, use default timeFormat");
        timeFormat = DefaultTimeFormat;
    }
    if(MyStrptime(timeStr, timeFormat, &timeInfo) == NULL) {
        LOGE("Parse time string err! timeStr: %s, timeFormat: %s\n",timeStr,timeFormat);
        return -1;
    }
    /* adjust time to config timezone*/
    timeInfo.tm_isdst = -1;
    adjustedTime = timegm(&timeInfo);
    if (adjustedTime == -1) {
        LOGE("mktime failed");
        return -1;
    }
    /* adjust time to config timezone */
    timeOffset = (g_sipinfo.timeZone * 60 * 60);
    *timeStamp = adjustedTime - timeOffset;
    // LOGD("adjust time to config timezone, offset: %lld",timeOffset);
    return 0;
}

//修改
int SipConfig::ParseTimestampToTimestr(time_t timeStamp, char *strFormat, char *timeStr, size_t strSize)
{
     char *timeFormat;
    time_t timeOffset;
    time_t adjustedTime;
    if(timeStr == NULL) {
        LOGE("timeStr is null");
        return -1;
    }
    if(strFormat != NULL) {
        timeFormat = strFormat;
    } else {
        LOGD("timeFormat is null, use default timeFormat");
        timeFormat = DefaultTimeFormat;
    }
    /* adjust time to config timezone */
    timeOffset = (g_sipinfo.timeZone * 60 * 60);
    adjustedTime = timeStamp + timeOffset;
    // LOGD("adjust time to config timezone, offset: %lld",timeOffset);
    struct tm *zoneTime = gmtime(&adjustedTime);
    if (zoneTime == NULL) {
        LOGE("gmtime failed");
        return -1;
    }
    /* adjust time to config timezone*/
    size_t result = strftime(timeStr, strSize, timeFormat, zoneTime);
    if (result == 0) {
        LOGE("strftime failed");
        return -1;
    }
    return 0;
}

/*****************************************************************************
//	描述：	通过获取到的摄像头对应deviceId来判断通道 //修改
//	输入参数：
//deviceId 摄像头对应deviceId
*****************************************************************************/
int SipConfig::FindChannelByDeviceId(char *deviceId)
{
   int chn = 0;
    for(;chn<g_max_channels;chn++) {
        if(0 == strcmp(deviceId, g_sipinfo.channelId[chn]))
            break;
    }
    return chn >= g_max_channels ? -1 : chn;
}


/*****************************************************************************
//	描述：	解析获取到的sdp信息 //修改
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
    while(sdpToken != NULL) {
        LOGD("token line:%s",sdpToken);
        if (!strncmp(sdpToken, "v=", strlen("v="))) {
            // example: v=0
            if (sscanf(sdpToken, "v=%s",  (char *)m_sdptrack->sdpVersion) != 1) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "o=", strlen("o="))) {
            // example: o=44030200491320000000 0 0 IN IP4 192.168.1.1
            if (sscanf(sdpToken, "o=%s %s %s %s %s %s",  (char *)m_sdptrack->userName, (char *)m_sdptrack->sessionId  , \
                (char *)m_sdptrack->sessionVersion, (char *)m_sdptrack->netWorkType, (char *)m_sdptrack->addressType, \
                (char *)m_sdptrack->address) != 6) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "s=", strlen("s="))) {
            // example: s=Play
            if (sscanf(sdpToken, "s=%s",  (char *)m_sdptrack->sessionName) != 1) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "c=", strlen("c="))) {
            // example: c=IN IP4 192.168.1.1
            if (sscanf(sdpToken, "c=%s %s %s", (char *)m_sdptrack->netWorkType, \
                (char *)m_sdptrack->addressType, (char *)m_sdptrack->address) != 3) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "t=", strlen("t="))) {
            // example: t=0 0
            if (sscanf(sdpToken, "t=%ld %ld", &m_sdptrack->startTime, &m_sdptrack->endTime) != 2) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "m=video", strlen("m=video"))) {
            // example: m=video 5060 TCP/RTP/AVP 96 97 98 99
            if (sscanf(sdpToken, "m=%s %d %s %[0-9 ]", (char *)m_sdptrack->mediaType, \
                &m_sdptrack->port, (char *)m_sdptrack->transportProtocol, (char *)m_sdptrack->fmtList) != 4) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                char *result = strstr(m_sdptrack->transportProtocol,"TCP");
                if (result)
                    m_sdptrack->protocol = GB28181_TRANS_TYPE_TCP;
                else
                    m_sdptrack->protocol = GB28181_TRANS_TYPE_UDP;
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "m=audio", strlen("m=audio"))) {
            if(sscanf(sdpToken, "m=%s %d %s %[0-9 ]", (char *)m_sdptrack->mediaType, \
                &m_sdptrack->audioPort, m_sdptrack->transportProtocol, (char *)m_sdptrack->fmtList) != 4) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                m_sdptrack->hasAudio = 1;
                char *result = strstr(m_sdptrack->transportProtocol,"TCP");
                if (result)
                    m_sdptrack->protocol = GB28181_TRANS_TYPE_TCP;
                else
                    m_sdptrack->protocol = GB28181_TRANS_TYPE_UDP;
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "a=rtpmap", strlen("a=rtpmap"))) {
            if (strstr(sdpToken, "PCMA/8000")) {
                m_sdptrack->audioCodec = AUDIO_CODEC_PCMA;
            }
        } else if (!strncmp(sdpToken, "y=", strlen("y="))) {
            // example: y=0200000003
            if (sscanf(sdpToken, "y=%d", &m_sdptrack->ssrc) != 1) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "a=downloadspeed", strlen("a=downloadspeed"))) {
            // example: a=downloadspeed:4
            if (sscanf(sdpToken, "a=downloadspeed:%f", &m_sdptrack->scale) != 1) {
                LOGE("parse line error:%s", sdpToken);
                return -1;
            } else {
                LOGD("parse line success:%s", sdpToken);
            }
        } else if (!strncmp(sdpToken, "stream", strlen("stream"))) {
            // example: 
            // stream:0(主码流)
            // stream:1(子码流)
            // streamnumber:0(主码流-2022)
            // streamnumber:1(子码流-2022)
            // streamprofile:0(主码流-大华)
            // streamprofile:1(子码流-大华)
            // streamMode:main(主码流-水星+TP-LINK)
            // streamMode:sub(子码流-水星+TP-LINK)
            if (strstr(sdpToken, ":0") != NULL || 
                strstr(sdpToken, "main") != NULL) {
                m_sdptrack->codeStream = 0;
                LOGD("parse line success:%s", sdpToken);
            } else if (strstr(sdpToken, ":1") != NULL || 
                strstr(sdpToken, "sub") != NULL) {
                m_sdptrack->codeStream = 1;
                LOGD("parse line success:%s", sdpToken);
            } else {
                LOGE("parse line error:%s", sdpToken);
            }
        } else {
            LOGI("no parsing rule is added,Skip this line:%s", sdpToken);
        }
        sdpToken = strtok(NULL, "\n");
    }
    LOGD("-----------sdp parse end-----------\n");
    if (m_sdptrack->port == 0 || m_sdptrack->address[0] == '\0') {
        LOGE("Essential SDP fields missing: port=%d, address=%s",
             m_sdptrack->port, m_sdptrack->address);
        return -1;
    }
    return 0;
}

//修改
int SipConfig::UpdateRtpSendCfgBySDPInfo(int channel, SdpTrack m_sdptrack)
{
    g_rtpsendPs[channel].hasAudio = m_sdptrack.hasAudio;
    strcpy(g_rtpsendPs[channel].rtpServerIp,m_sdptrack.address);
    g_rtpsendPs[channel].rtpServerPort = m_sdptrack.port;
    strcpy(g_rtpsendPs[channel].rtplocalIp,g_sipinfo.deviceIp);
    strcpy(g_rtpsendPs[channel].rtpProtocol,m_sdptrack.transportProtocol);
	g_rtpsendPs[channel].sdpProtocol = m_sdptrack.protocol;
    g_rtpsendPs[channel].u32Ssrc = m_sdptrack.ssrc;
    strcpy(g_rtpsendPs[channel].sessionName,m_sdptrack.sessionName);
    g_rtpsendPs[channel].startTime = m_sdptrack.startTime;
    g_rtpsendPs[channel].endTime = m_sdptrack.endTime;
    g_rtpsendPs[channel].scale = m_sdptrack.scale > 0 ? m_sdptrack.scale : 1;
    return 0;
}

//修改
char *SipConfig::MyStrptime(const char *buf, const char *format, tm *tm)
{
    char *endPtr = NULL;
    endPtr = strptime(buf,format,tm);
    if(endPtr != NULL)
        return endPtr;
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

//strftime/strptime //修改
//StartTime:2020-01-23T00:00:00
//EndTime  :2020-01-23T23:59:59
int SipConfig::FindVideoByTime(char *dirName, int channel, time_t startTime, time_t endTime, int *total)
{
    DIR *dir;
    struct dirent *entry;
    time_t filetime;
    *total = 0;

    if (!(dir = opendir(dirName))) {
        fprintf(stderr, "Cannot open directory: %s\n", dirName);
        return -1;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        if(ParseFileNameToTimestamp(entry->d_name, RecordFileTimeFormat, &filetime) < 0) {
            LOGE("parse file name time err\n");
            continue;
        }
        if (filetime >= startTime && filetime <= endTime) {
            LOGD("file timestamp:%lld\t%s was created between %lld and %lld", \
                (long long)filetime, entry->d_name, startTime, endTime);
            *total +=1;
        }
    }
    closedir(dir);
    return 0;
}

//修改
int SipConfig::get_sip_command_sequence_number()
{
    int cseq;
    pthread_mutex_lock(&mx_sip);
    g_client.sn = (g_client.sn + 1) | ((g_client.sn + 1) == 0);
    cseq = g_client.sn;
    pthread_mutex_unlock(&mx_sip);
    return cseq;
}

//修改
int SipConfig::GetGBDeviceConfig(DeviceConfig_t *config)
{
    if(!strcmp(g_devsta.status_on,"ON"))
        config->status = 1;
    else
        config->status = 0;
    config->codeStream = g_sipinfo.codeStream;
    config->serverIp = g_sipinfo.serverIp;
    config->sipServerPort = g_sipinfo.sipServerPort;
    config->serverId = g_sipinfo.serverId;
    config->domain = g_sipinfo.realm;
    config->encode = !strcmp(g_sipinfo.deviceEncode,"OFF")?0:1;
    config->passwd = g_sipinfo.passwd;
    config->regExpires = g_sipinfo.regExpires;
    config->heartBeatInterval = g_sipinfo.heartBeatInterval;
    config->userid = g_sipinfo.userid;
    config->sipLoaclPort = g_sipinfo.sipLoaclPort;
    config->alertId = g_sipinfo.alertId;
    return 0;
}

//修改
bool SipConfig::GetDeviceStatus(void)
{
    if(!strcmp(g_devsta.status_on,"ON"))
        return true;
    else
        return false;
}

//修改
bool SipConfig::GetSipClientStatus()
{
    return !g_client.quit;
}

//修改
bool SipConfig::GetSipClientRegister()
{
    return g_client.mRegistered;
}

//修改
int SipConfig::GetRtpStreamFps(int *fps)
{
   *fps = g_rtpsendPs[0].fps;
    return 0;
}

//修改
int SipConfig::SetRtpStreamFps(int fps)
{
    int i;
    for(i=0;i<g_max_channels;i++)
        g_rtpsendPs[i].fps = fps;
    return 0;
}

//修改
int SipConfig::GetRtpRecordFileDir(int chn,char *dir)
{
    sprintf(dir, "%s", g_rtpsendPs[chn].recordDir);
    return 0;
}

//修改
int SipConfig::GetRtpFifoSize(int *width, int *height)
{
    *width = g_ringFifoSize.width;
    *height = g_ringFifoSize.height;
    return 0;
}

//修改
int SipConfig::SetRtpFifoSize(int width, int height)
{
    g_ringFifoSize.width = width;
    g_ringFifoSize.height = height;
    return 0;
}

//修改
int SipConfig::GetRtpStreamResolution(int *width, int *height)
{
    *width = g_streamSize.width;
    *height = g_streamSize.height;
    return 0;
}

//修改
int SipConfig::SetRtpStreamResolution(int width, int height)
{
    g_streamSize.width = width;
    g_streamSize.height = height;
    return 0;
}

int SipConfig::SendGbStreamData(int channel, unsigned char *buffer, int size, int encode_type)
{
    if(GetRtpStreamStatus(channel) != true)
        return -1;
    if(Config::ringput_fifochn(channel+REALTIME_FIFO_OFFSET, buffer, size, encode_type)==0)
        Config::ringreset_fifochn(MAX_CHN+REALTIME_FIFO_OFFSET);//buff full
    return 0;
}

bool SipConfig::GetRtpStreamStatus(int channel)
{
   
    bool status = g_rtpsendPs[channel].usefifo;
    return status;
}

//修改
void SipConfig::rtp_array_destroy(RtpSendPs **array)
{
    if (!array || !*array) return;
    
    free(*array);
    *array = NULL;
}

//修改
RtpSendPs *SipConfig::rtp_array_create(int channel)
{
    RtpSendPs* array = (RtpSendPs*)calloc(channel, sizeof(RtpSendPs));
    if (!array) return NULL;
    return array;
}

int SipConfig::gcd(int a, int b)
{
     while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}
