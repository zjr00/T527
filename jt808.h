#ifndef JT808_H
#define JT808_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <stdint.h>


typedef uint16_t WORD;
typedef int DWORD;
typedef char  BYTE;
typedef uint32_t hhd_os_time_t;
typedef uint64_t hhd_os_tick_t;


#define JT808_MSG_TERM_HEART_BEAT               0x0002  // 心跳
#define JT808_MSG_TERM_LOCATION                   0x0200  // 位置信息汇报
#define PACKAGE_MAX_DATA_LENGTH  768    /**< 最大数据长度(数据字段序列化后的长度) */
#define EXT_FIELD_MAX_COUNT 20 // 报文打包过程中最多附加字段个数
#define PACKAGE_MAX_RETRY_COUNT  3      /**< 最大重发次数 */
#define HHD_FIRMWARE_VERSION "W-1000TF-1-V0_4_1-240619"
#define JT808_TYPE_HLINK    1   // 消息类型配置，航鸿达消息类型
#if JT808_TYPE_HLINK

	#define JT808_HEAD_POS_HEAD_START       1
	#define JT808_HEAD_POS_MSG_ID       (JT808_HEAD_POS_HEAD_START + 0)
	#define JT808_HEAD_POS_MSG_ATTR     (JT808_HEAD_POS_HEAD_START + 2)
	#define JT808_HEAD_POS_MSG_TERM     (JT808_HEAD_POS_HEAD_START + 4)
	#define JT808_HEAD_POS_MSG_SN       (JT808_HEAD_POS_HEAD_START + 10)
	#define JT808_BODY_START_POS        (JT808_HEAD_POS_HEAD_START + 12)

	#define JT808_HEAD_TERM_NO_LENGTH   6

#else

	#define JT808_HEAD_POS_HEAD_START   1
	#define JT808_HEAD_POS_MSG_ID       (JT808_HEAD_POS_HEAD_START + 0)
	#define JT808_HEAD_POS_MSG_ATTR     (JT808_HEAD_POS_HEAD_START + 2)
	#define JT808_HEAD_POS_MSG_VER      (JT808_HEAD_POS_HEAD_START + 4)
	#define JT808_HEAD_POS_MSG_TERM     (JT808_HEAD_POS_HEAD_START + 5)
	#define JT808_HEAD_POS_MSG_SN       (JT808_HEAD_POS_HEAD_START + 15)
	#define JT808_HEAD_POS_FRAGMENT     (JT808_HEAD_POS_HEAD_START + 17)

	#define JT808_HEAD_TERM_NO_LENGTH   10

#endif

// ---------------------------------------
// - 0200 固定字段
// ---------------------------------------
#define FIELD_0200_WARN_FLG 0
#define FIELD_0200_STAT 1
#define FIELD_0200_LATITUDE 2
#define FIELD_0200_LONGITUDE 3
#define FIELD_0200_ALTITUDE 4
#define FIELD_0200_SPEED 5
#define FIELD_0200_DIRECT 6
#define FIELD_0200_TIME 7
#define FIELD_0200_MAX_FIELD_COUNT 8

// ---------------------------------------
// - 0200 警报标识
// ---------------------------------------
#define HLINK_WARN_SUOGAN (1u)                   // 锁杆破坏
#define HLINK_WARN_WAIKE (1u << 1)               // 外壳破坏
#define HLINK_WARN_ABT_LOW (1u << 2)             // 电量低
#define HLINK_WARN_MCU_COMM (1u << 3)            // 主副MCU通讯异常
#define HLINK_WARN_SUOGAN_DAOXIAN (1u << 4)      // 锁杆/锁绳内部导线被剪报警
#define HLINK_WARN_GPS_TIANXIAN (1u << 5)        // 定位天线断路报警
#define HLINK_WARN_GPS_TIANXIAN_DUANLU (1u << 6) // 定位天线短路报警
#define HLINK_WARN_DIANJI_LOCK (1u << 7)         // 电机施封过程卡死报警
#define HLINK_WARN_DIANJI_UNLOCK (1u << 8)       // 电机解封过程卡死报警
#define HLINK_WARN_SPEED (1u << 9)               // 超速报警
#define HLINK_WARN_PARKING_TIMEOUT (1u << 10)    // 超时停车
#define HLINK_WARN_GNSS (1u << 11)               // GNSS模块发生故障
#define HLINK_WARN_POWER (1u << 12)              // 终端主电源掉电

// ---------------------------------------
// - 0200 状态位定义
// ---------------------------------------
#define HLINK_STAT_ACC (1)                   // 0: ACC关;1:ACC开
#define HLINK_STAT_GPS_VALID (1 << 1)        // 0:GPS无效定位;1:GPS有效定位
#define HLINK_STAT_SOUTH_LAT (1 << 2)        // 0:北纬:1:南纬
#define HLINK_STAT_WEST_LNG (1 << 3)         // 0:东经;1:西经
#define HLINK_STAT_SHORT_CONNECT (1 << 4)    // 0:长连接;1短连接
#define HLINK_STAT_GPS_ON (1 << 5)           // 0：定位模块关闭; 1:定位模块打开
#define HLINK_STAT_REST (1 << 6)             // 运动传感器状态-----0：运动状态; 1:静止状态
#define HLINK_STAT_LOCKED (1 << 14)          // 0：解封;   1:施封;
#define HLINK_STAT_SUOGAN (1 << 15)          // 0：锁杆开;  1:锁杆关;
#define HLINK_STAT_CHARGING (1 << 16)        // 00:无充电;   01：充电中;  10:充满电;
#define HLINK_STAT_CHARGED_FULL (1 << 17)    //
#define HLINK_STAT_PARKING_TIMEOUT (1 << 10) //
#define HLINK_STAT_2G (1 << 18)              // 00:缺省;  01:2G;  10:3G;  11:4G
#define HLINK_STAT_3G (1 << 19)              //
#define HLINK_STAT_4G (0x03 << 18)           //
#define HLINK_STAT_SIMCARD1 (1 << 20)        // 0:缺省;  01:当前sim卡1;  10:当前sim卡2
#define HLINK_STAT_SIMCARD2 (1 << 21)        //

// ---------------------------------------
// - 0200 扩展字段定义
// ---------------------------------------
enum EXT_FIELD_0200
{
    EXT_FIELD_0200_MILEAGE = 0x01,        // 里程
    EXT_FIELD_0200_FUEL_LEFT = 0x02,      // 油量
    EXT_FIELD_0200_TANK_FUEL_CAP = 0x03,  // 油罐油量
    EXT_FIELD_0200_LOAD_SENSOR = 0x04,    // 载重传感器
    EXT_FIELD_0200_TEMPERATURE_SENSOR = 0x05,    // 温度传感器
    EXT_FIELD_0200_SMOKE_SENSOR = 0x06,          // 烟雾传感器
    EXT_FIELD_0200_WATER_SENSOR = 0x07,          // 水浸传感器
    EXT_FIELD_0200_LOCK_BLUETOOTH = 0x61,        // 电子锁蓝牙MAC/ID号
    EXT_FIELD_0200_LOCK_STATUS_INFO = 0x62,      // 电子锁状态信息
    EXT_FIELD_0200_WIFI_MAC = 0x50,       // WIFI_MAC信息最大不超过8组
    EXT_FIELD_0200_BLE_MAC = 0x51,        // BLE_MAC信息最大不超过8组
    EXT_FIELD_0200_EVENT = 0x60,          // 事件信息
    EXT_FIELD_0200_SUB_LOCK_STAT = 0x63,  // 子锁状态数据
    EXT_FIELD_0200_LORA = 0x64,           // LORA温湿度数据
    EXT_FIELD_0200_LBS = 0x65,            // LBS解析过后的坐标
    EXT_FIELD_0200_BASE_STATION = 0x66,   // 基站信息
    EXT_FIELD_0200_DYNAMIC_PASS = 0x67,   // 键盘开锁动态密码
    EXT_FIELD_0200_BATTERY = 0x68,        //  1   电池电量百比分1Byte（0%-100%）
    EXT_FIELD_0200_VOLTAGE = 0x69,        //  2   电池电压值：2Byte(单位:10mV)
    EXT_FIELD_0200_CSQ = 0x6a,            //  1   网络CSQ信号值：1Byte（范围：0-31）
    EXT_FIELD_0200_GPS_STAR = 0x6b,       //  1   使用的卫星数量： 1Byte
    EXT_FIELD_0200_SIM_IMSI = 0x6C,       //  SIM_IMSI值[N]    IMSI[N]  SIM卡的IMSI码----网络重联时触发上传
    EXT_FIELD_0200_CLIENT_ID = 0x6D,      //  BYTE[N] client id（可以通过0X0310配置,配置后每条0200报文都将带有客户ID进行上传）
    EXT_FIELD_0200_OTA_RESULT = 0x6E,     //  STRING  终端升级结果：升级结果,当前版本号，示例：0,G4-360_20220301，级结果：0 – 升级成功，1 – 升级失败
    EXT_FIELD_0200_SIM_ICCID = 0x71,      //  SIM_ICCID   SIM卡ICCID
    EXT_FIELD_0200_BUSINESS_DATA = 0x78,  //  业务数据196字节
    EXT_FIELD_0200_DATA_LORA = 0x79,      //  按整条信息长度（LORA防拆标签数据）
    EXT_FIELD_0200_REPORTED_TIME = 0x80,  /*!< 当前回传间隔模式+下次0x200上报间隔*/
    EXT_FIELD_0200_REPORTED_VIDEO = 0xa0, // 照片/短视频文件ID.（视频终端专用）
};

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


typedef enum jt808_send_channel_type {
    JT808_SEMD_CHANNEL_PLATFORM = 0,
    JT808_SEMD_CHANNEL_BLE      = 1,
    JT808_SEMD_CHANNEL_INVALID = 0xffu
} jt808_send_channel_type_t;

typedef enum {
    TASK_MAIN,
    TASK_GPRS,
    TASK_JT808,
    TASK_NFC,
    TASK_PROCESS,
    TASK_PROCESS_MCU1,
    TASK_SCHEDULE,
    TASK_STATE,
    TASK_OTA,
//    TASK_DEPUTY_MCU_OTA,
    TASK_MCUCOM,
    TASK_BLE,
    TASK_LORAWAN,
    TASK_STATE_MCU1,
	TASK_WIFI,
	TASK_SLEEP_MCU1,
	TASK_SLEEP_MCU0,
    TASK_DISPLAY,
    TASK_LORA,
    TASK_LORA_MCU1,
    TASK_RSA,
    TASK_SMS,
    TASK_STM1,
    TASK_STM2,
    TASK_MAX_COUNT
} sys_task_id_t;

typedef enum sys_ipc_request_type {
    IPC_REQ_TYPE_UNKNOWN = 0,           /*!< IPC_REQ_TYPE_UNKNOWN */
    IPC_REQ_TYPE_AGPS,              /*!< IPC_REQ_TYPE_AGPS */
    IPC_REQ_TYPE_GPRS_AGPS,         /*!< IPC_REQ_TYPE_GPRS_AGPS */
    IPC_REQ_TYPE_OTA,               /*!< IPC_REQ_TYPE_OTA */
    IPC_REQ_TYPE_DEPUTY_MCU_OTA,    /*!< IPC_REQ_TYPE_DEPUTY_MCU_OTA */
    IPC_REQ_TYPE_DEPUTY_MCU_OTA_RDY,/*!< IPC_REQ_TYPE_DEPUTY_MCU_OTA_RDY */
    IPC_REQ_TYPE_OTA_DATA,          /*!< IPC_REQ_TYPE_OTA_DATA */
    IPC_REQ_TYPE_OTA_DATA_ACK,      /*!< IPC_REQ_TYPE_OTA_DATA_ACK */
    IPC_REQ_TYPE_PASSWD_UNLOCK,     /*!< IPC_REQ_PASSWD_UNLOCK */
    IPC_REQ_TYPE_NFC_LOCK_UNLOCK,   /*!< IPC_REQ_TYPE_NFC_LOCK_UNLOCK */
    IPC_REQ_TYPE_DISPLAY_CTRL,      /*!< IPC_REQ_TYPE_DISPLAY_CTRL */
    IPC_REQ_TYPE_EVENT_LOG,         /*!< IPC_REQ_TYPE_EVENT */
    IPC_REQ_INTER_MCU_DATA,         /*!< IPC_REQ_INTER_MCU_DATA */
    IPC_REQ_SEND_REQUEST,           /*!< IPC_REQ_SEND_REQUEST */
    IPC_REQ_JT808_0200,             /*!< IPC_REQ_JT808_0200 */
    IPC_REQ_RECVED_JT808_REQ,       /*!< IPC_REQ_RECVED_JT808_REQ */
    IPC_REQ_RECVED_BLE_REQ,         /*!< IPC_REQ_RECVED_BLE_REQ */
    IPC_REQ_SYS_SLEEP,              /*!< IPC_REQ_SYS_SLEEP */
	IPC_REQ_TYPE_AUTO_LOCK,         /*!< IPC_REQ_TYPE_AUTO_LOCK */
	IPC_REQ_TYPE_LORAWAN_CTRL,      /*!< IPC_REQ_TYPE_LORAWAN_CTRL */
	IPC_REQ_TYPE_BEEP,              /*!< IPC_REQ_TYPE_LORAWAN_CTRL */
	IPC_REQ_JT808_210,
	IPC_REQ_TYPE_MOTOR,
	IPC_REQ_SLEEP_WAKEUP,
	IPC_REQ_UPDATE_CONNECTION,
	IPC_REQ_JT808_0002,
	IPC_REQ_TYPE_GPS_DATA,
	IPC_REQ_TYPE_AGPS_DATA,
	IPC_REQ_TYPE_BLE_DATA,
	IPC_REQ_TYPE_LED,
    IPC_HEART_BEAT,
    IPC_DISPLAY_REQUEST,
    IPC_REQ_LORA_PARAM_DATA,
    IPC_REQ_RECVED_LORA_REQ,
    IPC_REQ_JT808_0610,
    IPC_REQ_BUSINESS_UPDATE,
    IPC_REQ_RECVED_JT808_BLE_REQ = 0x24,
	IPC_REQ_TYPE_MAX                /*!< IPC_REQ_TYPE_MAX */
} sys_ipc_request_type_t;


typedef enum hhd_event_type
{
    HHD_EV_SUOGAN_IN = 0x0000u,                       /**>  锁杆闭合事件  0x00（普通）      */
    HHD_EV_SUOGAN_OUT = 0x0001u,                      /**>  锁杆打开事件  0x00（普通）      */
    HHD_EV_LOCK_BY_SUOGAN_IN = 0x0002u,               /**>  闭合锁杆施封事件;   0x00（普通） */
    HHD_EV_LOCK_BY_NFC = 0x0003u,                     /**>  刷卡施封成功事件;   0x00（普通） */
    HHD_EV_UNLOCK_BY_NFC = 0x0004u,                   /**>  刷卡解封成功事件;   0x00（普通） */
    HHD_EV_LOCK_BY_BLE = 0x0005u,                     /**>  BLE施封成功事件;  0x00（普通）  */
    HHD_EV_UNLOCK_BY_BLE = 0x0006u,                   /**>  BLE解封成功事件;  0x00（普通）  */
    HHD_EV_LOCK_BY_PLATFORM = 0x0007u,                /**>  平台施封成功事件;   0x00（普通） */
    HHD_EV_UNLOCK_BY_PLATFORM = 0x0008u,              /**>  平台解封成功事件;   0x00（普通） */
    HHD_EV_LOCK_BY_SMS = 0x0009u,                     /**>  SMS施封成功事件;  0x00（普通）  */
    HHD_EV_UNLOCK_BY_SMS = 0x000Au,                   /**>  SMS解封成功事件;  0x00（普通）  */
    HHD_EV_ERR_LOCK_INVALID_NFC = 0x000Bu,            /**>  刷非注册卡施封失败事件;    0x00（普通） */
    HHD_EV_ERR_UNLOCK_INVALID_NFC = 0x000Cu,          /**>  刷非注册卡解封失败事件;    0x00（普通） */
                                                      /*----------------------------------------------------------------------------------------------*/
    HHD_EV_NFC_LOCK_OUTOF_AREA = 0x000Du,             /**>  区域外刷卡施封失败事件 0x01（报警） */
    HHD_EV_NFC_UNLOCK_OUTOF_AREA = 0x000Eu,           /**>  区域外刷卡解封失败事件 0x01（报警） */
    HHD_EV_SUOGAN_OUT_IN_LOCK = 0x000Fu,              /**>  施封状态下锁杆被打开事件    0x01（报警） */
    HHD_EV_WAIKE = 0x0010u,                           /**>  外壳被拆事件  0x01（报警） */
    HHD_EV_FANGJIAN = 0x0011u,                        /**>  锁杆/锁绳内导线被剪断事件   0x01（报警） */
    HHD_EV_LOW_VOLTAGE = 0x0012u,                     /**>  电池电压过低休眠下线  0x01(报警) */
    HHD_EV_INVALID_PASSWORD = 0x0013u,                /**>  输入错误密码次数过多  0x01(报警) */
    HHD_EV_WAIT_LOCK_TIMEOUT = 0x0014u,               /**>  解封后长时间未拉开锁杆，自动施封;   0x00（普通）*/
    HHD_EV_DAOLI_WAKEUP = 0x0015u,                    /**>  短连接倒立激活上线   0x00（普通） */
    HHD_EV_TOUCH_WAKEUP = 0x0016u,                    /**>  短连接触摸激活上线   0x00（普通） */
    HHD_EV_USER_LOW_VOLTAGE = 0x0017u,                /**>  用户设定的低电量阈值报警提醒  0x01（报警） */
    HHD_EV_LOCK_MOTOR_STUCK = 0x0018u,                /**>  施封时候电机卡住报警  0x01（报警） */
    HHD_EV_UNLOCK_MOTOR_STUCK = 0x0019u,              /**>  解封时候电机卡住报警  0x01（报警） */
    HHD_EV_GPS_ANTEN_OPEN = 0x001Au,                  /**>  定位天线开路报警    0x01（报警） */
    HHD_EV_GPS_ANTEN_SHORT_CIRCUIT = 0x001Bu,         /**>  定位天线短路报警   0x01（报警） */
    HHD_EV_MCU_COMMUNICAT = 0x001Cu,                  /**>  MCU通信异常报警  0x01（报警） */
    HHD_EV_CHARG_FULL = 0x001Du,                      /**>  充满电时    0x00（普通）  */
    HHD_EV_CHARGER_PULLOUT = 0x001Eu,                 /**>  拔除充电器   0x00（普通）  */
    HHD_EV_CHARGER_INSERT = 0x001Fu,                  /**>  连接充电器充电 0x00（普通） */
    HHD_EV_NORMAL_SHUTDOWN = 0x0020u,                 /**> 设备正常关机提醒 0x00（普通） */
    HHD_EV_UNLOCK_BY_STATIC_PSWORD = 0x0022u,         /**>  静态密码开锁事件 0x00（普通） */
    HHD_EV_LOCK_PHONE_BLE = 0x0023u,                  /**>  手机端上报蓝牙施封 0x00（普通） */
    HHD_EV_UNLOCK_PHONE_BLE = 0x0024u,                /**>  手机端上报蓝牙解封 0x00（普通） */
    HHD_EV_BUSINESS_DATA_PLATFORM = 0x0025u,          /**>  业务数据发生变更 0x00（普通） */
    HHD_EV_UNLOCK_BY_RANDOM_PSWORD = 0x0026u,         /**>  动态密码开锁事件 0x00（普通） */
    HHD_EV_BUSINESS_DATA_BLE = 0x002Bu,               /**>  蓝牙写入业务数据 0x00（普通） */
    HHD_EV_BUSINESS_DATA_GSENSOR = 0x002Cu,           /**>  上报翻转事件     0x00（普通） */
    HHD_EV_UNLOCK_SUBLOCK_BY_PLATFORM_LORA = 0x0051u, /**>  母锁操作：平台远程经母锁LORA解封子锁事件 0x00（普通） */
    HHD_EV_UNLOCK_SUBLOCK_BY_BLE = 0x0052u,           /**>  自身操作：APP通过BLE解封子锁事件 0x00（普通） */
    HHD_EV_UNLOCK_SUBLOCK_BY_BLE_LORA = 0x0053u,      /**>  母锁操作：APP通过BLE经母锁LORA解封子锁事件 0x00（普通） */
    HHD_EV_UNLOCK_SUBLOCK_BY_SMS_LORA = 0x0054u,      /**>  母锁操作：远程SMS经母锁LORA解封子锁事件 0x00（普通）  */
    HHD_EV_LOCK_SUBLOCK_BY_SUOGAN_IN = 0x0055u,       /**>  自身操作：子锁闭合锁杆自动施封事件 0x00（普通）  */
    HHD_EV_UNLOCK_SUBLOCK_BY_SUOGAN_OUT = 0x0056u,    /**>  自身操作：子锁解封后打开锁杆事件 0x00（普通）  */
    HHD_EV_SUBLOCK_BY_WAIKE_OPEN = 0x0057u,           /**>  自身操作：子锁外壳被拆事件 0x01（报警）  */
    HHD_EV_LOCK_SUBLOCK_BY_SUOGAN_EXC = 0x0058u,      /**>  自身操作：子锁施封状态下锁杆被打开/被剪事件(异常) 0x01（报警）  */
    HHD_EV_SUBLOCK_BY_TIMEOUT = 0x0059u,              /**>  自身操作：子锁通讯超时事件 0x01（报警）  */
    HHD_EV_SUBLOCK_BY_BATTERY_LOW = 0x005Au,          /**>  自身操作：子锁低电量报警事件 0x01（报警）  */
    //                                           = 0x005Bu,                /**>  该事件待定 母锁操作：检测到行驶自动禁止LORA解封事件 0x00（普通）  */
    HHD_EV_SET_UNLOCK_SUBLOCK_BY_BLE_LORA = 0x005Cu,      /**>  母锁操作：BLE设置LORA解封事件; 0x00（普通） */
    HHD_EV_SET_LOCK_SUBLOCK_BY_PLATFORM_LORA = 0x005Du,   /**>  母锁操作：平台设置LORA施封事件; 0x00（普通） */
    HHD_EV_SET_UNLOCK_SUBLOCK_BY_PLATFORM_LORA = 0x005Eu, /**>  母锁操作：平台设置LORA解封事件; 0x00（普通） */
    HHD_EV_SET_LOCK_SUBLOCK_BY_SMS_LORA = 0x005Fu,        /**>  母锁操作：SMS设置LORA施封事件; 0x00（普通） */
    HHD_EV_SET_UNLOCK_SUBLOCK_BY_SMS_LORA = 0x0060u,      /**>  母锁操作：SMS设置LORA解封事件; 0x00（普通） */
    HHD_EV_UNLOCK_SUBLOCK_BY_NFC = 0x0061u,               /**>  自身操作：子锁刷自身注册卡解封 0x00（普通） */
    HHD_EV_LOCK_SUBLOCK_BY_NFC = 0x0062u,                 /**>  自身操作：子锁刷自身注册卡施封 0x00（普通） */
    HHD_EV_SUBLOCK_BY_UNLOCK_INVALID_NFC = 0x0063u,       /**>  母锁操作：子锁刷非注册解封卡（子锁与母锁都没有绑定该卡） 0x00（普通） */
    HHD_EV_SUBLOCK_BY_LOCK_INVALID_NFC = 0x0064u,         /**>  母锁操作：子锁刷非注册施封卡（子锁与母锁都没有绑定该卡） 0x00（普通） */
    HHD_EV_UNLOCK_SUBLOCK_BY_NFC_LORA = 0x0065u,          /**>  母锁操作：子锁刷卡经母锁解封成功 0x00（普通） */
    HHD_EV_LOCK_SUBLOCK_BY_NFC_LORA = 0x0066u,            /**>  母锁操作：子锁刷卡经母锁施封成功 0x00（普通） */
    HHD_EV_LOCK_SUBLOCK_BY_PLATFORM_LORA = 0x0067u,       /**>  母锁操作：平台远程经母锁LORA施封子锁事件 0x00（普通） */
    HHD_EV_LOCK_SUBLOCK_BY_BLE = 0x0068u,                 /**>  自身操作：APP通过BLE施封子锁事件 0x00（普通） */
    HHD_EV_LOCK_SUBLOCK_BY_BLE_LORA = 0x0069u,            /**>  母锁操作：APP通过BLE经母锁LORA施封子锁事件 0x00（普通） */
    HHD_EV_LOCK_SUBLOCK_BY_SMS_LORA = 0x006Au,            /**>  母锁操作：远程SMS经母锁LORA施封子锁事件 0x00（普通） */
    HHD_EV_SET_LOCK_SUBLOCK_BY_BLE_LORA = 0x006Bu,        /**>  母锁操作：BLE设置LORA施封事件; 0x00（普通） */
    HHD_EV_LOCK_SUBLOCK_BY_TIME_OUT = 0x006Cu,            /**>  自身操作：子锁解封后长时间未拉开锁杆，自动施封; 0x00（普通） */

    /*LORA防拆标签设备相关的事件*/
    HHD_PLATFORM_LORA_BE_DISMANTLED = 0x0080u,       /**>  LORA防拆标签被拆*/
    HHD_PLATFORM_LORA_TIMEOUT = 0x0081u,             /**>  LORA防拆标签心跳超时 */
    HHD_PLATFORM_LORA_RETURN_TO_NORMAL_BE = 0x0082u, /**>  LORA防拆标签被拆恢复正常 */
    HHD_PLATFORM_LORA_RETURN_TO_NORMAL = 0x0083u,    /**>  LORA防拆标签心跳超时恢复正常 */

    HHD_EV_FIRMWARE_VERSION = 0x0090u, /**>  固件版本上报事件 */

    /*驾驶行为事件*/
    HHD_EV_OVERSPEED_ALARM                      = 0x00A0u,               /**>  超速报警事件 */                                  
    HHD_EV_PARKINT_OVERTIME_ALARM               = 0x00A1u,               /**>  停车超时报警事件 */     

        /*电子围栏相关事件*/
    //刷卡
    HHD_EV_NO_VALID_TIME_NFC_UNLOCK_FAIL         = 0x0030u,      /**>  电子围栏非有效截止日期刷卡，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_NFC_LOCK_FAIL           = 0x0031u,      /**>  电子围栏非有效截止日期刷卡，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_NFC_UNLOCK_FAIL  = 0x0032u,    /**>  电子围栏非有效时间区间刷卡，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_NFC_LOCK_FAIL  = 0x0033u,      /**>  电子围栏非有效时间区间刷卡，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_CARD_ID_NFC_LOCK_FAIL        = 0x0048u,      /**>  非有效围栏规则卡施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_CARD_ID_NFC_UNLOCK_FAIL      = 0x0049u,      /**>  非有效围栏规则卡解封失败事件     0x00（普通） */
    HHD_EV_IN_AREA_NFC_UNLOCK_SUCCESS            = 0x004Au,      /**>  电子围栏内刷卡解封成功事件     0x00（普通） */
    HHD_EV_IN_AREA_NFC_LOCK_SUCCESS              = 0x004Bu,      /**>  电子围栏内刷卡施封成功事件     0x00（普通） */
    HHD_EV_OUT_AREA_NFC_UNLOCK_FAIL              = 0x004Cu,      /**>  电子围栏外刷卡解封失败事件     0x00（普通） */
    HHD_EV_OUT_AREA_NFC_LOCK_FAIL                = 0x004Du,      /**>  电子围栏外刷卡施封失败事件     0x00（普通） */
    //密码
    HHD_EV_NO_VALID_PASSWORD_UNLOCK_FAIL         = 0x0034u,      /**>  电子围栏内非有效密码，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_PASSWORD_LOCK_FAIL           = 0x0035u,      /**>  电子围栏内非有效密码，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_PASSWORD_UNLOCK_FAIL    = 0x0036u,      /**>  电子围栏内非有效截止日期密码，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_PASSWORD_LOCK_FAIL      = 0x0037u,      /**>  电子围栏内非有效截止日期密码，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_PASSWORD_UNLOCK_FAIL = 0x0038u,/**>  电子围栏内非有效时间区间密码，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_PASSWORD_LOCK_FAIL = 0x0039u,  /**>  电子围栏内非有效时间区间密码，施封失败事件     0x00（普通） */
    HHD_EV_IN_AREA_PASSWORD_UNLOCK_SUCCESS       = 0x004Eu,      /**>  电子围栏内密码解封成功事件     0x00（普通） */
    HHD_EV_OUT_AREA_PASSWORD_UNLOCK_FAIL          = 0x004Fu,      /**>  电子围栏外密码解封失败事件     0x00（普通） */
    HHD_EV_IN_AREA_PASSWORD_LOCK_SUCCESS         = 0x0114u,        /**>  电子围栏内密码施封成功事件     0x00（普通） */
    HHD_EV_OUT_AREA_PASSWORD_LOCK_FAIL           = 0x0115u,        /**>  电子围栏外密码施封失败事件     0x00（普通） */
    //SMS
    HHD_EV_NO_VALID_SMS_UNLOCK_FAIL              = 0x003au,      /**>  电子围栏内非有效SMS参数，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_SMS_LOCK_FAIL                = 0x003bu,      /**>  电子围栏内非有效SMS参数，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_SMS_UNLOCK_FAIL         = 0x003cu,      /**>  电子围栏非有效截止日期SMS，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_SMS_LOCK_FAIL           = 0x003du,      /**>  电子围栏非有效截止日期SMS，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_SMS_UNLOCK_FAIL = 0x003eu,     /**>  电子围栏非有效时间区间SMS，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_SMS_LOCK_FAIL  = 0x003fu,      /**>  电子围栏非有效时间区间SMS，施封失败事件     0x00（普通） */
    HHD_EV_IN_AREA_SMS_LOCK_SUCCESS              = 0x0100u,      /**>  电子围栏内SMS施封成功事件     0x00（普通） */
    HHD_EV_IN_AREA_SMS_UNLOCK_SUCCESS            = 0x0101u,      /**>  电子围栏内SMS解封成功事件     0x00（普通） */
    HHD_EV_OUT_AREA_SMS_LOCK_FAIL                = 0x0102u,      /**>  电子围栏外SMS施封失败事件     0x00（普通） */
    HHD_EV_OUT_AREA_SMS_UNLOCK_FAIL              = 0x0103u,      /**>  电子围栏外SMS解封失败事件     0x00（普通） */
    //BLE
    HHD_EV_NO_VALID_BLE_UNLOCK_FAIL              = 0x0040u,      /**>  电子围栏内非有效BLE参数，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_BLE_LOCK_FAIL                = 0x0041u,      /**>  电子围栏内非有效BLE参数，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_BLE_UNLOCK_FAIL         = 0x0042u,      /**>  电子围栏非有效截止日期BLE，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_BLE_LOCK_FAIL           = 0x0043u,      /**>  电子围栏非有效截止日期BLE，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_BLE_UNLOCK_FAIL = 0x0044u,     /**>  电子围栏非有效时间区间BLE，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_BLE_LOCK_FAIL  = 0x0045u,      /**>  电子围栏非有效时间区间BLE，施封失败事件     0x00（普通） */
    HHD_EV_IN_AREA_BLE_LOCK_SUCCESS              = 0x0104u,      /**>  电子围栏内蓝牙施封成功事件     0x00（普通） */
    HHD_EV_IN_AREA_BLE_UNLOCK_SUCCESS            = 0x0105u,      /**>  电子围栏内蓝牙解封成功事件     0x00（普通） */
    HHD_EV_OUT_AREA_BLE_UNLOCK_FAIL              = 0x0106u,      /**>  电子围栏外蓝牙解封失败事件     0x00（普通） */
    HHD_EV_OUT_AREA_BLE_LOCK_FAIL                = 0x0107u,      /**>  电子围栏外蓝牙施封失败事件     0x00（普通） */

    //平台（远程）
    HHD_EV_NO_VALID_PLATFORM_UNLOCK_FAIL              = 0x0108u,      /**>  电子围栏内非有效平台参数，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_PLATFORM_LOCK_FAIL                = 0x0109u,      /**>  电子围栏内非有效平台参数，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_PLATFORM_UNLOCK_FAIL         = 0x010Au,      /**>  电子围栏非有效截止日期平台，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_PLATFORM_LOCK_FAIL           = 0x010Bu,      /**>  电子围栏非有效截止日期平台，施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_PLATFORM_UNLOCK_FAIL = 0x010Cu,     /**>  电子围栏非有效时间区间平台，解封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_PLATFORM_LOCK_FAIL  = 0x010Du,      /**>  电子围栏非有效时间区间平台，施封失败事件     0x00（普通） */
    HHD_EV_IN_AREA_PLATFORM_LOCK_SUCCESS              = 0x010Eu,      /**>  电子围栏内远程施封成功事件     0x00（普通） */
    HHD_EV_IN_AREA_PLATFORM_UNLOCK_SUCCESS            = 0x010Fu,      /**>  电子围栏内远程解封成功事件     0x00（普通） */
    HHD_EV_OUT_AREA_PLATFORM_UNLOCK_FAIL              = 0x0110u,      /**>  电子围栏外远程解封失败事件     0x00（普通） */
    HHD_EV_OUT_AREA_PLATFORM_LOCK_FAIL                = 0x0111u,      /**>  电子围栏外远程施封失败事件     0x00（普通） */

    //触摸
    HHD_EV_NO_VALID_TIME_TOUCH_LOCK_FAIL           = 0x0046u,      /**>  围栏截止日期后触摸施封失败事件     0x00（普通） */
    HHD_EV_NO_VALID_TIME_INTERVAL_TOUCH_LOCK_FAIL  = 0x0047u,      /**>  非有效围栏时间触摸施封失败事件     0x00（普通） */
    HHD_EV_IN_AREA_TOUCH_LOCK_SUCCESS              = 0x0112u,      /**>  电子围栏内触摸施封成功事件     0x00（普通） */
    HHD_EV_OUT_AREA_TOUCH_LOCK_FAIL                = 0x0113u,      /**>  电子围栏外触摸施封失败事件     0x00（普通） */

    //子锁围栏事件
    //远程
    HHD_EV_SUBLOCK_UNVALID_PLATFORM_UNLOCK_FAIL  = 0x0116u,               /**>  子锁非有效围栏经主网关远程参数解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_PLATFORM_LOCK_FAIL    = 0x0117u,               /**>  子锁非有效围栏经主网关远程参数施封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_PLATFORM_TIME_UNLOCK_FAIL     = 0x0118u,       /**>  子锁围栏截止日期后经主网关远程解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_PLATFORM_TIME_LOCK_FAIL       = 0x0119u,       /**>  子锁围栏截止日期后经主网关远程施封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_PLATFORM_UNLOCK_FAIL = 0x011Au,    /**>  子锁非有效围栏时间经主网关远程解封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_PLATFORM_LOCK_FAIL   = 0x011Bu,    /**>  子锁非有效围栏时间经主网关远程施封失败事件 */
    HHD_EV_SUBLOCK_OUT_AREA_PLATFORM_UNLOCK_FAIL = 0x011Cu,               /**>  子锁电子围栏外经主网关远程解封失败事件 */
    HHD_EV_SUBLOCK_OUT_AREA_PLATFORM_LOCK_FAIL   = 0x011Eu,               /**>  子锁电子围栏外经主网关远程施封失败事件 */
    HHD_EV_SUBLOCK_IN_AREA_PLATFORM_UNLOCK_SUCCESS = 0x0145u,             /**>  子锁电子围栏内经主网关远程解封成功事件 */
    HHD_EV_SUBLOCK_IN_AREA_PLATFORM_LOCK_SUCCESS = 0x0146u,               /**>  子锁电子围栏内经主网关远程施封成功事件 */

    //BLE
    HHD_EV_SUBLOCK_UNVALID_BLE_UNLOCK_FAIL             = 0x011Fu,         /**>  子锁非有效围栏经主网关BLE参数解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_BLE_LOCK_FAIL               = 0x0120u,         /**>  子锁非有效围栏经主网关BLE参数施封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_BLE_TIME_UNLOCK_FAIL        = 0x0121u,         /**>  子锁围栏截止日期后经主网关BLE解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_BLE_TIME_LOCK_FAIL          = 0x0122u,         /**>  子锁围栏截止日期后经主网关BLE施封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_BLE_UNLOCK_FAIL = 0x0123u,         /**>  子锁非有效围栏时间经主网关蓝牙解封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_BLE_LOCK_FAIL   = 0x0124u,         /**>  子锁非有效围栏时间经主网关蓝牙施封失败事件 */
    HHD_EV_SUBLOCK_OUT_AREA_BLE_UNLOCK_FAIL            = 0x0125u,         /**>  子锁电子围栏外经主网关蓝牙解封失败事件 */
    HHD_EV_SUBLOCK_OUT_AREA_BLE_LOCK_FAIL              = 0x0126u,         /**>  子锁电子围栏外经主网关蓝牙施封失败事件 */
    HHD_EV_SUBLOCK_IN_AREA_BLE_UNLOCK_SUCCESS          = 0x0147u,         /**>  子锁电子围栏内经主网关蓝牙解封成功事件 */
    HHD_EV_SUBLOCK_IN_AREA_BLE_LOCK_SUCCESS            = 0x0148u,         /**>  子锁电子围栏外经主网关蓝牙施封成功事件 */

    //密码
    HHD_EV_SUBLOCK_UNVALID_PASSWORD_UNLOCK_FAIL             = 0x0127u,         /**>  子锁非有效围栏经主网关密码解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_PASSWORD_LOCK_FAIL               = 0x0128u,         /**>  子锁非有效围栏经主网关密码施封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_PASSWORD_TIME_UNLOCK_FAIL        = 0x0129u,         /**>  子锁围栏截止日期后经主网关密码解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_PASSWORD_TIME_LOCK_FAIL          = 0x012Au,         /**>  子锁围栏截止日期后经主网关密码施封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_PASSWORD_UNLOCK_FAIL = 0x012Bu,         /**>  子锁非有效围栏时间经主网关密码解封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_PASSWORD_LOCK_FAIL   = 0x012Cu,         /**>  子锁非有效围栏时间经主网关密码施封失败事件 */
    HHD_EV_SUBLOCK_IN_AREA_PASSWORD_UNLOCK_SUCCESS          = 0x012Du,         /**>  子锁电子围栏内经主网关密码解封成功事件 */
    HHD_EV_SUBLOCK_OUT_AREA_PASSWORD_UNLOCK_FAIL            = 0x012Eu,         /**>  子锁电子围栏外经主网关密码解封失败事件 */
    HHD_EV_SUBLOCK_IN_AREA_PASSWORD_LOCK_SUCCESS            = 0x012Fu,         /**>  子锁电子围栏内经主网关密码施封成功事件 */
    HHD_EV_SUBLOCK_OUT_AREA_PASSWORD_LOCK_FAIL              = 0x0130u,         /**>  子锁电子围栏外经主网关密码施封失败事件 */

    //刷卡
    HHD_EV_SUBLOCK_UNVALID_NFC_TIME_UNLOCK_FAIL        = 0x0131u,         /**>  子锁围栏截止日期后经主网关NFC解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_NFC_TIME_LOCK_FAIL          = 0x0132u,         /**>  子锁围栏截止日期后经主网关NFC施封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_NFC_UNLOCK_FAIL = 0x0133u,         /**>  子锁非有效围栏时间经主网关NFC解封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_NFC_LOCK_FAIL   = 0x0134u,         /**>  子锁非有效围栏时间经主网关NFC施封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_NFC_LOCK_FAIL               = 0x0135u,         /**>  子锁非有效围栏卡经主网关NFC施封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_NFC_UNLOCK_FAIL             = 0x0136u,         /**>  子锁非有效围栏卡经主网关NFC解封失败事件 */
    HHD_EV_SUBLOCK_IN_AREA_NFC_UNLOCK_SUCCESS          = 0x0137u,         /**>  子锁电子围栏内经主网关NFC解封成功事件 */
    HHD_EV_SUBLOCK_IN_AREA_NFC_LOCK_SUCCESS            = 0x0138u,         /**>  子锁电子围栏内经主网关NFC施封成功事件 */
    HHD_EV_SUBLOCK_OUT_AREA_NFC_UNLOCK_FAIL            = 0x0139u,         /**>  子锁电子围栏外经主网关NFC解封失败事件 */
    HHD_EV_SUBLOCK_OUT_AREA_NFC_LOCK_FAIL              = 0x013Au,         /**>  子锁电子围栏外经主网关NFC施封失败事件 */

    //sms
    HHD_EV_SUBLOCK_UNVALID_SMS_LOCK_FAIL               = 0x013Bu,         /**>  子锁非有效围栏SMS参数经主网关SMS施封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_SMS_UNLOCK_FAIL             = 0x013Cu,         /**>  子锁非有效围栏SMS参数经主网关SMS解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_SMS_TIME_UNLOCK_FAIL        = 0x013Du,         /**>  子锁围栏截止日期后经主网关SMS解封失败事件 */
    HHD_EV_SUBLOCK_UNVALID_SMS_TIME_LOCK_FAIL          = 0x013Eu,         /**>  子锁围栏截止日期后经主网关SMS施封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_SMS_UNLOCK_FAIL = 0x013Fu,         /**>  子锁非有效围栏时间经主网关SMS解封失败事件 */
    HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_SMS_LOCK_FAIL   = 0x0140u,         /**>  子锁非有效围栏时间经主网关SMS施封失败事件 */
    HHD_EV_SUBLOCK_IN_AREA_SMS_LOCK_SUCCESS            = 0x0141u,         /**>  子锁电子围栏内经主网关SMS施封成功事件 */
    HHD_EV_SUBLOCK_IN_AREA_SMS_UNLOCK_SUCCESS          = 0x0142u,         /**>  子锁电子围栏内经主网关SMS解封成功事件 */
    HHD_EV_SUBLOCK_OUT_AREA_SMS_LOCK_FAIL              = 0x0143u,         /**>  子锁电子围栏外经主网关SMS施封失败事件 */
    HHD_EV_SUBLOCK_OUT_AREA_SMS_UNLOCK_FAIL            = 0x0144u,         /**>  子锁电子围栏外经主网关SMS解封失败事件 */
    
    //传感器报警事件
    HHD_EV_SMOKE_ALARM = 0x0200,                           /**>  烟雾报警事件 */
    HHD_EV_WATER_ALARM = 0x0201,                           /**>  水浸告警事件 */
    HHD_EV_HIGH_TEMPERATURE_ALARM = 0x0202,                /**>  温度过高告警事件 */

    //设备自身发生的事件
    HHD_EV_REMOTE_OPEN_DOOR_SUCCESS = 0x0220,              /**>  远程开箱门成功事件 */
    HHD_EV_CLOSE_DOOR = 0x0221,                            /**>  关箱门事件 */
    HHD_EV_BIND_BOX_LOCK = 0x0222,                         /**>  箱子与电子锁绑定事件（还锁事件） */
    HHD_EV_UNBIND_BOX_LOCK = 0x0223,                       /**>  箱子与电子锁解绑事件（借锁事件） */
    HHD_EV_BLUETOOTH_OPEN_DOOR_SUCCESS = 0x0224,           /**>  蓝牙开箱门成功事件 */
    HHD_EV_TRIGGER_REPORT_LATEST_STATUS = 0x0225,          /**>  触发柜子上报最新状态事件 */
    HHD_EV_DOOR_OPEN = 0x0226,                             /**>  箱门打开事件 */

    //运维事件
    HHD_EV_REMOTE_OPEN_ALL_BOXES = 0x0227,                 /**>  远程一键开所有箱子操作事件 */
    HHD_EV_BLUETOOTH_OPEN_ALL_BOXES = 0x0228,              /**>  蓝牙一键开所有箱子操作事件 */
    HHD_EV_PCB_OPEN_ALL_BOXES = 0x0229,                    /**>  PCB一键开所有箱子操作事件 */
    HHD_EV_PHYSICAL_EMERGENCY_OPEN_BOX = 0x022A,           /**>  物理应急开箱事件 */

    HHD_EV_CAPTURE_VIDEO = 0x00B0u,      /**>  远程捉拍/捉录短视频事件 */   
} hhd_event_type_t;

typedef enum hhd_event_level
{
    HHD_EV_LEVEL_NORMAL,
    HHD_EV_LEVEL_ALART,
} hhd_event_level_t;

typedef enum jt808_encrypt_type {
    JT808_ENCRYPT_NONE = 0,
    JT808_ENCRYPT_RSA,
    JT808_ENCRYPT_HHD_AES,
    JT808_ENCRYPT_INVALID = 0xffu
} jt808_encrypt_type_t;


typedef enum package_fail_op {
    FAIL_OP_DROP,           /**< 丢弃 */
    FAIL_OP_SAVE_OFFLINE    /**< 保存历史数据 */
} package_fail_op_t;

typedef enum package_qos {
    QOS_ONECE,              /**< QOS_ONECE */
    QOS_AT_LEAST_ONECE,     /**< QOS_AT_LEAST_ONECE */
    QOS_ONLY_ONECE          /**< QOS_ONLY_ONECE */
} package_qos_t;

typedef enum gps_close_reason_type {
    GPS_CLOSE_REASON_NO,                   //正在运行，没有关闭
    GPS_CLOSE_REASON_NORMAL,               //正常关闭
    GPS_CLOSE_REASON_CMD,		           //禁止使用
    GPS_CLOSE_REASON_ECO,		           //省电模式（定位成功，关闭1分钟）
	GPS_CLOSE_REASON_MOTIONLESS,           //三轴静止时间到达
    GPS_CLOSE_REASON_LOWBAT,               //低电量
} gps_close_reason_type_type_t;

typedef enum bsp_io_state_id {
    BSP_IO_STATE_UNKNOWN          = 0,/*!< BSP_IO_STATE_UNKNOWN */

    //! 主副MCU上的IO状态
    BSP_IO_STATE_GDSW,                /*!< BSP_IO_STATE_GDSW */
    BSP_IO_STATE_KEY,                 /*!< BSP_IO_STATE_KEY */
    BSP_IO_STATE_NFC,                 /*!< BSP_IO_STATE_NFC */
    BSP_IO_STATE_HRSW,                /*!< BSP_IO_STATE_HRSW */
    BSP_IO_STATE_FJSW,                /*!< BSP_IO_STATE_FJSW */ //1:无正常情况 0:正常情况 
    BSP_IO_STATE_FCSW,                /*!< BSP_IO_STATE_FCSW */
    BSP_IO_STATE_XCSW,                /*!< BSP_IO_STATE_XCSW */
    BSP_IO_STATE_GSENSOR,             /*!< BSP_IO_STATE_GSENSOR 倒立*/
    BSP_IO_STATE_CHARGING,            /*!< BSP_IO_STATE_CHARGING */
    BSP_IO_STATE_FULLY_CHARGED,       /*!< BSP_IO_STATE_FULLY_CHARGED */
    BSP_IO_STATE_LORAWAN_POWER,       /*!< BSP_IO_STATE_LORAWAN_POWER */
    BSP_IO_STATE_LORAWAN_JOINNG,
    BSP_IO_STATE_LORAWAN_JOINED,
	BSP_IO_STATE_GSENSOR_SHARK,       //运动
	BSP_IO_STATE_CHANGE,
    BSP_IO_STATE_FCSW2,    
    BSP_IO_STATE_LORA_POWER,           //LORA电源开关       0关          1开
    BSP_IO_STATE_LORA_READY,           //LORA初始化状态  0未准备好   1已经准备好        
    BSP_IO_STATE_CONNTION,             //0 长连接 1 短连接
    /*------------ 1 byte以上的数据，起始id从32开始 -----------------*/
    BSP_ADC_BATTERY = 0x20,            
	BSP_IO_STATE_LOCK,                 //主MCU施解封状态 
    BSP_IO_STATE_GPS,                  //0关闭   1有效    2无效
    BSP_IO_STATE_GPRS,                 //0没箭头，1单箭头，2双箭头
    BSP_IO_GPRS_SIGNAL,                //GPRS信号 0-6   0代表关闭
    BSP_BLE_STATE,                     //0没有蓝牙 1关闭 2打开 3连接
    BSP_IO_STATE_MAX_COUNT             /*!< BSP_IO_STATE_MAX_COUNT */
} bsp_io_state_id_t;

typedef enum {
    JT808_STATE_INIT = 0,
    JT808_STATE_DISCONNECTED,
    JT808_STATE_CONNECTED,
    JT808_STATE_WAIT_RECONNECT,
} jt808_client_state_t;

typedef enum {
    JT808_EVENT_ANY = -1,
    JT808_EVENT_ERROR = 0,          /*!< on error event, additional context: connection return code, error handle from esp_tls (if supported) */
    JT808_EVENT_CONNECTED,          /*!< connected event, additional context: session_present flag */
    JT808_EVENT_DISCONNECTED,       /*!< disconnected event */
    JT808_EVENT_DATA,               /*!< data event, additional context:
                                        - msg_id               message id
                                        - topic                pointer to the received topic
                                        - topic_len            length of the topic
                                        - data                 pointer to the received data
                                        - data_len             length of the data for this event
                                        - current_data_offset  offset of the current data for this event
                                        - total_data_len       total length of the data received
                                        - retain               retain flag of the message
                                        Note: Multiple JT808_EVENT_DATA could be fired for one message, if it is
                                        longer than internal buffer. In that case only first event contains topic
                                        pointer and length, other contain data only with current data length
                                        and current data offset updating.
                                         */
    JT808_EVENT_BEFORE_CONNECT,     /*!< The event occurs before connecting */
    JT808_EVENT_DELETED,            /*!< Notification on delete of one message from the internal outbox,
                                        if the message couldn't have been sent and acknowledged before expiring
                                        defined in OUTBOX_EXPIRED_TIMEOUT_MS.
                                        (events are not posted upon deletion of successfully acknowledged messages)
                                        - This event id is posted only if JT808_REPORT_DELETED_MESSAGES==1
                                        - Additional context: msg_id (id of the deleted message).
                                        */
} jt808_event_id_t;


typedef enum {
    JT808_ERROR_TYPE_NONE = 0,
    JT808_ERROR_TYPE_TCP_TRANSPORT,
    JT808_ERROR_TYPE_CONNECTION_REFUSED,
} jt808_error_type_t;


typedef enum {
    JT808_CONNECTION_ACCEPTED = 0,                   /*!< Connection accepted  */
    JT808_CONNECTION_REFUSE_PROTOCOL,                /*!< MQTT connection refused reason: Wrong protocol */
    JT808_CONNECTION_REFUSE_ID_REJECTED,             /*!< MQTT connection refused reason: ID rejected */
    JT808_CONNECTION_REFUSE_SERVER_UNAVAILABLE,      /*!< MQTT connection refused reason: Server unavailable */
    JT808_CONNECTION_REFUSE_BAD_USERNAME,            /*!< MQTT connection refused reason: Wrong user */
    JT808_CONNECTION_REFUSE_NOT_AUTHORIZED           /*!< MQTT connection refused reason: Wrong username or password */
} jt808_connect_return_code_t;




/**************************************
 * 结构体
 * 
 * 
 * 
 * 
 * 
 **************************************/

typedef struct jt808_msg_item_define {
	short id;
	const char *name;
	jt808_dtype_t target_type;	// 打包数据类型
    char id_len=0;  // 附加数据时ID占用字节个数，不需要时为0
	char len_len=0; // 附加数据时数据长度占用字节数，不需要时为0

	char fix_length =0;		// 打包固定长度
	char align_right =0;	// 是否右对齐（如终端电话号码）
	char padding_byte=0;	// 固定长度需要补齐时的填补数据，如终端电话号码前面补0


	char sub_msg_len_size =0;	// 如果有子包的情况，子包总长度占用的字节数
	char sub_msg_count = 0;		// 子包数据项个数
	struct jt808_msg_define *sub_msg_define = NULL;
} jt808_msg_item_define_t;

typedef struct jt808_msg_define {
	short msg_id;						// 消息ID
	const char *name;
    char field_count;					// 字段个数（消息定义的固定字段个数）
	jt808_msg_item_define_t *fields = NULL;		// 字段定义
    char exfield_count;					// 附加字段个数
	jt808_msg_item_define_t *exfields= NULL;		// 附加字段定义
} jt808_msg_define_t;

typedef struct package {
    uint16_t    package_type;       /**< 报文类型，与业务对应 */
    uint16_t    sn;                 /**< 消息流水号 */
    uint8_t     qos;                /**< QOS 枚举类型：package_qos_t */
    uint8_t     fail_op;            /**< 失败后的操作 枚举类型：package_fail_op_t*/
    uint8_t     max_retry;          /**< 最大重试次数 */
    uint8_t     encrypt;            /**< 加密方式  参考jt808_encrypt_type_t*/
    uint8_t     is_0200_event;		/**< 是否为事件报文*/
    uint8_t     is_business_data;
    uint8_t     is_upgrade_versions;
    uint16_t    buffer_size;          /**< 缓存空间大小 */
    uint16_t    data_len;           /**< 报文长度 */
    uint8_t     data[0];            /**< 报文内容(方便使用整块内存进行管理) */
} package_t;

#define SYS_IPC_MESSAGE_BUFF_LEN    128         // 单个IPC消息数据部分的长度
typedef struct sys_ipc_message {
    uint8_t from;                               // 发送方sys_task_id_t
    uint8_t to;                                 // 接收方sys_task_id_t

    uint8_t sn;                        // 请求流水号
    uint8_t  req_type;           // 取值参考sys_ipc_request_type_t 请求类型，对应不同的业务类型
    uint8_t  is_resp;                           // 是否为应答消息、为1时表示为应答报文

    uint8_t  pack_id;                           // 包序号从0开始，用于超过长度的数据分包
    uint16_t total_data_len;                    // 数据总长度
    uint16_t data_len;                          // 当前报文数据长度
    uint8_t  data[SYS_IPC_MESSAGE_BUFF_LEN];    // 数据
}sys_ipc_message_t;

#define SYS_IPC_MESSAGE_MAX_SIZE    (sizeof(sys_ipc_message_t) - SYS_IPC_MESSAGE_BUFF_LEN + 1536 )
typedef struct hhd_event_log
{
    uint32_t ts;       // 时间ms
    uint64_t tick;     // 任务时钟
    uint8_t event_flag;     // 标志： 0 位置上报 1产生事件
    hhd_event_type_t event; // 事件
    hhd_event_level_t level;
    hhd_event_level_t video_level;
    uint8_t sublock_flag;  // 标志： 0 正常报文 1子锁产生事件
    uint8_t sublock_id[6]; // 子锁id
    uint8_t box;
    union
    {
        char nfc[16];
        char password[16];
        char ble_user[81];
        char plat_user[81];
        char ble_mac[6];
        char telno[16];
        uint8_t nodata[1];
    }; // 附加数据
} hhd_event_log_t;

typedef struct jt808_message {
    char *data;
    int length;
} jt808_message_t;


typedef struct jt808_msg_config {
    char version;			//协议版本号
    char auth_code_len;			//鉴权码长度
    char auth_code[32];			//鉴权码
    jt808_encrypt_type_t encrypt_type;			//加密方式
    char imei[16];				//终端IMEI号
    char term_no[21];				//终端设备编号
    char firm_version[32];			//终端固件版本号
	void *aes_key;				//AES加密密钥
	void *rsa_key;				//RSA加密密钥
} jt808_msg_config_t;

typedef struct jt808_connection {
	const jt808_msg_config_t *config;			//用于存储与消息配置相关的信息
    jt808_message_t message;			//用于存储当前连接中的消息内容
    uint16_t last_message_id;			//用于存储上一个消息的ID
    char *buffer;				//用于存储接收或发送消息的缓冲区
    int buffer_length;			//存储缓冲区的长度
} jt808_connection_t;


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

typedef struct jt808_msg_fields_data {
    jt808_encrypt_type_t  encrypt_type;			//加密类型，枚举
    char field_values_count;			//字段个数
    char exfield_values_count;			//扩展字段个数

    jt808_msg_value_t * field_values;			//字段值
    jt808_msg_value_t * exfield_values;			//扩展字段值
} jt808_msg_fields_data_t;

typedef uint64_t hhd_os_tick_t;

typedef struct {
    uint8_t num;                                /*!< Satellite number */
    uint8_t elevation;                          /*!< Elevation value */
    uint16_t azimuth;                           /*!< Azimuth in degrees */
    uint8_t snr;                                /*!< Signal-to-noise ratio */
} sats_in_view_desc_t;

typedef struct {
    uint8_t date;                               /*!< Fix date */
    uint8_t month;                              /*!< Fix month */
    uint8_t year;                               /*!< Fix year */
    uint8_t hours;                              /*!< Hours in UTC */
    uint8_t minutes;                            /*!< Minutes in UTC */
    uint8_t seconds;                            /*!< Seconds in UTC */
} gps_time_t;

typedef struct state_gnss {
	uint8_t  cmd;          		/*!< GPS是否禁用 */
	uint8_t  agps_cmd;			/*!< 表示AGPS正在获取数据，禁止关闭gps */
	uint8_t  task_run;          /*!< 任务是否在运行 */
    gps_close_reason_type_type_t  task_run_close_reason;       /*!< 任务关闭原因 具体查看*/
	uint8_t  eco;          		/*!< 省电模式 */
    uint8_t  is_valid;          /*!< 是否有效 */
    uint8_t  current_is_valid;  /*!< 当前是否有效 */
    hhd_os_tick_t effective_valid_last_tick; /*!< 最后一次有效定位的时间 */
    uint8_t	 antenna_status;	/*!< 天线状态 0正常  1开路 2短路*/
    float    latitude;          /*!< 纬度 */
    float    longitude;         /*!< 经度 */
    float    altitude;          /*!< 海拔 */
    float    speed;             /*!< 速度 */
    float    course;            /*!< 方向 */
    float	 pdop;				/*!< 位置精度因子*/
    float	 set_pdop;			/*!< 设置位置精度因子精度*/
    float    current_latitude;          /*!< 当前纬度 */
    float    current_longitude;         /*!< 当前经度 */
    float    current_altitude;          /*!< 当前海拔 */
    float    current_speed;             /*!< 当前速度 */
    float    current_course;            /*!< 当前方向 */
    float	 current_pdop;				/*!< 当前位置精度因子*/
    uint8_t  sats_in_use;       /*!< 使用卫星数 */
    uint8_t	 fix_mode;			/*!< 定位状态： 1定位无效 2 2D定位 3 3D定位*/
    uint8_t	 set_fix_mode;			/*!< GNSS定位有效质量设置*/
    uint8_t	 set_effective_counter;	/*!< 设置需达标的有效质量次数 */
    uint8_t	 effective_counter;	/*!< 当前有效质量次数 */
    uint8_t	 send_flag;			/*!< 本次数据是否可以发送 0不满足 1满足*/
    hhd_os_tick_t effective_last_sync_at;/*!< 最后一次有效数据同步时间（有效定位满足条件） */
    hhd_os_tick_t last_sync_at;      /*!< 最后同步时间(满足条件达标有效定位) */
    hhd_os_tick_t current_last_sync_at;      /*!< 最后同步时间(gps输出有效定位) */
    hhd_os_tick_t task_run_start_timer;	/*!< 任务开启的运行时间 */
    hhd_os_tick_t task_run_last_timer;	/*!< 任务最后的运行时间 */
    hhd_os_tick_t agps_send_last_timer;  /*!< AGPS最后发送数据的时间 */
    uint8_t	 manufacturer;//gps模块厂商	0x01: zkw------中科微	0x02: ublox------U-BLOX
    uint8_t gp_sats_in_view;//gps可见行星数量
    uint8_t bd_sats_in_view;//北斗可见行星数量
    gps_time_t time;
    uint32_t gsensor_motionless_time = 180;//静止状态时间，（单位秒）默认180秒
    hhd_os_tick_t gps_save_tick;
    sats_in_view_desc_t gp_sats_in_view_desc[24];//gps可见行星列表
    sats_in_view_desc_t bd_sats_in_view_desc[24];//北斗可见行星列表
} state_gnss_t;

typedef struct sensor_data
{
    int16_t temperature;            // 温度，单位为0.1摄氏度
    uint32_t Gravity;               // 重力传感器，单位为0.1g
    uint8_t temperature_alarm_flag; // 报警标志，0x01表示报警，0x00表示正常
    uint8_t smong_alarm_flag;       // 报警标志，0x01表示报警，0x00表示正常
    uint8_t water_alarm_flag;       // 报警标志，0x01表示报警，0x00表示正常
} Sensor_data;


typedef struct
{
    uint8_t channel;          // 通道号
    uint8_t bluetooth_mac[7]; // 电子锁
    uint8_t wifi_mac[7];      // wifi
    uint8_t user_info[30];    // 用户信息
    uint8_t box_faut;         // 箱子故障
    uint16_t box_charge;      // 箱子的充满电状态
} hhd_box_data_t;


typedef struct
{
    int key_len;
    char E[4];
    char N[128];
    char D[128];
    char P[64];
    char Q[64];
    char DP[64];
    char DQ[64];
    char QP[64];
}hhd_rsa_context_t;


typedef enum warn_check_state {
    CHECK_RESULT_UNKNOWN,
    CHECK_RESULT_PASS,
    CHECK_RESULT_NO_PASS
} warn_check_state_t;


typedef warn_check_state_t (* sys_warn_check_function) (void);
typedef struct jt808_client *jt808_client_handle_t;

struct sys_warn_to_808_config {
    int bit;
    sys_warn_check_function check_fun;
};

typedef struct {
    int event_handle;
    // esp_event_loop_handle_t event_loop_handle;
    int task_stack;
    int task_prio;
    jt808_msg_config_t jt808_config;
    char host[50];
    char scheme[16];
    short port;
    bool auto_reconnect;
    void *user_context;
    int network_timeout_ms;
    int refresh_connection_after_ms;
    int reconnect_timeout_ms;
    int message_retransmit_timeout;
} jt808_config_storage_t;

typedef struct jt808_connect_info {
    char client_id[32];
    char *username;
    char *password;
    int clean_session;
    long keepalive;
} jt808_connect_info_t;


typedef struct jt808_state {
    jt808_connect_info_t *connect_info;         //连接信息
    char *in_buffer;                            //输入缓冲区
    char *out_buffer;                           //输出缓冲区
    int in_buffer_length;                       //输入缓冲区长度   
    int out_buffer_length;                      //输出缓冲区长度
    long long message_length;                   //消息长度
    long long in_buffer_read_len;               //从接收缓冲区读取的数据长度
    jt808_message_t *outbound_message;          //待发送消息
    jt808_connection_t jt808_connection;        //连接状态
    short pending_msg_id;                       //待发送消息id
    short pending_msg_type;                     //待发送消息类型
    int pending_publish_qos;                    //待发送消息qos
    int pending_msg_count;                      //待发送消息数量        
} jt808_state_t;

typedef struct jt808_error_codes {
    /* compatible portion of the struct corresponding to struct hhd_tls_last_error */
    int hhd_tls_last_hhd_err;              /*!< last hhd_err code reported from esp-tls component */
    /* esp-mqtt specific structure extension */
    jt808_error_type_t error_type;            /*!< error type referring to the source of the error */
    jt808_connect_return_code_t connect_return_code; /*!< connection refused error code reported from MQTT broker on connection */
    /* tcp_transport extension */
    int       hhd_transport_sock_errno;         /*!< errno from the underlying socket */

} jt808_error_codes_t;


typedef struct {
    jt808_event_id_t event_id;       /*!< MQTT event type */
    jt808_client_handle_t client;    /*!< MQTT client handle for this event */
    void *user_context;                 /*!< User context passed from MQTT client config */
    const jt808_message_t * message;
    jt808_msg_fields_data_t * feilds;
    WORD msgid;              /*!< MQTT messaged id of message */
    WORD msgsn;              /*!< MQTT messaged id of message */
    int session_present;                /*!< MQTT session_present flag for connection event */
    jt808_error_codes_t *error_handle; /*!< esp-mqtt error handle including esp-tls errors as well as internal mqtt errors */
    bool retain;                        /*!< Retained flag of the message associated with this event */
} jt808_event_t;

typedef struct jt808_queue_list_t  * jt808_queue_handle_t;

struct jt808_client {
    int socket_up;
    int socket_down;
    jt808_config_storage_t *config;
    jt808_state_t  jt808_state;
    jt808_connect_info_t connect_info;
    jt808_client_state_t state;
    long long refresh_connection_tick; //连接的时间
    long long keepalive_tick;
    long long reconnect_tick;
    int wait_timeout_ms;
    int auto_reconnect;
    jt808_event_t event;
    bool run;
    bool wait_for_ping_resp;
    jt808_queue_handle_t out_queue;
};

typedef struct jt808_0313_param_query_data {
    uint16_t  package_type;       /**< 原报文类型 */
    uint16_t  request_sn;         /**< 原报文流水号 */
    uint16_t  count;              /**< 参数个数 */
    uint16_t * param_ids;          /**< 参数ID列表 */
} jt808_0313_param_query_data_t;

class Jt808 {
public:
    Jt808();
    ~Jt808();
    int jt808_init();
    void SetIsSend(bool send);

private:
    int process_upload_location(sys_ipc_message_t * msg); //上报808数据
    int jt808_process_receive(jt808_client_handle_t client);//接收下方的808数据
    int jt808_connect(jt808_client_handle_t client, int flag);
    void func_sendthread();
    void func_recvthread();

    package_t * package_jt808_create(uint16_t buffer_size);

    uint32_t sys_states_get_808_states_attribute(void);

    uint8_t jt808_msg_get_ver(const uint8_t *data, int len);
    WORD jt808_msg_get_msgid(const uint8_t *data, int len);
    WORD jt808_msg_get_msgsn(const uint8_t *data, int len);
    int jt808_value_set_dword(jt808_msg_value_t *value, uint8_t id, DWORD v);
    int jt808_value_set_word(jt808_msg_value_t *value, uint8_t id, WORD v);
    int jt808_value_set_bytes(jt808_msg_value_t *value, uint8_t id, BYTE *v, int len);
    const char *hhd_os_get_time_yymmddhhmmss_from_ts(char *datetime, int time_zone, hhd_os_time_t timestamp);
    const char *hhd_os_get_time_yymmddhhmmss(char *datetime, int time_zone);
    const jt808_msg_config_t * package_get_jt808_config(jt808_send_channel_type_t send_channel, jt808_encrypt_type_t encrypt_type);
    void jt808_msg_init(jt808_connection_t *connection, const jt808_msg_config_t *config, uint8_t *buffer, size_t buffer_length);
    int jt808_value_set_string(jt808_msg_value_t *value, uint8_t id, const char * v);
    int jt808_value_set_byte(jt808_msg_value_t *value, uint8_t id, BYTE v);
    int bsp_io_state_get_state(bsp_io_state_id_t state_id);
    int buffer_append_byte(uint8_t *buffer, uint8_t v);
    int buffer_append_dword(uint8_t *buffer, uint32_t v);
    int jt808_append_dword(jt808_connection_t *connection, DWORD v);
    int jt808_append_bytes(jt808_connection_t *connection, const BYTE *v, uint16_t len);
    int jt808_msg_append_data_word_arr_with_count(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value);

    int msg_value_get_dword(const jt808_msg_value_t *value, DWORD *word);
    int jt808_msg_append_data(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value,int judge);
    int jt808_msg_append_data_bytes(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value);
    int jt808_msg_append_data_bcd(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value);
    
    jt808_msg_value_t * get_msg_value_by_id(jt808_msg_value_t *values, int value_count, uint16_t field_id);

    char * msg_value_get_string(const jt808_msg_value_t *value, char *data, int len);
    int jt808_msg_append_data(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value);
    int jt808_append_byte_repeat(jt808_connection_t *connection, BYTE v, int count);
    int jt808_msg_append_checkcode(jt808_connection_t *connection);
    jt808_message_t * jt808_msg_final(jt808_connection_t *connection);
    WORD jt808_msg_append_msgid(jt808_connection_t *connection, short message_id);
    int jt808_append_bcd(jt808_connection_t *connection, const char *str, int len, bool isAlignRight, int bcdSize);
    int append_byte(jt808_connection_t *conn, char v);
    int jt808_append_word(jt808_connection_t *connection, WORD v);
    int jt808_append_msgbody_attr(jt808_connection_t *connection, char verFlag, bool fragmented, jt808_encrypt_type_t encryptType, DWORD len);
    int init_message(jt808_connection_t *connection, WORD msg_id, WORD * out_msg_sn);

    jt808_msg_item_define_t * get_msg_item_define_by_id(jt808_msg_item_define_t *defines, int count, uint16_t field_id);
    int jt808_msg_append_extra_data(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value);
    int jt808_append_byte(jt808_connection_t *connection, BYTE v);



    int jt808_pack_message_fix_fields(jt808_connection_t *connection, const jt808_msg_define_t *msg_def, jt808_msg_value_t *values, int value_count);
    int jt808_pack_message_ext_fields(jt808_connection_t *connection, const jt808_msg_define_t *msg_def, jt808_msg_value_t *values, int value_count);
    jt808_message_t * jt808_pack_message(jt808_connection_t *connection, const jt808_msg_define_t *msg_def, jt808_msg_fields_data_t *data, WORD * out_msg_sn);
    jt808_msg_define_t *jt808_get_msg_define(uint16_t msg_id);


    int pack_common_build_state_bits(DWORD * sate_bits);
    int hhd_event_get_ext_info(const hhd_event_log_t *evlog, uint8_t *data, uint16_t max_size);

    int pack_jt808_0200(package_t *package, uint8_t *out_data, uint16_t out_data_size, hhd_event_log_t *event, jt808_send_channel_type_t send_channel, jt808_encrypt_type_t encrypt_type);
    int pack_common_build_warn_bits(DWORD * warn_bits);
    
    
    
    // - 0002 心跳报文定义
    static jt808_msg_define_t jt808_msg_term_hreartbreat;
    // - 0200 固定字段
    static jt808_msg_item_define_t jt808_msg_term_location_fix_fields[];
    // - 0200 扩展字段
    static jt808_msg_item_define_t jt808_msg_term_location_ext_fields[];
    // - 0200 报文定义
    static jt808_msg_define_t jt808_msg_term_location;
    static jt808_msg_config_t  g_jt808_msg_config;
    static uint8_t  g_jt808_msg_inited;
    state_gnss_t g_state_gps_info;
    Sensor_data g_sensor_data;


    hhd_box_data_t g_box_list[10] = {0};
    uint8_t g_box_number = 10; // HHD_SUBLOCK_BOX_NUM_MAX;
    int g_state_time_zone = 0;
    uint8_t g_connection_long_short = 0;// 0:长连接;1短连接
    unsigned char SealingState = 0x30;	//施封状态    0x30: 解封状态    0x31: 施封状态
    unsigned char LOCK_POLE_STATUS  = 0xff;      //锁杆最终状态  2:锁杆被剪断 1:锁杆正常插入   0:锁杆正常打开
    unsigned char charge_status = 0xff;	//0: 未充电   1: 充电中 未充满电   2: 充电中 已充满电
    unsigned char g_status_rest  = 0;				  // 运动传感器状态-----0：运动状态; 1:静止状态
    uint8_t g_sim_card_id        = 1;           // 0:缺省;  01:当前sim卡1;  10:当前sim卡2
    uint32_t  g_io_state = 0u;
    std::mutex mutex; // 互斥锁

    jt808_client_handle_t client;
    sys_ipc_message_t *msg_recved;
    uint8_t g_process_buffer[SYS_IPC_MESSAGE_MAX_SIZE];

    std::thread send_thread; //上报报文线程
    std::thread recv_thread; //接收报文线程

    bool IsSend =NULL;

    /*!
    * @brief 检测是否触发防剪报警
    *
    * @return
    */
    static  warn_check_state_t sys_warn_check_suogan_fangjian(void);


    /*!
    * @brief   检测是否触发放拆报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_waike_fangchai(void);


    /*!
    * @brief   检测是否触发低电量报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_low_battery(void);


    /*!
    * @brief   检测是否触发主副MCU通讯报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_mcucom(void);


    /*!
    * @brief   检测是否触发锁杆/锁绳子内部导线报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_suogan_daoxian(void);


    /*!
    * @brief   检测是否触发天线短路报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_tixian_short_circuit(void);


    /*!
    * @brief   检测是否触发天线断路报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_tixian_broken_circuit(void);


    /*!
    * @brief   检测是否触发电机施封卡死报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_motor_shifeng(void);

    /*!
    * @brief   检测是否触发电机解封卡死报警
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_motor_jiefeng(void);


    /*!
    * @brief   检测是否触发终端主电源掉电
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_main_power_lost(void);


    /*!
    * @brief   检测是否超速
    *
    * @return
    */
    static warn_check_state_t sys_warn_check_speed();


    static warn_check_state_t sys_warn_check_chaoshi();

    /*!
    * @brief   返回符合808报文定义的终端报警标志位
    *
    * @return
    */
    uint32_t sys_warn_get_808_warn_attribute(void);


    static uint16_t g_car_speed_warning_num;//设置报警， 单位： KM（ 0： 表示不启用本功能）
    static  bool g_sate_last_time;
    

    const struct sys_warn_to_808_config g_warn_to_808_config[12] ={
        { .bit = HLINK_WARN_SUOGAN          ,   .check_fun = sys_warn_check_suogan_fangjian},
        { .bit = HLINK_WARN_WAIKE           ,   .check_fun = sys_warn_check_waike_fangchai },
        { .bit = HLINK_WARN_ABT_LOW         ,   .check_fun = sys_warn_check_low_battery },
        { .bit = HLINK_WARN_MCU_COMM        ,   .check_fun = sys_warn_check_mcucom },
        { .bit = HLINK_WARN_SUOGAN_DAOXIAN  ,   .check_fun = sys_warn_check_suogan_daoxian },
        { .bit = HLINK_WARN_GPS_TIANXIAN    ,   .check_fun = sys_warn_check_tixian_short_circuit },
        { .bit = HLINK_WARN_GPS_TIANXIAN_DUANLU,.check_fun = sys_warn_check_tixian_broken_circuit },
        { .bit = HLINK_WARN_DIANJI_LOCK     ,   .check_fun = sys_warn_check_motor_shifeng },
        { .bit = HLINK_WARN_DIANJI_UNLOCK   ,   .check_fun = sys_warn_check_motor_jiefeng },
        { .bit = HLINK_WARN_SPEED           ,   .check_fun = sys_warn_check_speed },
        { .bit = HLINK_WARN_PARKING_TIMEOUT,  .check_fun = sys_warn_check_chaoshi },
        { .bit = HLINK_WARN_POWER           ,   .check_fun = sys_warn_check_main_power_lost }
};
;

};
#endif