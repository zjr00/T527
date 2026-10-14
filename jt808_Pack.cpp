#include "jt808_Pack.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <string>

uint16_t Packag::result = 0x00;

/*功能：jt808数据解析*/

void Packag::set_camera_status(int bit_pos, int value)
{
    if (bit_pos < 0 || bit_pos > 15) {
        return ;   // 无效索引，返回当前值不变
    }

    if (value) {
        result |= (1 << bit_pos);   // 置1
    } else {
        result &= ~(1 << bit_pos);  // 清0
    }
}

/// @brief 设置参数回调
/// @param tslKey 回调函数id
/// @param callback
void Packag::addParameterCallback(int tslKey, OnParameterCallback callback)
{
    parameterCallbacks_[tslKey].push_back(callback);
}

void Packag::clearParameterCallbacks()
{
    parameterCallbacks_.clear();
}

void Packag::cleanup_exfields(jt808_msg_value_t *values, int count)
{
    if (!values || count <= 0) return;
    for (int i = 0; i < count; ++i) {
        if (values[i].type == JT808_DTYPE_STRING && values[i].value.d_string.data) {
            free((void*)values[i].value.d_string.data);
            values[i].value.d_string.data = NULL;
            values[i].value.d_string.len = 0;
        }
    }
}

/*****************************************************************************
//	描述：	将四个字节的数据存储到结构体中
//value 存储报文数据类型结构体
//id id
//v 需要存储的信息
*****************************************************************************/
int Packag::jt808_value_set_dword(jt808_msg_value_t *value, uint8_t id, DWORD v)
{
    if (!value)
    return -1;

	value->id = id;
	value->type = JT808_DTYPE_DWORD;
	value->value.d_dword = v;

	return 0;
}

/*****************************************************************************
//	描述：	将两个字节的数据存储到结构体中
//value 存储报文数据类型结构体
//id id
//v 需要存储的信息
*****************************************************************************/
int Packag::jt808_value_set_word(jt808_msg_value_t *value, uint8_t id, WORD v)
{
    if (!value) {
        printf("错误: jt808_value_set_word()接收到空指针! value=%p\n", value);
        return -1;
    }

	value->id = id;
	value->type = JT808_DTYPE_WORD;
	value->value.d_word = v;

	return 0;
}

int Packag::jt808_value_set_bytes(jt808_msg_value_t *value, uint8_t id, BYTE *v, int len)
{
   	if (!value) {
        printf("错误: jt808_value_set_bytes()接收到空指针! value=%p\n", value);
        return -1;
    }

	value->id = id;
	value->type = JT808_DTYPE_BYTE_ARR;
    value->value.d_bytes.data = v;
	value->value.d_bytes.len = len;

	return 0;
}


int Packag::jt808_value_set_string(jt808_msg_value_t *value, uint8_t id, const char *v)
{
    if (!value || !v) {
        printf("错误: jt808_value_set_string ()接收到空指针! value=%p, v=%p\n", value, v);
        return -1;
    }

    char* new_data = strdup(v);   // 等价于 malloc + strcpy
    if (!new_data) {
        printf("错误: strdup内存分配失败!\n");
        return -1;
    }
	value->id = id;
	value->type = JT808_DTYPE_STRING;
	value->value.d_string.data = new_data;
	value->value.d_string.len  = strlen(v);
	return 0;
}

int Packag::jt808_value_set_byte(jt808_msg_value_t *value, uint8_t id, BYTE v)
{
     if (!value) {
        printf("错误: jt808_value_set_byte()接收到空指针! value=%p\n", value);
        return -1;
    }

	value->id = id;
	value->type = JT808_DTYPE_BYTE;
	value->value.d_byte = v;

	return 0;
}

/// @brief 获取流水号
/// @param msgBody 报文
/// @return 
uint16_t Packag::Get_flowId(uint8_t *msgBody)
{

    uint16_t flowId = (msgBody[11] << 8) | msgBody[12];
    return flowId;
}

/// @brief 参数设置应答
/// @param msgBody 
/// @param value 
/// @return 
int Packag::Packag_0311(uint8_t *msgBody, jt808_msg_value_t *value)
{
    int sub = 13;
    int paraCount = msgBody[sub]; //参数个数
    int ext_field_index = 0;
    sub+=1;
    for (int  i = 0; i < paraCount; i++)
    {
        string cmdContent = "";
        int tslKey = msgBody[sub];
        sub+=1;
        int paraLength = msgBody[sub];
        sub+=1;
        for (int j = 0; j < paraLength; j++) {
            // 将每个字节转换为字符并添加到字符串
            cmdContent += (char)msgBody[sub + j];
        }

        sub+=paraLength;
        printf("tslKey = %d, paraLength = %d, cmdContent = %s\n", 
               tslKey, paraLength, cmdContent.c_str());
        
        switch (tslKey)
        {
        case 0x01: // 协同机的ip
        {
            jt808_value_set_byte(&value[ext_field_index],EXT_FIELD_MACHINE_IP,SetParameter01(cmdContent));
            ext_field_index++;
           
        }
        
           
            break;
        case 0x03://630服务器端口
        {
            jt808_value_set_byte(&value[ext_field_index],EXT_FIELD_630SERVER_PORT,SetParameter03(cmdContent));
            ext_field_index++;
        }
            break;
        case 0x04://ftp配置
        {
            jt808_value_set_byte(&value[ext_field_index],EXT_FIELD_FTP,SetParameter04(cmdContent));
            ext_field_index++;
        }
            break;
        case 0x06://道闸
        {
            jt808_value_set_byte(&value[ext_field_index],EXT_FIELD_FTP,SetParameter06(cmdContent));
            ext_field_index++;
        }
            break;
        case 0x07://地磅
        {
            jt808_value_set_byte(&value[ext_field_index],EXT_FIELD_FTP,SetParameter07(cmdContent));
            ext_field_index++;
        }
            break;
        
        default:
            break;
        }
    }

    return ext_field_index;
}


/// @brief 读取参数应答
/// @param msgBody 
/// @param value 
/// @return 
int Packag::Packag_0313(uint8_t *msgBody, jt808_msg_value_t *value)
{
    int sub = 13;
    int paraCount = msgBody[sub]; //参数个数
    int ext_field_index = 0;
    for (int  i = 0; i < paraCount; i++)
    {
        int tslKey = msgBody[sub+1+i];
        LOGI("tslKey =%d\n",tslKey);
        switch (tslKey)
        {
        case 0x01: // 协同机的ip
            jt808_value_set_string(&value[ext_field_index],EXT_FIELD_MACHINE_IP,parameter01());
            ext_field_index++;
            break;
        case 0x02: //ai相机的状态
            jt808_value_set_word(&value[ext_field_index],EXT_FIELD_AI_STATUS,parameter02());
            ext_field_index++;
            break;
        case 0x03://630服务器端口
            jt808_value_set_string(&value[ext_field_index],EXT_FIELD_630SERVER_PORT,parameter03().c_str());
            ext_field_index++;
            break;
        case 0x04: //终端向ftp服务器上传照片的地址路径
            jt808_value_set_string(&value[ext_field_index],EXT_FIELD_FTP,parameter04());
            ext_field_index++;
            break;
         case 0x05: //相机ip
            jt808_value_set_string(&value[ext_field_index],EXT_IPC_IP,parameter05());
            ext_field_index++;
            break;
        case 0x06: //道闸
            jt808_value_set_string(&value[ext_field_index],EXT_GATE_IPPROT,parameter06());
            ext_field_index++;
            break;
        case 0x07: //地磅
            jt808_value_set_string(&value[ext_field_index],EXT_WEIGH,parameter07());
            ext_field_index++;
            break;
        default:
            break;
        }
    }

    return ext_field_index;
    
}

int Packag::Packag_0315(uint8_t *msgBody, jt808_msg_value_t *value)
{
    int sub = 13;
    int paraCount = msgBody[sub]; //参数个数
    sub+=1;
    int ext_field_index = 0;
    uint8_t sealLockStatus = 0x01;  // 默认失败
    for (int  i = 0; i < paraCount; i++)
    {
        //char cmdContent[6];
        int tslKey = msgBody[sub];//获取参数ID
        sub+=1;
        int paraLength = msgBody[sub];
        sub+=1;
        switch (tslKey)
        {
        case 0x01: // 施封
        {

            string lock1,lock2;
            get_lockNum(lock1,lock2,msgBody,paraLength,sub);
            sub+=paraLength;

            //cout<<"lock2:"<<lock2<<endl;
            int ret = reader.TCPConnect();
            if (ret >= 0) {
                // 两个锁操作都成功才置0
                if (reader.NetSealLock(lock1.c_str()) == 0 &&
                    reader.NetL666_SealLock(lock2.c_str()) == 0) {
                    sealLockStatus = 0x00;
                }
            }
            jt808_value_set_byte(&value[ext_field_index], EXT_FIELD_SEALLOCK, sealLockStatus);
            
            
            ext_field_index++;
            reader.close_sock();
        }
          

            break;
        case 0x02: //解封
        {
            string lock1,lock2;
            get_lockNum(lock1,lock2,msgBody,paraLength,sub);
            sub+=paraLength;

            int ret = reader.TCPConnect();
            
            if (ret >= 0) {
                // 两个锁操作都成功才置0
                if (reader.NetUnSealLock(lock1.c_str()) == 0 &&
                    reader.NetL666_UnSealLock(lock2.c_str()) == 0) {
                    sealLockStatus = 0x00;
                }
            }
            jt808_value_set_byte(&value[ext_field_index], EXT_FIELD_SEALLOCK, sealLockStatus);
            
            
            ext_field_index++;
            reader.close_sock();
        }
            break;
        case 0x03://抓拍
        {
            jt808_value_set_byte(&value[ext_field_index], EXT_FIELD_SNAPSHOT, sealLockStatus);
            ext_field_index++;
            break;
        }
        case 0x04://抬竿
        {
            jt808_value_set_byte(&value[ext_field_index], EXT_FIELD_LIFT_BARRIER, sealLockStatus);
            ext_field_index++;
            break;
        }
        default:
            break;
        }
    }

    return ext_field_index;
}

void Packag::triggerCallbacks(int tslKey, const string &content)
{
    auto it = parameterCallbacks_.find(tslKey);
    if (it != parameterCallbacks_.end()) {
        for (auto& callback : it->second) {
            callback(tslKey, content);
        }
    }
}

/// @brief 剪切字符串
/// @param str 剪切的标志
/// @param delimiter 
/// @return 
vector<string> Packag::split(const string &str, char delimiter)
{
    vector<string> tokens;
    stringstream ss(str);
    string token;
    
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

string Packag::execShell(const string &cmd)
{
        unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    char buffer[128];
    string result;
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        result += buffer;
    }
    // 去除末尾换行
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

string Packag::getActiveInterface()
{
    string iface = execShell("ip route show default | awk '{print $5}'");
    if (iface.empty()) {
        // 备选命令（某些系统可能没有 ip 命令）
        iface = execShell("route -n | grep '^0.0.0.0' | awk '{print $8}'");
    }
    if (iface.empty()) {
        cerr << "无法获取当前活跃接口，将使用默认 eth0" << endl;
        iface = "eth0";
    }
    return iface;
}

BYTE Packag::SetParameter01(string data)
{
    // const string configFile = "/etc/network/interfaces";
    // const string backupFile = "/etc/network/interfaces.backup";
        
    // // 备份原配置文件
    // if (system(("cp " + configFile + " " + backupFile).c_str()) != 0) {
    //     cerr << "备份配置文件失败" << endl;
    //     return 0x01;
    // }
    
    // // 读取原文件内容
    // ifstream inputFile(configFile);
    // if (!inputFile.is_open()) {
    //     cerr << "无法打开配置文件: " << configFile << endl;
    //     return 0x01;
    // }

    // vector<string> lines;
    // string line;
    // bool inEth0Section = false;
    // bool eth0Modified = false;
        
    // while (getline(inputFile, line)) {
    //     // 检查是否进入 eth0 配置段
    //     if (line.find("auto eth0") != string::npos) {
    //         inEth0Section = true;
    //     }
        
    //     // 如果在 eth0 配置段中，修改相应的配置行
    //     if (inEth0Section) {
    //         if (line.find("address") != string::npos) {
    //             line = "address " + data;
    //             eth0Modified = true;
    //         }
            
    //         // 检查是否离开 eth0 配置段
    //         if (line.find("auto eth1") != string::npos || 
    //             (line.find("auto ") != string::npos && line != "auto eth0")) {
    //             inEth0Section = false;
    //         }
    //     }
        
    //     lines.push_back(line);
    // }
    // inputFile.close();

    // ofstream outputFile(configFile);
    // if (!outputFile.is_open()) {
    //     cerr << "无法写入配置文件: " << configFile << endl;
    //     return 0x01;
    // }
    
    // for (const auto& l : lines) {
    //     outputFile << l << endl;
    // }

    // outputFile.close();
    // cout << "配置文件已更新，正在重启网络服务..." << endl;
    //     // 方法1：使用 systemctl (较新的系统)
    // int result = system("systemctl restart networking");
    
    // if (result != 0) {
    //     // 方法2：使用传统 service 命令
    //     result = system("service networking restart");
    // }
    
    // if (result != 0) {
    //     // 方法3：直接使用 /etc/init.d/
    //     result = system("/etc/init.d/networking restart");
    // }
    
    // // 等待网络服务重启完成
    // sleep(3);

    //  const string configFile = "/etc/network/interfaces";
    // const string backupFile = "/etc/network/interfaces.backup";

    // // 1. 获取当前活跃接口
    // string activeIface = getActiveInterface();
    // cout << "检测到当前活跃接口: " << activeIface << endl;

    // // 2. 备份原配置文件
    // if (system(("cp " + configFile + " " + backupFile).c_str()) != 0) {
    //     cerr << "备份配置文件失败" << endl;
    //     return 0x01;
    // }

    // // 3. 读取并修改
    // ifstream inputFile(configFile);
    // if (!inputFile.is_open()) {
    //     cerr << "无法打开配置文件: " << configFile << endl;
    //     return 0x01;
    // }

    // vector<string> lines;
    // string line;
    // bool inTargetSection = false;
    // bool ipModified = false;

    // string targetAuto = "auto " + activeIface;      // 例如 "auto ens33"
    // string addressPrefix = "address";

    // while (getline(inputFile, line)) {
    //     // 检查是否进入目标接口配置段
    //     if (line.find(targetAuto) != string::npos) {
    //         inTargetSection = true;
    //     }

    //     // 如果在目标段内，修改 address 行
    //     if (inTargetSection) {
    //         // 去除行首空格以便更准确匹配
    //         string trimmed = line;
    //         trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    //         if (trimmed.find(addressPrefix) == 0) {  // 行以 "address" 开头
    //             line = addressPrefix + " " + data;
    //             ipModified = true;
    //         }

    //         // 检查是否离开该段：遇到其他 auto 开头的行（但不是目标 auto 行）
    //         if (line.find("auto ") != string::npos && line != targetAuto) {
    //             inTargetSection = false;
    //         }
    //     }

    //     lines.push_back(line);
    // }
    // inputFile.close();

    // if (!ipModified) {
    //     cerr << "警告: 在 " << activeIface << " 配置段中未找到 address 行，可能该接口为 DHCP 或配置格式不符" << endl;
    //     // 可以根据需要决定是否继续
    // }

    // // 4. 写回文件
    // ofstream outputFile(configFile);
    // if (!outputFile.is_open()) {
    //     cerr << "无法写入配置文件: " << configFile << endl;
    //     return 0x01;
    // }

    // for (const auto& l : lines) {
    //     outputFile << l << endl;
    // }
    // outputFile.close();

   
    // triggerCallbacks(0x01, activeIface);//回调协同机ip

        
   return 0x00;
}

BYTE Packag::SetParameter03(string data)
{
    size_t comma_pos = data.find(',');
    string port1 = data.substr(0, comma_pos);
    string port2 = data.substr(comma_pos + 1);

    if (Config::SetToFile("IPC","port1",port1)&& Config::SetToFile("IPC","port2",port2))
    {
        return 0x00;
    }
    
   
    return 0x01;
}

BYTE Packag::SetParameter04(string data)
{
    int ret;
    vector<string> parts = split(data, ',');

    string server = parts[0];      // "hhdlink.online:21"
    string path = parts[1];         // "/file"
    string username = parts[2];      // "admin"
    string password = parts[3];      // "123456"

    size_t comma_pos = server.find(':');
    string host = server.substr(0, comma_pos);//ip
    string port = server.substr(comma_pos + 1);//端口

    ret = Config::SetToFile("FTP","FTP_IP",host);
    ret &=Config::SetToFile("FTP","FTP_pass",password);
    ret &=Config::SetToFile("FTP","FTP_port",port);
    ret &=Config::SetToFile("FTP","FTP_user",username);
    ret &=Config::SetToFile("FTP","Remot_path",path);

    return ret;
}

BYTE Packag::SetParameter06(string data)
{
    size_t comma_pos = data.find(',');
    string IP = data.substr(0, comma_pos);
    string port = data.substr(comma_pos + 1);

    int ret;
    ret = Config::SetToFile("Gate","Ip",IP);
    ret &=Config::SetToFile("Gate","Port",port);

    return ret;
}

BYTE Packag::SetParameter07(string data)
{
    size_t comma_pos = data.find(',');
    string SerialPort = data.substr(0, comma_pos);
    string BaudRate = data.substr(comma_pos + 1);

    int ret;
    ret = Config::SetToFile("Communication","Weigh_SerialPort",SerialPort);
    ret &=Config::SetToFile("Communication","Weigh_BaudRate",BaudRate);
    return 0;
}

const char * Packag::parameter01()
{
    struct ifaddrs *ifaddr, *ifa;
    static string ipstr;   // 静态变量，保存最后找到的IP
    ipstr.clear();

    if (getifaddrs(&ifaddr) == -1) {
        printf("getifaddrs failed\n");
        return (char*)"";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        // 只关心 IPv4 地址
        if (ifa->ifa_addr->sa_family != AF_INET) continue;

        // 检查接口标志：必须 UP 且 RUNNING，且不是回环接口
        if ((ifa->ifa_flags & IFF_UP) &&
            (ifa->ifa_flags & IFF_RUNNING) &&
            !(ifa->ifa_flags & IFF_LOOPBACK)) {

            struct sockaddr_in* ipv4 = (struct sockaddr_in*)ifa->ifa_addr;
            char addr_buf[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &(ipv4->sin_addr), addr_buf, sizeof(addr_buf)) != nullptr) {
                ipstr = addr_buf;   // 复制到静态string
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ipstr.c_str();
}

uint16_t Packag::parameter02()
{
    return result;
}

string Packag::parameter03()
{
    return Config::Get("IPC","port1")+","+Config::Get("IPC","port2");
}

const char *Packag::parameter04()
{
    ostringstream oss;
    oss << Config::Get("FTP","FTP_IP")<<"," //IP
    << Config::Get("FTP","FTP_pass")<<","  //端口
    << Config::Get("FTP","Remot_path")<<"," //服务器保存路径
    << Config::Get("FTP","FTP_user")<<"," //用户名
    << Config::Get("FTP","FTP_pass"); //密码
   
    static string ftp;
    ftp = oss.str();
    return ftp.c_str();
}

const char *Packag::parameter05()
{
    return nullptr;
}

const char *Packag::parameter06()
{
    ostringstream oss;
    oss << Config::Get("Gate","Ip")<<"," //IP
    << Config::Get("Gate","Port"); //端口
   
    static string gate;
    gate = oss.str();
    return gate.c_str();
}

const char *Packag::parameter07()
{
    ostringstream oss;
    oss << Config::Get("Communication","Weigh_SerialPort")<<"," //串口
    << Config::Get("Communication","Weigh_SerialPort"); //波特率
   
    static string weigh;
    weigh = oss.str();
    return weigh.c_str();
}

void Packag::get_lockNum(string &lock1, string &lock2, uint8_t *msgBody, int length, int start)
{
    string result;
    for (int i = 0; i < length; i++) {

        uint8_t byte = msgBody[start + i];
       
        if (byte == 0x2C) {
            // 遇到逗号，直接添加到结果
            result.push_back(',');
            continue;
        }
        uint8_t high = (byte >> 4) & 0x0F;
        uint8_t low = byte & 0x0F;
        
        if (high <= 9) {
            result.push_back(high + '0');
        } else {
            // 非BCD，转换为十六进制字符
            result.push_back(high - 10 + 'a');
        }
        
        if (low <= 9) {
            result.push_back(low + '0');
        } else {
            result.push_back(low - 10 + 'a');
        }
    }
    size_t pos = result.find(',');

    if (pos != string::npos) {
        if (pos <= result.size()) {
            lock1 = result.substr(0, pos);
        }
        if (pos + 1 < result.size()) {
            lock2 = result.substr(pos + 1);
        }
    } 

   
}
