#include "jt808.h"
#include "config.h"
#include <arpa/inet.h>
#define JT808_HEAD_ATTR_H_VER_FLG               (1 << 6)                // 版本>标识
#define JT808_HEAD_ATTR_H_FRAGMENT              (1 << 5)                // 分包>标识
#define JT808_TYPE_HLINK    1   // 消息类型配置，航鸿达消息类型
#define JT808_MSG_FLAG          (char) 0x7E
#define JT808_MSG_ESCAPE_FLAG   (char) 0x7D
#define JT808_MSG_ESCAPE_7E_L   (char) 0x02
#define JT808_MSG_ESCAPE_7D_L   (char) 0x01

#if JT808_TYPE_HLINK

#define JT808_MESSAGE_ID_SIZE	2
#define JT808_MESSAGE_BODY_SIZE	2
#define JT808_MESSAGE_ENCAULATION_SIZE	4
#define JT808_MAX_FIXED_HEADER_SIZE 12    // 不算“消息包封装项”

#else

#define JT808_MAX_FIXED_HEADER_SIZE 17    // 不算“消息包封装项”

#endif

#define JT808_MSG_PLAT_COMM_RESP                  0x8001  // 平台通用应答
#define JT808_MSG_TERM_HHD_DATA_REQ_RESP          0x8400  // 终端数据请求应答
#define JT808_MSG_TERM_RSA_PUBLIC_KEY_RESP        0x0611  // 终端上报RSA公钥应答

// - 0002 心跳报文定义
jt808_msg_define_t Jt808::jt808_msg_term_hreartbreat =
{
    .msg_id = JT808_MSG_TERM_HEART_BEAT,
    .name = "Terminal Heartbeat",
    .field_count = 0,
    .fields = 0,
    .exfield_count = 0,
    .exfields = 0
};

// - 0200 固定字段
jt808_msg_item_define_t Jt808::jt808_msg_term_location_fix_fields[] = {
    {.id = FIELD_0200_WARN_FLG, .name = "Warning flag", .target_type = JT808_DTYPE_DWORD},
    {.id = FIELD_0200_STAT, .name = "Status", .target_type = JT808_DTYPE_DWORD},
    {.id = FIELD_0200_LATITUDE, .name = "Latitude", .target_type = JT808_DTYPE_DWORD},
    {.id = FIELD_0200_LONGITUDE, .name = "Longitude", .target_type = JT808_DTYPE_DWORD},
    {.id = FIELD_0200_ALTITUDE, .name = "Altitude", .target_type = JT808_DTYPE_WORD},
    {.id = FIELD_0200_SPEED, .name = "Speed", .target_type = JT808_DTYPE_WORD},
    {.id = FIELD_0200_DIRECT, .name = "Direction", .target_type = JT808_DTYPE_WORD},
    {.id = FIELD_0200_TIME, .name = "Time", .target_type = JT808_DTYPE_BCD, .fix_length = 6, .padding_byte = 0x00},
};


// - 0200 扩展字段
jt808_msg_item_define_t Jt808::jt808_msg_term_location_ext_fields[] = {
    {.id = EXT_FIELD_0200_MILEAGE, .name = "Mileage", .target_type = JT808_DTYPE_DWORD, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_FUEL_LEFT, .name = "Fuel left", .target_type = JT808_DTYPE_WORD, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_TANK_FUEL_CAP, .name = "Fuel capacity of tank", .target_type = JT808_DTYPE_WORD, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_LOAD_SENSOR, .name = "Load sensor", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_WIFI_MAC, .name = "WIFI MAC data", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_BLE_MAC, .name = "BLE MAC data", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_EVENT, .name = "Event data", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_SUB_LOCK_STAT, .name = "Sub lock status", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_LORA, .name = "LORA data", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_LBS, .name = "LBS", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_BASE_STATION, .name = "Base station", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_DYNAMIC_PASS, .name = "Dynamic password", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1, .fix_length = 8},
    {.id = EXT_FIELD_0200_BATTERY, .name = "Battery capacity", .target_type = JT808_DTYPE_DWORD, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_VOLTAGE, .name = "Battery voltage", .target_type = JT808_DTYPE_WORD, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_CSQ, .name = "CSQ", .target_type = JT808_DTYPE_BYTE, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_GPS_STAR, .name = "GPS stars", .target_type = JT808_DTYPE_BYTE, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_SIM_IMSI, .name = "IMSI", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_CLIENT_ID, .name = "Client ID", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_OTA_RESULT, .name = "OTA result", .target_type = JT808_DTYPE_STRING, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_SIM_ICCID, .name = "ICCID", .target_type = JT808_DTYPE_STRING, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_BUSINESS_DATA, .name = "Business Data", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_DATA_LORA, .name = "lora Data", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_REPORTED_TIME, .name = "REPORTED_TIME", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_REPORTED_VIDEO, .name = "REPORTED_video", .target_type = JT808_DTYPE_STRING, .id_len = 1, .len_len = 1},

    {.id = EXT_FIELD_0200_TEMPERATURE_SENSOR, .name = "TEMPERATURE_SENSOR", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_SMOKE_SENSOR, .name = "SMOKE_SENSOR", .target_type = JT808_DTYPE_BYTE, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_WATER_SENSOR, .name = "WATER_SENSOR", .target_type = JT808_DTYPE_BYTE, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_LOCK_BLUETOOTH, .name = "LOCK_BLUETOOTH", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
    {.id = EXT_FIELD_0200_LOCK_STATUS_INFO, .name = "LOCK_STATUS_INFO", .target_type = JT808_DTYPE_BYTE_ARR, .id_len = 1, .len_len = 1},
};


// - 0200 报文定义
jt808_msg_define_t Jt808::jt808_msg_term_location =
{
    .msg_id = JT808_MSG_TERM_LOCATION,
    .name = "Upload location",
    .field_count = sizeof(jt808_msg_term_location_fix_fields) / sizeof(*jt808_msg_term_location_fix_fields),
    .fields = jt808_msg_term_location_fix_fields,
    .exfield_count = sizeof(jt808_msg_term_location_ext_fields) / sizeof(*jt808_msg_term_location_ext_fields),
    .exfields = jt808_msg_term_location_ext_fields
};

jt808_msg_config_t  Jt808::g_jt808_msg_config;
uint8_t  Jt808::g_jt808_msg_inited = 0;

uint16_t Jt808::g_car_speed_warning_num = 0;
bool Jt808::g_sate_last_time = false;

Jt808::Jt808()
{
}

Jt808::~Jt808()
{
    send_thread.join();
    recv_thread.join();
    free(client->jt808_state.in_buffer);
    free(client->config);
    free(client);
}

int Jt808::process_upload_location(sys_ipc_message_t *msg)
{
    int err = 0;
    package_t * package = NULL;
    hhd_event_log_t * evlog = NULL;
    int         data_len;
    uint8_t * p_data = NULL;

    printf("-- process_upload_location --\n");
    if (msg->req_type == IPC_REQ_TYPE_EVENT_LOG)
    {
        printf("--- EVENT ----\n");
        if (msg->data_len != sizeof(hhd_event_log_t))
        {
            printf("%s:%d Invalid Event Arguments.\n", __FILE__, __LINE__);
            return -1;
        }
		evlog = (hhd_event_log_t *) msg->data;
    }

     // 创建包
    package = package_jt808_create(PACKAGE_MAX_DATA_LENGTH);
    if (package == NULL)
    {
        printf("%s:NO MEMORY!!!!!!\n", __func__ );
        return -1;
    }

    if(evlog != NULL)
    {
        printf("evlog no NULL!!!!!!\n");
    }

    // 打包报文
    printf("package->buffer:%d,package->data_len:%d\n",package->buffer_size,package->data_len);
    jt808_send_channel_type_t send_channel = JT808_SEMD_CHANNEL_PLATFORM;
    printf("> pack_jt808_0200%d\n",msg->from);
    if(msg->from == TASK_BLE){
		send_channel = JT808_SEMD_CHANNEL_BLE;
	}
	else{
		send_channel = JT808_SEMD_CHANNEL_PLATFORM;
	}
    data_len = pack_jt808_0200(package, package->data, package->buffer_size, evlog, send_channel, JT808_ENCRYPT_NONE);
    if (data_len <= 0)
    {
        printf("Fail pack_jt808_0200, RC:%d\n", data_len );
        free(package);
    }
    printf("pack_jt808_0200 SUCCESS\n");
    for (int i = 0; i < package->data_len; i++)
    {
        printf("%02X ", package->data[i]);
        if ((i + 1) % 16 == 0)
        { // 每行打印16个字节
            printf("\n");
        }
    }
    printf("\n");
    //Config::Show(package->data,"上报的808数据");
    package->data_len = data_len;

    uint8_t blind_flag = 0;
    if(1)
    {
        // 发送报文
        send(client->socket_up, package->data, package->data_len, 0);
        printf("send data success\n");
    }
    else
    {
        blind_flag = 1;
    }

    return data_len;
}

int Jt808::jt808_process_receive(jt808_client_handle_t client)
{
    uint8_t msg_ver;
    uint16_t param_ids[256];
    int len;
    WORD msg_id;
    WORD msg_sn;
    jt808_msg_fields_data_t *data = NULL;
    package_t *package = NULL;
    jt808_send_channel_type_t send_channel = JT808_SEMD_CHANNEL_PLATFORM;
    jt808_0313_param_query_data_t query_data;

    uint8_t tbuffer[200] = {0};
    int ret = recv(client->socket_up, tbuffer, 200, 0);
    if (ret <= 0)
    {
        //printf("recv error, ret = %d\n",ret);
        return -1;
    }
    // for (int i = 0; i < 200; i++)
    // {
    //     printf("%02x ", tbuffer[i]);
    // }
    //printf("\n");
    Config::Show(tbuffer,"接收的808数响应数据");

    msg_ver = jt808_msg_get_ver(tbuffer, 20);
    msg_id = jt808_msg_get_msgid(tbuffer, 20);
    msg_sn = jt808_msg_get_msgsn(tbuffer, 20);

    printf("-------- PACK[0x%04X] DATA -------\n", msg_id);

    client->event.event_id = JT808_EVENT_DATA;
    client->event.feilds = NULL;
    client->event.msgid = msg_id;
    client->event.msgsn = msg_sn;

    Config::isCar =true;
    switch (msg_id)
    {
        case JT808_MSG_PLAT_COMM_RESP: // 0x8001通用应答
            printf("SERVER ANSWER:JT808_MSG_PLAT_COMM_RESP\n");
            break;
        case JT808_MSG_TERM_HHD_DATA_REQ_RESP: // 0x8400响应应答
            printf("SERVER ANSWER:JT808_MSG_TERM_HHD_DATA_REQ_RESP\n");
            break;
        case JT808_MSG_TERM_RSA_PUBLIC_KEY_RESP:                                                    // 0x0611 终端上传RSA公钥响应
            printf("SERVER ANSWER:JT808_MSG_TERM_RSA_PUBLIC_KEY_RESP\n");
            break;
        // case JT808_MSG_TERM_HHD_QUERY_PARAM:
        //     package = package_jt808_create(PACKAGE_MAX_DATA_LENGTH);
        //     if (package == NULL)
        //     {
        //         printf("%s:NO MEMORY!!!!!!\n", __func__ );
        //         return -1;
        //     }
        //     memset(&query_data, 0, sizeof(query_data));
        //     query_data.package_type = msg_id;
        //     query_data.count        = 47;
        //     query_data.param_ids    = param_ids;
        //     query_data.request_sn   = msg_sn;
        //     len = pack_jt808_0313(package, &query_data, package->data, package->buffer_size, send_channel, JT808_ENCRYPT_NONE); // 0x0313 查询终端参数
        //     if (len <= 0)
        //     {
        //         printf("Fail pack_jt808_0313, RC:%d\n", len );
        //         free(package);
        //     }
        //     printf("answer check\n");
        //     for (int i = 0; i < package->data_len; i++)
        //     {
        //         printf("%02x ", package->data[i]);
        //     }
        //     printf("\n");
        //     // 发送应答
        //     ipcmsg_request_send_jt808_package(0,package);
        //     printf("SERVER ANSWER:JT808_MSG_TERM_HHD_QUERY_PARAM\n");
        default: // 发给proccess任务去处理
            // ipcmsg_recved_jt808_request(TASK_JT808, client->jt808_state.in_buffer, client->jt808_state.in_buffer_read_len);
            break;
    }

    printf("Process message msg_ver: %d msg_id=0x%04X, msg_sn=0x%04x\n", msg_ver, msg_id, msg_sn);
    client->jt808_state.in_buffer_read_len = 0;
    memset(tbuffer, 0, 200);
    
    if (client->event.feilds)
    {
        if (client->event.feilds->exfield_values)
            free(client->event.feilds->exfield_values);
        if (client->event.feilds->field_values)
            free(client->event.feilds->field_values);
        free(client->event.feilds);
    }

    if (data)
    {
        if (data->field_values)
            free(data->field_values);
        if (data->exfield_values)
            free(data->exfield_values);
        free(data);
    }
    free(package);
    return 0;
}

int Jt808::jt808_init()
{
   
    client = static_cast<jt808_client_handle_t>(calloc(1, sizeof(jt808_client_handle_t)));
    if (client == NULL){
        printf("client calloc error\n");
        return 0;
    }
    client->config = static_cast<jt808_config_storage_t*>(calloc(1, sizeof(jt808_config_storage_t)));
    if (client->config == NULL){
        printf("client->config calloc error\n");
        return 0;
    }
    
    client->socket_up = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_up < 0){
        printf("socket_up error\n");
        return 0;
    }
    client->socket_down = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_down < 0){
        printf("socket_down error\n");
        return 0;
    }
    strcpy(client->config->host, Config::Get("Platform","IP_808").c_str());
    client->config->port = atoi(Config::Get("Platform","Prot_808").c_str());

    msg_recved = (sys_ipc_message_t*) g_process_buffer;
    msg_recved->sn = 0x00a7;
    msg_recved->req_type = IPC_REQ_JT808_0200;
    msg_recved->is_resp = 0;
    msg_recved->pack_id = 0;
    msg_recved->total_data_len = 24;
    msg_recved->data_len = 6;
    printf("req_type =%d\n",msg_recved->req_type);

    send_thread = std::thread(&Jt808::func_sendthread,this);
    recv_thread = std::thread(&Jt808::func_recvthread,this);
   
}

int Jt808::jt808_connect(jt808_client_handle_t client, int flag)
{
    client->wait_for_ping_resp = false;
    client->jt808_state.in_buffer_read_len = 0;
    client->jt808_state.message_length = 0;

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(client->config->host);
	server.sin_family = AF_INET;
    server.sin_port = htons(client->config->port);
    
    if (flag == 0){
        printf("down connecting....\n");
        if (connect(client->socket_down,(struct sockaddr *)&server,sizeof(server)) < 0)
        {
            return -1;
        }
        printf("down connected success\n");
    }else if (flag == 1){
        printf("up connecting....\n");
        if (connect(client->socket_up,(struct sockaddr *)&server,sizeof(server)) < 0)
        {
            return -1;
        }
        printf("up connected success\n");
    }
    return 0;
}

void Jt808::func_sendthread()
{
    if (jt808_connect(client,1) == -1)
    {
        printf("connect error\n");
        return ;
    }
    
    while(IsSend)
    {
        if (msg_recved->req_type == IPC_REQ_JT808_0200)
        {
            IsSend =false;
            msg_recved->req_type=0;
            printf("------ PROCESS: JT808-0200 Upload Location -------\n");
            process_upload_location( msg_recved );
            printf("------ PROCESS: JT808-0200 Upload Location Finish -------\n");
            //sleep(30);
        }
    }
   
}

void Jt808::func_recvthread()
{
    if (jt808_connect(client,0) == -1)
    {
        printf("connect error\n");
        return ;
    }

    while (1)
    {
        jt808_process_receive(client);
        sleep(200);
    }
}

package_t *Jt808::package_jt808_create(uint16_t buffer_size)
{
    package_t * package = NULL;
    uint8_t   * buffer  = NULL;
    uint16_t    temp_size = 0;

    temp_size = sizeof(package_t) + buffer_size;

    buffer = (uint8_t *)malloc(temp_size);
    if (buffer == NULL)
    {
        return NULL;
    }

    package = (package_t *) buffer;

    memset(package, 0, sizeof(package_t));
    package->buffer_size = buffer_size;

    return package;
}

uint32_t Jt808::sys_states_get_808_states_attribute(void)
{
     uint32_t states_attr = 0u;

    #define HLINK_STAT_ACC                      (1)                 // 0: ACC关;1:ACC开
    #define HLINK_STAT_GPS_VALID                (1 << 1)            // 0:GPS无效定位;1:GPS有效定位
    #define HLINK_STAT_SOUTH_LAT                (1 << 2)            // 0:北纬:1:南纬
    #define HLINK_STAT_WEST_LNG                 (1 << 3)            // 0:东经;1:西经
    #define HLINK_STAT_SHORT_CONNECT            (1 << 4)            // 0:长连接;1短连接
    #define HLINK_STAT_GPS_ON                   (1 << 5)            // 0：定位模块关闭; 1:定位模块打开
    #define HLINK_STAT_REST                     (1 << 6)            // 运动传感器状态-----0：运动状态; 1:静止状态
    #define HLINK_STAT_LOCKED                   (1 << 14)           // 0：解封;   1:施封;
    
    if(g_state_gps_info.longitude < 0){
    	states_attr |= HLINK_STAT_WEST_LNG;
    }

    if(g_state_gps_info.latitude < 0){
    	states_attr |= HLINK_STAT_SOUTH_LAT;
    }

    if(g_connection_long_short == 1){
    	states_attr |= HLINK_STAT_SHORT_CONNECT;
    }

    if(g_state_gps_info.task_run != 0){
    	states_attr |= HLINK_STAT_GPS_ON;
    }

    if(g_state_gps_info.is_valid != 0){
    	states_attr |= HLINK_STAT_GPS_VALID;
    }

    if (SealingState == 0x31){
        states_attr |= HLINK_STAT_LOCKED;
    }

    if (LOCK_POLE_STATUS == 1 || LOCK_POLE_STATUS == 2){     // 锁杆插入
        states_attr |= HLINK_STAT_SUOGAN;
    }
    if (charge_status != 0){   // 正在充电
    
        if (charge_status == 2){
            states_attr |= HLINK_STAT_CHARGED_FULL;
        }
        else{
            states_attr |= HLINK_STAT_CHARGING;
        }
    }

    if (g_status_rest == 1){       // 运动传感器状态-----0：运动状态; 1:静止状态
        states_attr |= HLINK_STAT_REST;
    }

    if(g_sim_card_id != 0){        // 0:缺省;  01:当前sim卡1;  10:当前sim卡2
        if(g_sim_card_id == 1){
            states_attr |= HLINK_STAT_SIMCARD1;
            states_attr &= ~HLINK_STAT_SIMCARD2;
        }
        else{
            states_attr |= HLINK_STAT_SIMCARD2;
            states_attr &= ~HLINK_STAT_SIMCARD1;
        }
    }
    return states_attr;
}

uint8_t Jt808::jt808_msg_get_ver(const uint8_t *data, int len)
{
#if JT808_TYPE_HLINK

    return 0;

#else

	if (len < JT808_HEAD_POS_MSG_VER + 1)
		return 0xff;

	return data[JT808_HEAD_POS_MSG_VER];

#endif
}

WORD Jt808::jt808_msg_get_msgid(const uint8_t *data, int len)
{
    if (len < JT808_HEAD_POS_MSG_ID + 2)
		return 0;

	return ((data[JT808_HEAD_POS_MSG_ID] << 8) | data[JT808_HEAD_POS_MSG_ID + 1]);
}

WORD Jt808::jt808_msg_get_msgsn(const uint8_t *data, int len)
{
    if (len < JT808_HEAD_POS_MSG_SN + 2)
		return 0;

	return (data[JT808_HEAD_POS_MSG_SN] << 8) | data[JT808_HEAD_POS_MSG_SN + 1];
}

/*****************************************************************************
//	描述：	将四个字节的数据存储到结构体中
//value 存储报文数据类型结构体
//id id
//v 需要存储的信息
*****************************************************************************/
int Jt808::jt808_value_set_dword(jt808_msg_value_t *value, uint8_t id, DWORD v)
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
int Jt808::jt808_value_set_word(jt808_msg_value_t *value, uint8_t id, WORD v)
{
    if (!value)
        return -1;

	value->id = id;
	value->type = JT808_DTYPE_WORD;
	value->value.d_word = v;

	return 0;
}

int Jt808::jt808_value_set_bytes(jt808_msg_value_t *value, uint8_t id, BYTE *v, int len)
{
   	if (!value)
		return -1;

	value->id = id;
	value->type = JT808_DTYPE_BYTE_ARR;
	value->value.d_bytes.data = v;
	value->value.d_bytes.len = len;

	return 0;
}

const char *Jt808::hhd_os_get_time_yymmddhhmmss_from_ts(char *datetime, int time_zone, hhd_os_time_t timestamp)
{
    struct tm *timenow = NULL;

    time_t temp = timestamp + (time_zone * 3600);

    timenow = localtime(&temp);
    sprintf(datetime, "%02d%02d%02d"
                      "%02d%02d%02d",
            (timenow->tm_year + 1900) % 100,
            timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour,
            timenow->tm_min, timenow->tm_sec);

    return datetime;
}

const char *Jt808::hhd_os_get_time_yymmddhhmmss(char *datetime, int time_zone)
{
    struct tm *timenow = NULL;

    time_t now = (time_zone * 3600);
    time(&now);
    // TODO, 时区！！！！
    timenow = gmtime(&now);
    sprintf(datetime,
            "%02d%02d%02d"
            "%02d%02d%02d",
            (timenow->tm_year + 1900) % 100, timenow->tm_mon + 1, timenow->tm_mday,
            timenow->tm_hour, timenow->tm_min, timenow->tm_sec);
    printf("datetime = %s\n", datetime);
    return datetime;
}

const jt808_msg_config_t *Jt808::package_get_jt808_config(jt808_send_channel_type_t send_channel, jt808_encrypt_type_t encrypt_type)
{
    if (g_jt808_msg_inited == 0)
    {
        g_jt808_msg_config.encrypt_type = encrypt_type;
        strncpy(g_jt808_msg_config.firm_version, HHD_FIRMWARE_VERSION, sizeof(g_jt808_msg_config.firm_version));
        strncpy(g_jt808_msg_config.term_no, "998877664455", sizeof(g_jt808_msg_config.term_no));
    }
    return &g_jt808_msg_config;
}

void Jt808::jt808_msg_init(jt808_connection_t *connection, const jt808_msg_config_t *config, uint8_t *buffer, size_t buffer_length)
{
    memset(connection, 0, sizeof(jt808_connection_t));
    connection->config = config;
    connection->buffer = (char *)buffer;
    connection->buffer_length = buffer_length;
}

int Jt808::jt808_value_set_string(jt808_msg_value_t *value, uint8_t id, const char *v)
{
    if (!value)
		return -1;

	value->id = id;
	value->type = JT808_DTYPE_STRING;
	value->value.d_string.data = v;
	value->value.d_string.len  = strlen(v);

	return 0;
}

int Jt808::jt808_value_set_byte(jt808_msg_value_t *value, uint8_t id, BYTE v)
{
    if (!value)
        return -1;

	value->id = id;
	value->type = JT808_DTYPE_BYTE;
	value->value.d_byte = v;

	return 0;
}

int Jt808::bsp_io_state_get_state(bsp_io_state_id_t state_id)
{
    if (state_id <= BSP_IO_STATE_UNKNOWN || state_id >= BSP_IO_STATE_MAX_COUNT)
    {
        printf("Error bsp_get_current_state, invalid state_id(%d)\n", state_id);
        return -1;
    }

    if ( (g_io_state & (1 << state_id)) != 0)
        return 1;
    else
        return 0;
}

int Jt808::buffer_append_byte(uint8_t *buffer, uint8_t v)
{
    buffer[0] = v;
	return 1;
}

int Jt808::buffer_append_dword(uint8_t *buffer, uint32_t v)
{
    buffer[0] = v >> 24;
	buffer[1] = v >> 16;
	buffer[2] = v >> 8;
	buffer[3] = v & 0xff;
	return 4;
}

int Jt808::jt808_append_dword(jt808_connection_t *connection, DWORD v)
{
    if (connection->buffer_length - connection->message.length < 4)
		return -1;

	connection->buffer[connection->message.length++] = v >> 24;
	connection->buffer[connection->message.length++] = v >> 16;
	connection->buffer[connection->message.length++] = v >> 8;
	connection->buffer[connection->message.length++] = v & 0xff;

	return 4;
}

int Jt808::jt808_append_bytes(jt808_connection_t *connection, const BYTE *v, uint16_t len)
{
    if (connection->message.length + len > connection->buffer_length || len == 0) {
        return 0;
    }

    memcpy(connection->buffer + connection->message.length, v, len);
    connection->message.length += len;
    return len;
}

int Jt808::jt808_msg_append_data_word_arr_with_count(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value)
{
    int i;
	int len;
	int total_len = 0;

	if (value->type != JT808_DTYPE_WORD_ARR_WITH_COUNT)
		return -1;

	len = jt808_append_word(connection, value->value.d_word_arr.count);
	if (len != 2)
	{
		printf("%s:%d(%s) Error append message\n", __FILE__, __LINE__, __func__);
		return -1;
	}

	total_len += len;
	for (i = 0; i < value->value.d_word_arr.count; ++i)
	{
		len = jt808_append_word(connection, value->value.d_word_arr.data[i]);
		if (len != 2)
		{
			printf("%s:%d(%s) Error append message\n", __FILE__, __LINE__, __func__);
			return -1;
		}
		total_len += len;
	}

	return total_len;
}

int Jt808::msg_value_get_dword(const jt808_msg_value_t *value, DWORD *word)
{
    uint8_t temp[12];
	if (value == NULL)
		return -1;
	if (value->type == JT808_DTYPE_BYTE)
		*word = value->value.d_byte;
	else if (value->type == JT808_DTYPE_WORD)
		*word = value->value.d_word;
	else if (value->type == JT808_DTYPE_DWORD)
		*word = value->value.d_dword;
	else if(value->type == JT808_DTYPE_STRING)
    {
        if(sizeof(temp) >= value->value.d_string.len)
        {
            memset(temp, 0, sizeof(temp));
            memcpy(temp, value->value.d_string.data, value->value.d_string.len);
            *word = atoi((char *)temp);
            return 0;
        }
        else
        {
            return -1;
        }
    }
	else
	{
		printf("Unsupported data type:%d\n", value->type);
		return -1;
	}

	return 0;
}

int Jt808::jt808_msg_append_data(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value, int judge)
{
   DWORD v;

	if (msg_value_get_dword(value, &v) != 0)
		return -1;

    if (judge == 0)
    {
        return append_byte(connection, v);
    }
    else if(judge == 1)
    {
        return jt808_append_word(connection, v);
    }
    else if (judge==2)
    {
        return jt808_append_dword(connection, v);
    }
    return 0;
}

int Jt808::jt808_msg_append_data_bytes(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value)
{
    const uint8_t *data_bytes = NULL;
	uint16_t data_bytes_len = 0;
	int len;

	if (value->type == JT808_DTYPE_STRING)
	{
		data_bytes = (const uint8_t *)value->value.d_string.data;
		data_bytes_len = value->value.d_string.data == NULL ? 0 : value->value.d_string.len;
	}
	else if (value->type == JT808_DTYPE_BYTE_ARR)
	{
		data_bytes = (uint8_t *)value->value.d_bytes.data;
		data_bytes_len = value->value.d_bytes.len;
	}

	// 右对齐，前面填充
	if (field_def->fix_length > 0 && field_def->align_right)
	{
		if (data_bytes_len > field_def->fix_length)
		{
			printf("%s:%d Field value invalid, [%s] fixed length: %d value length: %d\n",
					__FILE__, __LINE__, field_def->name, field_def->fix_length, data_bytes_len);
			return -1;
		}

		len = field_def->fix_length - data_bytes_len;
		if (len != jt808_append_byte_repeat(connection, field_def->padding_byte, len))
		{
			printf("%s:%d [%s] append byte error, out of buffer length, buffer_len: %d message_len: %d\n",
				__FILE__, __LINE__, field_def->name, connection->buffer_length, connection->message.length);
			return -1;
		}
	}

	len = 0;
	if (data_bytes_len > 0)
	{
		len = jt808_append_bytes(connection, (BYTE*)data_bytes, data_bytes_len);
		if (len != data_bytes_len)
		{
			printf("%s:%d [%s] append bytes error, out of buffer length, buffer_len: %d message_len: %d\n",
					__FILE__, __LINE__, field_def->name, connection->buffer_length, connection->message.length);
			return -1;
		}
	}

	// 固定长度，左对齐
	if (field_def->fix_length > 0 && !field_def->align_right)
	{
		len = field_def->fix_length - data_bytes_len;
		if (len != jt808_append_byte_repeat(connection, field_def->padding_byte, len))
		{
			printf("%s:%d [%s] append byte error, out of buffer length, buffer_len: %d message_len: %d\n",
				__FILE__, __LINE__, field_def->name, connection->buffer_length, connection->message.length);
			return -1;
		}
	}

	if (field_def->fix_length > 0)
		return field_def->fix_length;
	else
		return data_bytes_len;
}

int Jt808::jt808_msg_append_data_bcd(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value)
{
    int len;
	const uint8_t *data_bytes = NULL;
	uint16_t data_bytes_len = 0;

	if (value->type == JT808_DTYPE_STRING)
	{
		data_bytes = (const uint8_t *)value->value.d_string.data;
		data_bytes_len = value->value.d_string.data == NULL ? 0 : value->value.d_string.len;
	}
	else if (value->type == JT808_DTYPE_BYTE_ARR)
	{
		data_bytes = (uint8_t *)value->value.d_bytes.data;
		data_bytes_len = value->value.d_bytes.len;
	}

	// 长度为零且不需要补齐
	if (data_bytes_len ==  0 && field_def->fix_length == 0)
	{
		return 0;
	}

	len = field_def->fix_length > 0 ? field_def->fix_length : (data_bytes_len/2) + (data_bytes_len & 0x01);
	return jt808_append_bcd(connection, (char *)data_bytes, data_bytes_len, field_def->align_right, len);
}

jt808_msg_value_t *Jt808::get_msg_value_by_id(jt808_msg_value_t *values, int value_count, uint16_t field_id)
{
    if (values == NULL || value_count == 0)
		return NULL;

	while(--value_count >= 0)
	{
		if (values[value_count].id == field_id)
			return &values[value_count];
	}

	return NULL;
}

char *Jt808::msg_value_get_string(const jt808_msg_value_t *value, char *data, int len)
{
     static char bcd_table[] = "0123456789ABCDEF";
    int value_len;
    int i;
    DWORD dw_value;

    if (value == NULL || data == NULL || len < 1)
        return NULL;

    data[0] = '\0';
    switch (value->type)
    {
    case JT808_DTYPE_BCD:                           // BCD码取字节时原样返回
        if (value->value.d_bytes.data != NULL && value->value.d_bytes.len > 0)
        {
            if (value->value.d_bytes.len * 2 + 1 > len)
                return NULL;

            for (i = 0; i < value->value.d_bytes.len; ++i)
            {
                data[i * 2] = bcd_table[value->value.d_bytes.data[i] >> 4];
                data[i * 2 + 1] = bcd_table[value->value.d_bytes.data[i] & 0x0f];
            }
            data[i * 2] = '\0';
        }
        return data;

    case JT808_DTYPE_BYTE_ARR:
        if (value->value.d_bytes.data != NULL && value->value.d_bytes.len > 0)
        {
            if (value->value.d_bytes.len * 3 > len)
                return NULL;

            for (i = 0; i < value->value.d_bytes.len; ++i)
                snprintf(data + i * 3, len - 3 * i, "%02X \n", value->value.d_bytes.data[i]);
            data[i * 3 - 1] = '\0';
        }
        return data;

    case JT808_DTYPE_STRING:                    // 字符串取字节时放弃结束符\0
        if (value->value.d_string.data != NULL)
        {
            value_len = value->value.d_string.len;
            if (value_len + 1 > len)
                return NULL;

            memcpy(data, value->value.d_string.data, value_len);
            data[value_len] = '\0';
        }
        return data;

    case JT808_DTYPE_BYTE:
    case JT808_DTYPE_WORD:
    case JT808_DTYPE_DWORD:
        if (msg_value_get_dword(value, &dw_value) != 0)
            return NULL;

        snprintf(data, len, "%d\n", dw_value);
        return data;
    case JT808_DTYPE_NONE:

    	sprintf(data, "NULL\n");
    	return data;
    default:
        return NULL;
    }
}

/*****************************************************************************
//	描述：	报文定义的扩展字段
//connection 存储报文数据
//field_def 固定字段
//value 存储信息结构体
*****************************************************************************/
int Jt808::jt808_msg_append_data(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value)
{
    if (field_def == NULL)
		return 0;
    printf("[%s] [%s] id[%02x] type[%02x] \n", __func__, field_def->name, value->id, value->type);
	if (value == NULL && value->type != JT808_DTYPE_NONE)
	{
		printf("[%s] Value is NULL\n", field_def->name);
		if (field_def->fix_length == 0)
			return 0;

		if (field_def->fix_length != jt808_append_byte_repeat(connection, field_def->padding_byte, field_def->fix_length))
		{
			printf("%s:%d [%s] append byte error, out of buffer length, buffer_len: %d message_len: %d\n",
				__FILE__, __LINE__, field_def->name, connection->buffer_length, connection->message.length);
			return -1;
		}
		return field_def->fix_length;
	}
	if(value->type != JT808_DTYPE_NONE)
	{
		switch(field_def->target_type) {
            case JT808_DTYPE_BYTE:
                return jt808_msg_append_data(connection, field_def, value,0);

            case JT808_DTYPE_WORD:
                return jt808_msg_append_data(connection, field_def, value,1);

            case JT808_DTYPE_DWORD:
                return jt808_msg_append_data(connection, field_def, value,2);

            case JT808_DTYPE_STRING:
            case JT808_DTYPE_BYTE_ARR:
                return jt808_msg_append_data_bytes(connection, field_def, value);

            case JT808_DTYPE_BCD:
                printf("name[%s] type[%02x]\n", field_def->name, field_def->target_type);
                printf("pramenter ID [%02x] temp_len %d\n", value->id, value->value.d_string.len);
                return jt808_msg_append_data_bcd(connection, field_def, value);

            case JT808_DTYPE_WORD_ARR_WITH_COUNT:
                return jt808_msg_append_data_word_arr_with_count(connection, field_def, value);
            default:
                return 0;
		}
	}
	return 0;
}

int Jt808::jt808_append_byte_repeat(jt808_connection_t *connection, BYTE v, int count)
{
    if (count == 0 || connection->message.length + count > connection->buffer_length)
	{
		return 0;
	}
	memset(connection->buffer + connection->message.length, 0, count);
	connection->message.length += count;

	return count;
}

int Jt808::jt808_msg_append_checkcode(jt808_connection_t *connection)
{
    char *data;
    char crc = 0;
    int i = JT808_HEAD_POS_HEAD_START;

	if (connection->message.length < JT808_MAX_FIXED_HEADER_SIZE + 1
			|| connection->buffer_length - connection->message.length < 1)
	{
		return -1;
	}

	// buffer[0]为报文标志
	data = connection->buffer;
	for (i = JT808_HEAD_POS_HEAD_START; i < connection->message.length; ++i)
	{
		crc ^= data[i];
	}

	data[connection->message.length++] = crc;

	return 1;
}

jt808_message_t *Jt808::jt808_msg_final(jt808_connection_t *connection)
{
    char tmnow[32];
	int body_len = 0;
	int total_head_len = JT808_HEAD_POS_HEAD_START + JT808_MAX_FIXED_HEADER_SIZE;
//     char aes_key[16] = {
//         0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36
//         0x36, 0x36, 0x35, 0x35, 0x31, 0x32, 0x36, 0x36, 0x38, 0x38, 0x33, 0x34, 0x31, 0x32, 0x33, 0x34
//     };
    
    hhd_rsa_context_t rsa_key;


	// 如果分包了
#if JT808_TYPE_HLINK
	// 航鸿达无消息包封装项
#else
	if (jt808_msg_get_fragmented(connection->buffer, connection->message.length))
	{
		printf("Fragmented package.\n");
		total_head_len += 2;
	}
#endif
    printf("message_length = %d\n", connection->message.length);
	// 计算长度
	body_len = connection->message.length - total_head_len;
    // body_len = connection->buffer_length - total_head_len;
    printf("body_len = %d,message_length = %d\n",body_len, connection->message.length);

	// 更新报文长度信息
	connection->buffer[JT808_HEAD_POS_MSG_ATTR] |= 0x07 & (body_len >> 8);
	connection->buffer[JT808_HEAD_POS_MSG_ATTR + 1] = body_len & 0xff;
    
	// 计算校验码
	if (jt808_msg_append_checkcode(connection) != 1)
	{
		printf("%s:%d Error while append check code.\n", __FILE__, __LINE__);
		return NULL;
	}

	// 补充报文结束标志
	if (append_byte(connection, JT808_MSG_FLAG) != 1)
	{
		printf("%s:%d Error while append JT808_MSG_FLAG(0xFE).\n", __FILE__, __LINE__);
		return NULL;
    }

	return &connection->message;
}

WORD Jt808::jt808_msg_append_msgid(jt808_connection_t *connection, short message_id)
{
     if (connection->message.length + 2 > connection->buffer_length) {
        return 0;
    }

    while (message_id == 0) {
        message_id = ++connection->last_message_id;
    }

    printf("Message SN: %u\n", message_id);
    connection->buffer[connection->message.length++] = message_id >> 8;
    connection->buffer[connection->message.length++] = message_id & 0xff;

    return message_id;
}

int Jt808::jt808_append_bcd(jt808_connection_t *connection, const char *str, int len, bool isAlignRight, int bcdSize)
{
    int i;
    int highBitFlag = 1;
    int lenTarget;
    int dataIndex;

    char val;

    lenTarget = len / 2 + (len & 0x01);
    if (isAlignRight)
    {
        if (connection->buffer_length < connection->message.length + bcdSize
            || lenTarget > bcdSize)
        {
            return 0;
        }
        lenTarget = bcdSize;
        memset(&connection->buffer[connection->message.length], 0, lenTarget);
    }
    else
    {
        if (connection->buffer_length < connection->message.length + lenTarget)
        {
            return 0;
        }
    }

    if (isAlignRight)
    {
        connection->message.length += lenTarget - (len/2);  // 左边留空的字节数
        if (len & 0x01)
        {               // 长度为奇数
            connection->message.length -= 1;
            highBitFlag = 0;
        }
    }

    dataIndex = connection->message.length;
    for (i = 0; i < len; ++i) {
        if ((str[i] >= '0' && str[i] <= '9'))
            val = str[i] - '0';
        else if (str[i] >= 'a' && str[i] <= 'f')
            val = str[i] - 'a' + 10;
        else if (str[i] >= 'A' && str[i] <= 'F')
            val = str[i] - 'A' + 10;
        else
            return -2;

        if (highBitFlag) {
            connection->buffer[dataIndex] = val << 4;
        } else {
            connection->buffer[dataIndex++] |= val;
        }
        highBitFlag = !highBitFlag;
    }
    connection->message.length = dataIndex;

    return lenTarget;
}

int Jt808::append_byte(jt808_connection_t *conn, char v)
{
    if (conn->buffer_length - conn->message.length < 1)
	{
		return -1;
	}
	conn->buffer[conn->message.length++] = v;
	return 1;
}

int Jt808::jt808_append_word(jt808_connection_t *connection, WORD v)
{
    if (connection->buffer_length - connection->message.length < 2)
	{
		printf("buffer_length not enough...\n");
		return -1;
	}

	connection->buffer[connection->message.length++] = v >> 8;
	connection->buffer[connection->message.length++] = v & 0xff;

	return 2;
}

int Jt808::jt808_append_msgbody_attr(jt808_connection_t *connection, char verFlag, bool fragmented, jt808_encrypt_type_t encryptType, DWORD len)
{
    char h = 0;
	char l = 0;

	// 版本标识
	if (verFlag)
	{
		h |= JT808_HEAD_ATTR_H_VER_FLG;
	}

	// 分包
	if (fragmented)
	{
		h |= JT808_HEAD_ATTR_H_FRAGMENT;
	}

	// 消息体长度
	h |= (len >> 8) & 0x03;
	l = len & 0xff;

	if (-1 == append_byte(connection, h))
	    return -1;

	if (-1 == append_byte(connection, l))
		return 1;

	return 2;
}

/*****************************************************************************
//	描述：	打包消息头
//connection 存储报文数据
//msg_id 0x0200 结构体
//out_msg_sn 报文流水号
*****************************************************************************/
int Jt808::init_message(jt808_connection_t *connection, WORD msg_id, WORD *out_msg_sn)
{
    const jt808_msg_config_t *config = connection->config;
	WORD msg_sn;
	if (config == NULL)
	{
		printf("Error configuration not initialized!\n");
		return -1;
	}

    connection->message.data = connection->buffer;
    connection->message.length = 0;

    // 报文起始标志
    connection->message.data[connection->message.length++] = JT808_MSG_FLAG;

    // 消息ID
    jt808_append_word(connection, msg_id);
    printf("msg_id: 0x%04hX\n", msg_id);
    // 消息体属性
    jt808_append_msgbody_attr(connection, 0, 0, config->encrypt_type, 0);
    printf("encrypt_type: %u\n", config->encrypt_type);

// #if JT808_TYPE_HLINK
//     // 航鸿达版本无协议版本号字段
// #else
//     // 协议版本号
//     append_byte_escape(connection, 1);
// #endif

    // 终端手机号(终端编号)
    jt808_append_bcd(connection, (char *)config->term_no, strlen(config->term_no), true, JT808_HEAD_TERM_NO_LENGTH);
    printf("Term No: %s\n", config->term_no);

    // 消息流水号
    msg_sn = jt808_msg_append_msgid(connection, *out_msg_sn);
    if (out_msg_sn != NULL)
    {
        *out_msg_sn = msg_sn;
    }
    return connection->message.length;
}

jt808_msg_item_define_t *Jt808::get_msg_item_define_by_id(jt808_msg_item_define_t *defines, int count, uint16_t field_id)
{
     if (defines == NULL || count == 0)
        return NULL;

    while(--count >= 0)
    {
        if (defines[count].id == field_id)
            return &defines[count];
    }

    return NULL;
}

int Jt808::jt808_msg_append_extra_data(jt808_connection_t *connection, const jt808_msg_item_define_t *field_def, const jt808_msg_value_t *value)
{
    int len_pos;
    int data_len;

    if (connection == NULL || field_def == NULL || value == 0)
        return 0;

    // 附加数据项ID
    switch (field_def->id_len)
    {
        case 1:
            jt808_append_byte(connection, (uint8_t)field_def->id);
            break;

        case 2:
            jt808_append_word(connection, field_def->id);
            break;

        default:
            break;
    }

    // 附加数据项长度
    len_pos = connection->message.length;       // 保存长度位置
    if (field_def->len_len > 0)
    {
        jt808_append_byte_repeat(connection, 0x00, field_def->len_len);
    }

    data_len = jt808_msg_append_data(connection, field_def, value);
    if (data_len < 0)
    {
    	printf("extra_data_len %d\n", data_len);

    	return data_len;
    }

    if (data_len > 0 && field_def->len_len > 0)
    {
        // 修改附加字段长度
        switch (field_def->len_len)
        {
            case 1:
                connection->buffer[len_pos] = data_len & 0xff;
                break;

            case 2:
                connection->buffer[len_pos++] = data_len >> 8;
                connection->buffer[len_pos] = data_len & 0xff;
                break;

            default:
                printf("ExFiled define error [ext_len_len = %d]\n", field_def->len_len);
                return -1;
                break;
        }
    }

    return data_len + 2;
}

int Jt808::jt808_append_byte(jt808_connection_t *connection, BYTE v)
{
    if (connection->message.length + sizeof(BYTE) > connection->buffer_length) {
        return 0;
    }
    connection->buffer[connection->message.length++]  = v;
    return 1;
}


/*****************************************************************************
//	描述：	报文定义的扩展字段
//connection 存储报文数据
//msg_def 固定字段
//values 存储扩展字段的结构体
//value_count 存储的字段多少
*****************************************************************************/
int Jt808::jt808_pack_message_fix_fields(jt808_connection_t *connection, const jt808_msg_define_t *msg_def, jt808_msg_value_t *values, int value_count)
{
    int i;
	int rc;
	jt808_msg_value_t *item_value;
	char str[512];//单个附加数据最大的长度，如盲点数据，最大512
	char * temp_str = NULL;

	if (msg_def->field_count <= 0)
		return 0;

	if (msg_def->fields == NULL)
	{
		printf("%s, Field count[=%d] not zero but fields is NULL\n", __func__, msg_def->field_count);
		return 0;
	}

	printf("fix field count: %d\n", msg_def->field_count);
	for (i = 0; i < msg_def->field_count; ++i)
	{
		item_value = get_msg_value_by_id(values, value_count, msg_def->fields[i].id);
        //printf("id: [%02X] type[%02X]\n", item_value->id, item_value->type);
		rc = jt808_msg_append_data(connection, &msg_def->fields[i], item_value);
		if (rc < 0)
		{
			printf("%s:%d Make message error, field [%s] buff_len: %d  data_len: %d\n",
					__func__, __LINE__, msg_def->fields[i].name, connection->buffer_length, connection->message.length);
			return -1;
		}
		else
		{
		    temp_str = msg_value_get_string(item_value, str, sizeof(str));
		}
	}
	return 0;
}

int Jt808::jt808_pack_message_ext_fields(jt808_connection_t *connection, const jt808_msg_define_t *msg_def, jt808_msg_value_t *values, int value_count)
{
     int i;
    int rc;
    jt808_msg_item_define_t *def;
    char str[256];
    char * temp_str = NULL;
    if (msg_def->exfield_count <= 0)
        return 0;

    if (msg_def->exfields == NULL)
    {
        printf("%s, Extra field count[=%d] not zero but exfields is NULL\n", __func__, msg_def->exfield_count);
        return 0;
    }

    for (i = 0; i < value_count; ++i)
    {
        def = get_msg_item_define_by_id(msg_def->exfields, msg_def->exfield_count, values[i].id);
        if (def == NULL)
        {
            printf("%s:%d Message field value error, msg_id not found[%d]\n", __FILE__, __LINE__, values[i].id);
            return -1;
        }  
        rc = jt808_msg_append_extra_data(connection, def, &values[i]);
        if (rc < 0)
        {
        	printf("pramenter ID [%02x] temp_len %d\n", values[i].id, values[i].value.d_string.len);
        	if(values[i].type != JT808_DTYPE_NONE)
			{
                printf("values[i].value.d_string.data:%s,values[i].value.d_string.len:%d\n",values[i].value.d_string.data,values[i].value.d_string.len);
			}
            printf("%s:%d Make message error, field [%s] buff_len: %d  data_len: %d\n",
                    __func__, __LINE__, def->name, connection->buffer_length, connection->message.length);
            return -1;
        }
        else
        {
        	temp_str = msg_value_get_string(&values[i], str, sizeof(str));
        }
    }

    return 0;
}

/*****************************************************************************
//	描述：	打包报文数据
//connection 存储报文数据
//msg_def 固定字段
//data 附加字段信息
//out_msg_sn 报文流水号
*****************************************************************************/
jt808_message_t *Jt808::jt808_pack_message(jt808_connection_t *connection, const jt808_msg_define_t *msg_def, jt808_msg_fields_data_t *data, WORD *out_msg_sn)
{
    int rc;

	if (connection == NULL || msg_def == NULL)
	{
		return NULL;
	}

	printf("Packing message\n");
	printf("---------------\n");
	printf("MSG_ID: 0x%04hX\n", msg_def->msg_id);
	printf("BUF_LEN: %d\n", connection->buffer_length);
	if (msg_def->name)
	{
		printf("MSG_NAME: %s\n", msg_def->name);
	}
	printf("--- Main fields\n");
    // 打包消息头
    init_message(connection, msg_def->msg_id, out_msg_sn);

    if (msg_def->field_count == 0 && msg_def->exfield_count == 0)
    {
    	return jt808_msg_final(connection);
    }

    // 报文定义的固定字段
    rc = jt808_pack_message_fix_fields(connection, msg_def, data == NULL ? NULL : data->field_values, data == NULL ? 0 : data->field_values_count);
    if (rc != 0)
    {
    	printf("Make message error!\n");
    	return NULL;
    }

    // 报文附加字段
    if (data != NULL && data->exfield_values_count > 0)
    {
        if (data->exfield_values == NULL)
        {
            printf("%s:%d Giving extra fields count: %d, but extra fields is NULL\n", __FILE__, __LINE__, data->exfield_values_count);
            return NULL;
        }

        printf("--- Extra fields\n");
        rc = jt808_pack_message_ext_fields(connection, msg_def, data->exfield_values, data->exfield_values_count);
        if (rc != 0)
        {
            printf("Make message error [jt808_pack_message_ext_fields]!\n");
            return NULL;
        }
    }
    return jt808_msg_final(connection);
}

jt808_msg_define_t *Jt808::jt808_get_msg_define(uint16_t msg_id)
{
    switch (msg_id)
    {
    case JT808_MSG_TERM_HEART_BEAT: // 终端心跳 0002
        return &jt808_msg_term_hreartbreat;

    case JT808_MSG_TERM_LOCATION: // 实时位置状态信息汇报 0200
        return &jt808_msg_term_location;
    default:
        return NULL;
    }
}

int Jt808::pack_common_build_state_bits(DWORD *sate_bits)
{
    *sate_bits = sys_states_get_808_states_attribute();
    return 0;
}

int Jt808::hhd_event_get_ext_info(const hhd_event_log_t *evlog, uint8_t *data, uint16_t max_size)
{
    int len;
    uint8_t offset = 0;
    if (evlog->event_flag == 1)
    {
        switch (evlog->event)
        {
        case HHD_EV_SUOGAN_IN:
        case HHD_EV_SUOGAN_OUT:
        case HHD_EV_LOCK_BY_SUOGAN_IN:
        case HHD_EV_SUOGAN_OUT_IN_LOCK:
        case HHD_EV_WAIKE:
        case HHD_EV_FANGJIAN:
        case HHD_EV_LOW_VOLTAGE:
        case HHD_EV_INVALID_PASSWORD:
        case HHD_EV_WAIT_LOCK_TIMEOUT:
        case HHD_EV_DAOLI_WAKEUP:
        case HHD_EV_TOUCH_WAKEUP:
        case HHD_EV_USER_LOW_VOLTAGE:
        case HHD_EV_LOCK_MOTOR_STUCK:
        case HHD_EV_UNLOCK_MOTOR_STUCK:
        case HHD_EV_GPS_ANTEN_OPEN:
        case HHD_EV_GPS_ANTEN_SHORT_CIRCUIT:
        case HHD_EV_MCU_COMMUNICAT:
        case HHD_EV_CHARG_FULL:
        case HHD_EV_CHARGER_PULLOUT:
        case HHD_EV_CHARGER_INSERT:
        case HHD_EV_NORMAL_SHUTDOWN:
        case HHD_EV_BUSINESS_DATA_PLATFORM:
        case HHD_EV_BUSINESS_DATA_BLE:
        case HHD_EV_BUSINESS_DATA_GSENSOR:
        case HHD_EV_OVERSPEED_ALARM:
        case HHD_EV_PARKINT_OVERTIME_ALARM: 
        case HHD_EV_SMOKE_ALARM:
        case HHD_EV_WATER_ALARM:
        case HHD_EV_HIGH_TEMPERATURE_ALARM:
        case HHD_EV_TRIGGER_REPORT_LATEST_STATUS:
            data[0] = '\0';
            return 1;
        case HHD_EV_LOCK_SUBLOCK_BY_SUOGAN_IN:
        case HHD_EV_UNLOCK_SUBLOCK_BY_SUOGAN_OUT:
        case HHD_EV_SUBLOCK_BY_WAIKE_OPEN:
        case HHD_EV_LOCK_SUBLOCK_BY_SUOGAN_EXC:
        case HHD_EV_SUBLOCK_BY_TIMEOUT:
        case HHD_EV_SUBLOCK_BY_BATTERY_LOW:
        case HHD_EV_LOCK_SUBLOCK_BY_TIME_OUT:
        case HHD_PLATFORM_LORA_BE_DISMANTLED:
        case HHD_PLATFORM_LORA_TIMEOUT:
        case HHD_PLATFORM_LORA_RETURN_TO_NORMAL_BE:
        case HHD_PLATFORM_LORA_RETURN_TO_NORMAL:
            if (evlog->sublock_flag == 1)
            {
                memcpy(data, evlog->sublock_id, sizeof(evlog->sublock_id));
                offset += sizeof(evlog->sublock_id);
                data[offset] = '\0';
                return offset;
            }
            else
            {
                data[0] = '\0';
            }
            return 1;
        case HHD_EV_LOCK_BY_NFC:
        case HHD_EV_UNLOCK_BY_NFC:
        case HHD_EV_ERR_LOCK_INVALID_NFC:
        case HHD_EV_ERR_UNLOCK_INVALID_NFC:
        case HHD_EV_NFC_LOCK_OUTOF_AREA:
        case HHD_EV_NFC_UNLOCK_OUTOF_AREA:
        case HHD_EV_UNLOCK_SUBLOCK_BY_NFC:
        case HHD_EV_LOCK_SUBLOCK_BY_NFC:
        case HHD_EV_SUBLOCK_BY_UNLOCK_INVALID_NFC:
        case HHD_EV_SUBLOCK_BY_LOCK_INVALID_NFC:
        case HHD_EV_UNLOCK_SUBLOCK_BY_NFC_LORA:
        case HHD_EV_LOCK_SUBLOCK_BY_NFC_LORA:
        case HHD_EV_NO_VALID_TIME_NFC_LOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_NFC_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_NFC_LOCK_FAIL:
        case HHD_EV_NO_VALID_CARD_ID_NFC_LOCK_FAIL:
        case HHD_EV_NO_VALID_CARD_ID_NFC_UNLOCK_FAIL:
        case HHD_EV_IN_AREA_NFC_UNLOCK_SUCCESS:
        case HHD_EV_IN_AREA_NFC_LOCK_SUCCESS:
        case HHD_EV_OUT_AREA_NFC_UNLOCK_FAIL:
        case HHD_EV_OUT_AREA_NFC_LOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_NFC_TIME_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_NFC_TIME_LOCK_FAIL:
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_NFC_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_NFC_LOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_NFC_LOCK_FAIL:          
        case HHD_EV_SUBLOCK_UNVALID_NFC_UNLOCK_FAIL:        
        case HHD_EV_SUBLOCK_IN_AREA_NFC_UNLOCK_SUCCESS:    
        case HHD_EV_SUBLOCK_IN_AREA_NFC_LOCK_SUCCESS:      
        case HHD_EV_SUBLOCK_OUT_AREA_NFC_UNLOCK_FAIL:      
        case HHD_EV_SUBLOCK_OUT_AREA_NFC_LOCK_FAIL:
            if (evlog->sublock_flag == 1)
            {
                memcpy(data, evlog->sublock_id, sizeof(evlog->sublock_id));
                offset += sizeof(evlog->sublock_id);
            }
            len = strlen(evlog->nfc);
            if (len > 0 && len <= max_size)
            {
                memcpy(data + offset, evlog->nfc, len);
                offset += len;
                return offset;
            }
            else if (len == 0)
            {
                data[offset] = '\0';
                return 1;
            }
            break;

        case HHD_EV_LOCK_BY_BLE:
        case HHD_EV_UNLOCK_BY_BLE:
        case HHD_EV_UNLOCK_SUBLOCK_BY_BLE:
        case HHD_EV_LOCK_SUBLOCK_BY_BLE:
        case HHD_EV_UNLOCK_SUBLOCK_BY_BLE_LORA:
        case HHD_EV_LOCK_SUBLOCK_BY_BLE_LORA:
        case HHD_EV_SET_LOCK_SUBLOCK_BY_BLE_LORA:
        case HHD_EV_SET_UNLOCK_SUBLOCK_BY_BLE_LORA:
        case HHD_EV_NO_VALID_BLE_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_BLE_LOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_BLE_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_BLE_LOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_BLE_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_BLE_LOCK_FAIL:
        case HHD_EV_IN_AREA_BLE_LOCK_SUCCESS:
        case HHD_EV_IN_AREA_BLE_UNLOCK_SUCCESS:
        case HHD_EV_OUT_AREA_BLE_UNLOCK_FAIL:
        case HHD_EV_OUT_AREA_BLE_LOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_BLE_UNLOCK_FAIL:             
        case HHD_EV_SUBLOCK_UNVALID_BLE_LOCK_FAIL:               
        case HHD_EV_SUBLOCK_UNVALID_BLE_TIME_UNLOCK_FAIL:        
        case HHD_EV_SUBLOCK_UNVALID_BLE_TIME_LOCK_FAIL:          
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_BLE_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_BLE_LOCK_FAIL:  
        case HHD_EV_SUBLOCK_OUT_AREA_BLE_UNLOCK_FAIL:         
        case HHD_EV_SUBLOCK_OUT_AREA_BLE_LOCK_FAIL:           
        case HHD_EV_SUBLOCK_IN_AREA_BLE_UNLOCK_SUCCESS:       
        case HHD_EV_SUBLOCK_IN_AREA_BLE_LOCK_SUCCESS: 
            if (evlog->sublock_flag == 1)
            {
                memcpy(data, evlog->sublock_id, sizeof(evlog->sublock_id));
                offset += sizeof(evlog->sublock_id);
            }
            len = strlen(evlog->ble_user);
            if (len > 0 && len <= max_size)
            {
                memcpy(data + offset, evlog->ble_user, len);
                offset += len;
                return offset;
            }
            else if (len == 0)
            {
                data[offset] = '\0';
                return 1;
            }
            break;

        case HHD_EV_LOCK_BY_PLATFORM:
        case HHD_EV_UNLOCK_BY_PLATFORM:
        case HHD_EV_UNLOCK_SUBLOCK_BY_PLATFORM_LORA:
        case HHD_EV_LOCK_SUBLOCK_BY_PLATFORM_LORA:
        case HHD_EV_SET_LOCK_SUBLOCK_BY_PLATFORM_LORA:
        case HHD_EV_SET_UNLOCK_SUBLOCK_BY_PLATFORM_LORA:
        case HHD_EV_FIRMWARE_VERSION:
        case HHD_EV_NO_VALID_PLATFORM_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_PLATFORM_LOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_PLATFORM_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_PLATFORM_LOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_PLATFORM_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_PLATFORM_LOCK_FAIL:
        case HHD_EV_IN_AREA_PLATFORM_LOCK_SUCCESS:
        case HHD_EV_IN_AREA_PLATFORM_UNLOCK_SUCCESS:
        case HHD_EV_OUT_AREA_PLATFORM_UNLOCK_FAIL:
        case HHD_EV_OUT_AREA_PLATFORM_LOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_PLATFORM_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_PLATFORM_LOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_PLATFORM_TIME_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_PLATFORM_TIME_LOCK_FAIL:
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_PLATFORM_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_PLATFORM_LOCK_FAIL:
        case HHD_EV_SUBLOCK_OUT_AREA_PLATFORM_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_OUT_AREA_PLATFORM_LOCK_FAIL:
        case HHD_EV_SUBLOCK_IN_AREA_PLATFORM_UNLOCK_SUCCESS:
        case HHD_EV_SUBLOCK_IN_AREA_PLATFORM_LOCK_SUCCESS:  
            if (evlog->sublock_flag == 1)
            {
                memcpy(data, evlog->sublock_id, sizeof(evlog->sublock_id));
                offset += sizeof(evlog->sublock_id);
            }
            len = strlen(evlog->plat_user);
            if (len > 0 && len <= max_size)
            {
                memcpy(data + offset, evlog->plat_user, len);
                offset += len;
                return offset;
            }
            else if (len == 0)
            {
                data[offset] = '\0';
                return 1;
            }

            break;

        case HHD_EV_LOCK_BY_SMS:
        case HHD_EV_UNLOCK_BY_SMS:
        case HHD_EV_LOCK_SUBLOCK_BY_SMS_LORA:
        case HHD_EV_UNLOCK_SUBLOCK_BY_SMS_LORA:
        case HHD_EV_SET_LOCK_SUBLOCK_BY_SMS_LORA:
        case HHD_EV_SET_UNLOCK_SUBLOCK_BY_SMS_LORA:
        case HHD_EV_NO_VALID_SMS_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_SMS_LOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_SMS_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_SMS_LOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_SMS_UNLOCK_FAIL:
        case HHD_EV_NO_VALID_TIME_INTERVAL_SMS_LOCK_FAIL:
        case HHD_EV_IN_AREA_SMS_LOCK_SUCCESS:
        case HHD_EV_IN_AREA_SMS_UNLOCK_SUCCESS:
        case HHD_EV_OUT_AREA_SMS_LOCK_FAIL:
        case HHD_EV_OUT_AREA_SMS_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_SMS_LOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_SMS_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_SMS_TIME_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_UNVALID_SMS_TIME_LOCK_FAIL:
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_SMS_UNLOCK_FAIL:
        case HHD_EV_SUBLOCK_VALID_TIME_INTERVAL_SMS_LOCK_FAIL:
        case HHD_EV_SUBLOCK_IN_AREA_SMS_LOCK_SUCCESS:
        case HHD_EV_SUBLOCK_IN_AREA_SMS_UNLOCK_SUCCESS:
        case HHD_EV_SUBLOCK_OUT_AREA_SMS_LOCK_FAIL:
        case HHD_EV_SUBLOCK_OUT_AREA_SMS_UNLOCK_FAIL:
            if (evlog->sublock_flag == 1)
            {
                memcpy(data, evlog->sublock_id, sizeof(evlog->sublock_id));
                offset += sizeof(evlog->sublock_id);
            }
            len = strlen(evlog->telno);
            if (len > 0 && len <= max_size)
            {
                memcpy(data + offset, evlog->telno, len);
                offset += len;
                return offset;
            }
            else if (len == 0)
            {
                data[offset] = '\0';
                return 1;
            }
            break;
        case HHD_EV_UNLOCK_BY_STATIC_PSWORD:
        case HHD_EV_UNLOCK_BY_RANDOM_PSWORD:
            if (evlog->sublock_flag == 1)
            {
                memcpy(data, evlog->sublock_id, sizeof(evlog->sublock_id));
                offset += sizeof(evlog->sublock_id);
            }
            len = strlen(evlog->ble_user);
            if (len > 0 && len <= max_size)
            {
                memcpy(data + offset, evlog->password, len);
                offset += len;
                return offset;
            }
            else if (len == 0)
            {
                data[offset] = '\0';
                return 1;
            }
            break;
        case HHD_EV_REMOTE_OPEN_DOOR_SUCCESS:
        case HHD_EV_BLUETOOTH_OPEN_DOOR_SUCCESS:
        case HHD_EV_REMOTE_OPEN_ALL_BOXES:
        case HHD_EV_BLUETOOTH_OPEN_ALL_BOXES:
            data[0] = evlog->box;
            offset+=1;
            len = strlen(evlog->plat_user);
            if (len > 0 && len <= max_size)
            {
                memcpy(data + offset, evlog->plat_user, len);
                offset += len;
                return offset;
            }
            break;
        case HHD_EV_BIND_BOX_LOCK:
        case HHD_EV_UNBIND_BOX_LOCK:
            data[0] = evlog->box;
            offset+=1;
            if(evlog->ble_mac != NULL)
            {
                memcpy(data + offset, evlog->plat_user, 6);
                offset += 6;
            }
            len = strlen(evlog->plat_user);
            if (len > 0 && len <= max_size)
            {
                memcpy(data + offset, evlog->plat_user, len);
                offset += len;
                return offset;
            }
            break;
        case HHD_EV_PCB_OPEN_ALL_BOXES:
        case HHD_EV_PHYSICAL_EMERGENCY_OPEN_BOX:
        case HHD_EV_DOOR_OPEN:
        case HHD_EV_CLOSE_DOOR:
            data[0] = evlog->box;
            return 1;
        break;

        //箱子标号
        default:
            data[0] = '\0';
            return 1;
        }
    }
    return -1;
}


int Jt808::pack_jt808_0200(package_t *package, uint8_t *out_data, uint16_t out_data_size, hhd_event_log_t *event, jt808_send_channel_type_t send_channel, jt808_encrypt_type_t encrypt_type)
{
    jt808_connection_t message_data;
    const jt808_msg_config_t *config;
    jt808_msg_fields_data_t fields_data;
    jt808_msg_value_t values[FIELD_0200_MAX_FIELD_COUNT];
    jt808_msg_value_t exvalues[EXT_FIELD_MAX_COUNT];
    uint16_t voltage = 0;
    char tmnow[32] = {0};
    uint8_t i = 0;
    uint8_t event_data[128];
    uint8_t base_station_info[128];
    uint8_t data_frame[3] = {0};
    uint8_t base_station_info_len = 0;
    uint8_t lbs_info[9];
    uint8_t reported_time[5];
    int ext_field_index = 0;
    char file_name[60];
    int  dword_bits;
    int err = 0;
    unsigned char video_buff[100];
    static char send_box_buff[256] = {0};
    uint8_t Gravity_sensor[4];
    uint8_t satellite_cn[10] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa};
    uint8_t mac_addresses[10][7] = { 0 };
    uint8_t lock_status[10][12] = { 0 };
    uint8_t sublock_status_data[12] = { 0x00,0X31,0X39,0X33,0X31,0X38,0X38,0x10,0x22,0x30,0x01,0x50 };

    static uint16_t ap_data_len;
    static uint32_t wifi_ap_scan_tick_last = 0;

    const char *password = NULL; // 键盘密码
    

    // ---------------------------
    // - 初始化
    // ---------------------------
    memset(&fields_data, 0, sizeof(jt808_msg_fields_data_t));

    package->package_type = JT808_MSG_TERM_LOCATION;
    package->max_retry = PACKAGE_MAX_RETRY_COUNT;
    package->qos = QOS_ONLY_ONECE;
    package->fail_op = FAIL_OP_SAVE_OFFLINE;

    // ---------------------------
    // - 固定字段值
    // ---------------------------
    // 报警标志
    pack_common_build_warn_bits(&dword_bits);
    jt808_value_set_dword(&values[0], FIELD_0200_WARN_FLG, dword_bits);

    // 状态
    pack_common_build_state_bits(&dword_bits);
    jt808_value_set_dword(&values[1], FIELD_0200_STAT, dword_bits);

    jt808_value_set_dword(&values[2], FIELD_0200_LATITUDE, (DWORD)abs(g_state_gps_info.latitude * 1000000));   // 纬度
    jt808_value_set_dword(&values[3], FIELD_0200_LONGITUDE, (DWORD)abs(g_state_gps_info.longitude * 1000000)); // 经度
    // 判断定位是否有效，有效：上报实时数据  无效：上报0
    if (g_state_gps_info.is_valid == 1)
    {
        jt808_value_set_word(&values[4], FIELD_0200_ALTITUDE, (DWORD)g_state_gps_info.altitude); // 高程
        if (bsp_io_state_get_state(BSP_IO_STATE_GSENSOR_SHARK) == 0)                           // 三轴无运动
        {
            jt808_value_set_word(&values[5], FIELD_0200_SPEED, (WORD)0); // 速度
        }
        else
        {
            jt808_value_set_word(&values[5], FIELD_0200_SPEED, (WORD)(g_state_gps_info.speed * 10)); // 速度
        }
        jt808_value_set_word(&values[6], FIELD_0200_DIRECT, (WORD)g_state_gps_info.course); // 方向
    }
    else
    {
        jt808_value_set_word(&values[4], FIELD_0200_ALTITUDE, (DWORD)0); // 高程
        jt808_value_set_word(&values[5], FIELD_0200_SPEED, (WORD)0);     // 速度
        jt808_value_set_word(&values[6], FIELD_0200_DIRECT, (WORD)0);    // 方向
    }

    g_sensor_data.temperature = 0x1122;
    g_sensor_data.temperature_alarm_flag = 0x00;
    data_frame[0] = (uint8_t)(g_sensor_data.temperature >> 8); // 高位
    data_frame[1] = (uint8_t)g_sensor_data.temperature;        // 低位
    data_frame[2] = g_sensor_data.temperature_alarm_flag;

    g_sensor_data.Gravity = 0x11223344;
    Gravity_sensor[0] = (uint8_t)(g_sensor_data.Gravity >> 24); // 高位
    Gravity_sensor[1] = (uint8_t)(g_sensor_data.Gravity >> 16);
    Gravity_sensor[2] = (uint8_t)(g_sensor_data.Gravity >> 8);
    Gravity_sensor[3] = (uint8_t)g_sensor_data.Gravity; // 低位
    
    jt808_value_set_dword(&exvalues[ext_field_index++], EXT_FIELD_0200_MILEAGE, 0x11025136);
    jt808_value_set_word(&exvalues[ext_field_index++], EXT_FIELD_0200_FUEL_LEFT, 0x0125);
    jt808_value_set_word(&exvalues[ext_field_index++], EXT_FIELD_0200_TANK_FUEL_CAP, 0x0164);
    jt808_value_set_bytes(&exvalues[ext_field_index++], EXT_FIELD_0200_LOAD_SENSOR, (BYTE *)Gravity_sensor,sizeof(Gravity_sensor));
    jt808_value_set_bytes(&exvalues[ext_field_index++], EXT_FIELD_0200_TEMPERATURE_SENSOR, (BYTE *)data_frame, sizeof(data_frame));
    jt808_value_set_byte(&exvalues[ext_field_index++], EXT_FIELD_0200_SMOKE_SENSOR, g_sensor_data.smong_alarm_flag);
    jt808_value_set_byte(&exvalues[ext_field_index++], EXT_FIELD_0200_WATER_SENSOR, g_sensor_data.water_alarm_flag);

    for (int i = 0; i < g_box_number; i++) {
        memcpy(mac_addresses[i],g_box_list[i].wifi_mac, 7);
    }
    jt808_value_set_bytes(&exvalues[ext_field_index++], EXT_FIELD_0200_WIFI_MAC, (BYTE *)mac_addresses[0], 7*g_box_number);

    for (int i = 0; i < g_box_number; i++) {
        memcpy(mac_addresses[i],g_box_list[i].bluetooth_mac, 7);
    }
    jt808_value_set_bytes(&exvalues[ext_field_index++], EXT_FIELD_0200_BLE_MAC, (BYTE *)mac_addresses[0], 7*g_box_number);

    // 如果是事件上报
    if (event != NULL)
    {
        hhd_os_get_time_yymmddhhmmss_from_ts(tmnow, g_state_time_zone, event->ts);
        jt808_value_set_string(&values[7], FIELD_0200_TIME, tmnow);
        if (event->event_flag == 1)
        {
            package->is_0200_event = 1;
        }
        else
        {
            package->is_0200_event = 0;
        }
    }
    else
    {
        // 预防没有传入时间
        //  时间，上送平台使用UTC时间，新协议统一0时区
        jt808_value_set_string(&values[7], FIELD_0200_TIME, hhd_os_get_time_yymmddhhmmss(tmnow, g_state_time_zone));
        package->is_0200_event = 0;
    }

    fields_data.field_values = values;
    fields_data.field_values_count = FIELD_0200_MAX_FIELD_COUNT;

    package->sn = Config::package_get_sn(); // 报文流水号


    if (event != NULL)
    {
        if (event->event_flag == 1)
        {
            int event_data_len = 0;
            int temp_len = 0;

            event_data[event_data_len++] = (event->event >> 8) & 0x00FFu;
            event_data[event_data_len++] = event->event & 0x00FFu;
            event_data[event_data_len++] = event->level;

            temp_len = hhd_event_get_ext_info(event, event_data + event_data_len, sizeof(event_data) - event_data_len);
            if (temp_len < 0)
            {
                return -1;
            }
            event_data_len += temp_len;
            jt808_value_set_bytes(&exvalues[ext_field_index++], EXT_FIELD_0200_EVENT, (BYTE *)event_data, event_data_len);

            if (event->event == HHD_EV_FIRMWARE_VERSION)
            {
                package->is_upgrade_versions = 1;
            }
        }
    }

    //  当前回传间隔模式+下次0x200上报间隔
    buffer_append_byte(&reported_time[0], 0);
    buffer_append_dword(&reported_time[1], 5);
    jt808_value_set_bytes(&exvalues[ext_field_index++], EXT_FIELD_0200_REPORTED_TIME, (BYTE *)&reported_time[0], 5);
    
    fields_data.exfield_values = exvalues;
    fields_data.exfield_values_count = ext_field_index;

    // ---------------------------
    // - 打包数据
    // ---------------------------
    config = package_get_jt808_config(send_channel, encrypt_type);

    jt808_msg_init(&message_data, config, out_data, out_data_size);
    package->encrypt = encrypt_type;

    if (jt808_pack_message(&message_data, jt808_get_msg_define(JT808_MSG_TERM_LOCATION), &fields_data, &package->sn) != NULL){
        printf("jt808_pack_message SUCCESS...\n");
        package->data_len = message_data.message.length;
        return message_data.message.length;
    }else {
        printf("jt808_pack_message ERROR...\n");
        return -1;
    }
    
}

int Jt808::pack_common_build_warn_bits(DWORD *warn_bits)
{
    *warn_bits = sys_warn_get_808_warn_attribute();
    return 0;
}

void Jt808::SetIsSend(bool send)
{
    IsSend = send;
}

/*!
 * @brief 检测是否触发防剪报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_suogan_fangjian(void)
{
    printf("检测是否触发防剪报警\n");
    return CHECK_RESULT_UNKNOWN;
}

/*!
 * @brief   检测是否触发防拆报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_waike_fangchai(void)
{
    printf("检测是否触发防拆报警\n");
    return CHECK_RESULT_UNKNOWN;
    // BSP_IO_STATE_FCSW状态为0时，表示防拆报警未触发
    // return (bsp_io_state_get_current_state(BSP_IO_STATE_FCSW)) == 0 ? CHECK_RESULT_PASS : CHECK_RESULT_NO_PASS;
}

/*!
 * @brief   检测是否触发低电量报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_low_battery(void)
{
     printf("检测是否触发低电量报警\n");
     // return (bspal_battery_vol() > bsapal_battery_get_low()) ? CHECK_RESULT_PASS : CHECK_RESULT_NO_PASS;
    return (warn_check_state_t)0;
}

/*!
 * @brief   检测是否触发主副MCU通讯报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_mcucom(void)
{
    printf("检测是否触发主副MCU通讯报警\n");
    return CHECK_RESULT_PASS;
}

/*!
 * @brief   检测是否触发锁杆/锁绳子内部导线报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_suogan_daoxian(void)
{
    printf("检测是否触发锁杆\n");
    Jt808 *jt808 =new Jt808();
    //判断是否为施封状态
    if(jt808->SealingState == 0x31)
    {
        //判断锁绳是否正常
        if(jt808->LOCK_POLE_STATUS != 2)
        {
            return CHECK_RESULT_PASS;
        }
        else 
        {
            return CHECK_RESULT_NO_PASS;
        }
    }
    //解封状态不需要判断
    return CHECK_RESULT_PASS;
}

/*!
 * @brief   检测是否触发天线短路报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_tixian_short_circuit(void)
{
    printf("检测是否触发天线短路报警\n");
    Jt808 *jt808 =new Jt808();
    if(jt808->g_state_gps_info.antenna_status == 2)
	{
		return CHECK_RESULT_NO_PASS;
	}
	else
	{
		return CHECK_RESULT_PASS;
	}
    
}

/*!
 * @brief   检测是否触发天线断路报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_tixian_broken_circuit(void)
{
    printf("检测是否触发天线断路报警\n");
    Jt808 *jt808 =new Jt808();
    if(jt808->g_state_gps_info.antenna_status == 1)
	{
		return CHECK_RESULT_NO_PASS;
	}
	else
	{
		return CHECK_RESULT_PASS;
	}
}

/*!
 * @brief   检测是否触发电机施封卡死报警
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_motor_shifeng(void)
{
    printf("检测是否触发电机施封卡死报警\n");
    return CHECK_RESULT_PASS;
}

/*!
 * @brief   检测是否触发电机解封卡死报警
 *
 * @return
*/
warn_check_state_t Jt808::sys_warn_check_motor_jiefeng(void)
{
    printf("检测是否触发电机解封卡死报警\n");
    return CHECK_RESULT_PASS;
}

/*!
 * @brief   检测是否触发终端主电源掉电
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_main_power_lost(void)
{
    printf("检测是否触发终端主电源掉电\n");
    return CHECK_RESULT_PASS;
}

/*!
 * @brief 检测是否触发
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_speed()
{
    printf("检测是否触发\n");
    Jt808 *jt808 =new Jt808();
    if(g_car_speed_warning_num != 0)
    {
        if(jt808->g_state_gps_info.is_valid == 1)
        {
            if(jt808->g_state_gps_info.speed < g_car_speed_warning_num)
            {
                return CHECK_RESULT_PASS;
            } 
            else
            {
                return CHECK_RESULT_NO_PASS;
            }
        }
    }
    return CHECK_RESULT_UNKNOWN;
}

/*!
 * @brief 检测是否超时停车
 *
 * @return
 */
warn_check_state_t Jt808::sys_warn_check_chaoshi()
{
    printf("检测是否超时停车\n");
    if(g_sate_last_time == true)
    {
        return CHECK_RESULT_NO_PASS;
    } 
    else
    {

        return CHECK_RESULT_PASS;
    }
}

/*!
 * @brief   返回符合808报文定义的终端报警标志位
 *
 * @return
 */
uint32_t Jt808::sys_warn_get_808_warn_attribute(void)
{
   uint32_t  warn_attr = 0u;
    int i;

    for (i = 0; i < 12; ++i)
    {
        if (g_warn_to_808_config[i].check_fun == NULL){
            continue;
        }

        switch(g_warn_to_808_config[i].check_fun())
        {
            case CHECK_RESULT_NO_PASS:
                warn_attr |= g_warn_to_808_config[i].bit;
                break;

            case CHECK_RESULT_PASS:
            default:
                break;
        }
    }
    return warn_attr;
}
