#ifndef JT808_PACK_H
#define JT808_PACK_H
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include "config.h"
#include "ReaderLock.h"

typedef uint16_t WORD;
typedef int DWORD;
typedef char  BYTE;

typedef union jt808_msg_item_data {
	BYTE  d_byte;
	WORD  d_word;
	DWORD d_dword;

	struct {
	    WORD len;
	    const char * data;
	} d_string;

	struct {
	    WORD len;
		const BYTE *data;
	} d_bytes;

	struct {
		WORD count;
		const WORD *data;
	} d_word_arr;
} jt808_msg_item_data_t;

typedef struct jt808_msg_value {
	short id;			//消息ID
	short type;			//消息类型
	jt808_msg_item_data_t value;			//消息值
} jt808_msg_value_t;

typedef enum {
	/* 基本类型 */
	JT808_DTYPE_BYTE,
	JT808_DTYPE_WORD,
	JT808_DTYPE_DWORD,
	JT808_DTYPE_BYTE_ARR,
	JT808_DTYPE_BCD,
	JT808_DTYPE_STRING,

	JT808_DTYPE_WORD_ARR_WITH_COUNT,     // 一个字节表示word字段的个数， 后面跟n个word
	JT808_DTYPE_NONE
} jt808_dtype_t;



typedef enum Terminal_parameter
{
    EXT_FIELD_MACHINE_IP =0x01, //协同机的ip
    EXT_FIELD_AI_STATUS = 0x02,//630的状态
    EXT_FIELD_630SERVER_PORT =0x03, //协同机中630的服务端端口
	EXT_FIELD_FTP =0x04, //协同机中FTP配置
	EXT_IPC_IP =0x05, //相机ip
	EXT_GATE_IPPROT =0x06, //道闸ip和端口
	EXT_WEIGH =0x07, //地磅


}Terminal_parameter_definition;

typedef enum Platform_parameter
{
    EXT_FIELD_SEALLOCK =0x01,//施封锁
    EXT_FIELD_UNSEALLOCK = 0x02,//解封锁
    EXT_FIELD_SNAPSHOT =0x03, //抓拍
	EXT_FIELD_LIFT_BARRIER =0x04,//抬竿
}Platform_parameter_definition;

typedef enum  report_platform
{
	EXT_FIELD_CarNum =0x01, //车牌
}report_platform_definition;

class Packag
{
public:

	void set_camera_status(int bit_pos, int value);
	// 定义回调函数类型
    using OnParameterCallback = function<void(int tslKey, const string& content)>;
    
    // 设置参数回调函数（可以有多个）
    void addParameterCallback(int tslKey, OnParameterCallback callback);
    
    // 清理所有回调
    void clearParameterCallbacks();
	
	void cleanup_exfields(jt808_msg_value_t *values, int count);



    int jt808_value_set_dword(jt808_msg_value_t *value, uint8_t id, DWORD v);
    int jt808_value_set_word(jt808_msg_value_t *value, uint8_t id, WORD v);
    int jt808_value_set_bytes(jt808_msg_value_t *value, uint8_t id, BYTE *v, int len);
    int jt808_value_set_string(jt808_msg_value_t *value, uint8_t id, const char * v);
    int jt808_value_set_byte(jt808_msg_value_t *value, uint8_t id, BYTE v);



    uint16_t Get_flowId(uint8_t *msgBody);


	int Packag_0311(uint8_t *msgBody,jt808_msg_value_t *value);
	int Packag_0313(uint8_t *msgBody,jt808_msg_value_t *value);
	int Packag_0315(uint8_t *msgBody,jt808_msg_value_t *value);

	// 触发回调
    void triggerCallbacks(int tslKey, const string& content);

	static uint16_t result;
    ReaderLock reader;
private:
	// 存储回调函数：键值 -> 回调函数列表
    unordered_map<int, vector<OnParameterCallback>> parameterCallbacks_;
    
 
	vector<string> split(const string& str, char delimiter);

	string execShell(const string& cmd);
	string getActiveInterface();
	BYTE SetParameter01(string data);
	BYTE SetParameter03(string data);
	BYTE SetParameter04(string data);
	BYTE SetParameter06(string data);
	BYTE SetParameter07(string data);


	
    const char * parameter01();
    uint16_t parameter02();
    string parameter03();
	const char * parameter04();
	const char * parameter05();
	const char * parameter06();
	const char * parameter07();

	void get_lockNum(string &lock1,string &lock2,uint8_t *msgBody,int length,int start);

	

};

#endif