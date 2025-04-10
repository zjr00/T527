#include "base_json.h"

base_json::base_json()
{
    //montage_json();
}

base_json::~base_json()
{
    
}

void base_json::montage_json()
{
    // 创建根对象
    cJSON *root = cJSON_CreateObject();
    
    // 添加基础字段
    cJSON_AddStringToObject(root, "name", "张三");
    cJSON_AddNumberToObject(root, "age", 28);
    cJSON_AddBoolToObject(root, "is_employed", 1);
    
    // 创建嵌套地址对象
    cJSON *address = cJSON_CreateObject();
    cJSON_AddStringToObject(address, "street", "科技大道123号");
    cJSON_AddStringToObject(address, "city", "北京");
    cJSON_AddStringToObject(address, "country", "中国");
    cJSON_AddItemToObject(root, "address", address);

    // 创建技能数组
    cJSON *skills = cJSON_CreateArray();
    cJSON_AddItemToArray(skills, cJSON_CreateString("C/C++"));
    cJSON_AddItemToArray(skills, cJSON_CreateString("Linux"));
    cJSON_AddItemToArray(skills, cJSON_CreateString("网络编程"));
    cJSON_AddItemToObject(root, "skills", skills);

    // 创建项目经历数组（包含嵌套对象）
    cJSON *projects = cJSON_CreateArray();
    
    // 项目1
    cJSON *proj1 = cJSON_CreateObject();
    cJSON_AddStringToObject(proj1, "name", "网络协议栈开发");
    cJSON_AddNumberToObject(proj1, "duration", 18);
    cJSON_AddBoolToObject(proj1, "completed", 1);
    
    // 项目技术栈数组
    cJSON *tech1 = cJSON_CreateArray();
    cJSON_AddItemToArray(tech1, cJSON_CreateString("C++11"));
    cJSON_AddItemToArray(tech1, cJSON_CreateString("Boost.Asio"));
    cJSON_AddItemToArray(tech1, cJSON_CreateString("Linux"));
    cJSON_AddItemToObject(proj1, "technologies", tech1);
    
    cJSON_AddItemToArray(projects, proj1);

    // 项目2
    cJSON *proj2 = cJSON_CreateObject();
    cJSON_AddStringToObject(proj2, "name", "嵌入式系统开发");
    cJSON_AddNumberToObject(proj2, "duration", 12);
    cJSON_AddBoolToObject(proj2, "completed", 0);
    
    cJSON *tech2 = cJSON_CreateArray();
    cJSON_AddItemToArray(tech2, cJSON_CreateString("C"));
    cJSON_AddItemToArray(tech2, cJSON_CreateString("RTOS"));
    cJSON_AddItemToArray(tech2, cJSON_CreateString("ARM"));
    cJSON_AddItemToObject(proj2, "technologies", tech2);
    
    cJSON_AddItemToArray(projects, proj2);
    cJSON_AddItemToObject(root, "projects", projects);

    // 生成JSON字符串
    char *json_str = cJSON_Print(root);
    cout <<"拼接后的json报文："<<json_str<<endl;
    string encode =base64_encode((unsigned char *)json_str,strlen(json_str));
    cout<<"加密数据："<<encode<<endl;

    string dencode =base64_decode(encode);
    cout<<"解密数据："<<dencode<<endl;

    anal_json(dencode.c_str());
    // 清理内存
    cJSON_Delete(root);
}

void base_json::anal_json(const char *json_str)
{
    // 解析JSON字符串
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        printf("解析失败: %s\n", cJSON_GetErrorPtr());
        return;
    }

    // 解析顶层字段
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *age = cJSON_GetObjectItemCaseSensitive(root, "age");
    cJSON *is_employed = cJSON_GetObjectItemCaseSensitive(root, "is_employed");
    
    if (cJSON_IsString(name)) {
        printf("姓名: %s\n", name->valuestring);
    }
    if (cJSON_IsNumber(age)) {
        printf("年龄: %d\n", age->valueint);
    }
    if (cJSON_IsBool(is_employed)) {
        printf("在职状态: %s\n", cJSON_IsTrue(is_employed) ? "是" : "否");
    }

    // 解析地址对象
    cJSON *address = cJSON_GetObjectItem(root, "address");
    if (address) {
        cJSON *street = cJSON_GetObjectItem(address, "street");
        cJSON *city = cJSON_GetObjectItem(address, "city");
        cJSON *country = cJSON_GetObjectItem(address, "country");
        
        printf("\n地址信息:\n");
        printf("街道: %s\n", street->valuestring);
        printf("城市: %s\n", city->valuestring);
        printf("国家: %s\n", country->valuestring);
    }

    // 解析技能数组
    cJSON *skills = cJSON_GetObjectItem(root, "skills");
    if (skills) {
        printf("\n技能列表:\n");
        cJSON *skill;
        int index = 1;
        cJSON_ArrayForEach(skill, skills) {
            printf("%d. %s\n", index++, skill->valuestring);
        }
    }

    // 解析项目数组
    cJSON *projects = cJSON_GetObjectItem(root, "projects");
    if (projects) {
        printf("\n项目经历:\n");
        cJSON *project;
        int proj_index = 1;
        
        cJSON_ArrayForEach(project, projects) {
            cJSON *proj_name = cJSON_GetObjectItem(project, "name");
            cJSON *duration = cJSON_GetObjectItem(project, "duration");
            cJSON *completed = cJSON_GetObjectItem(project, "completed");
            cJSON *techs = cJSON_GetObjectItem(project, "technologies");

            printf("\n项目%d:\n", proj_index++);
            printf("名称: %s\n", proj_name->valuestring);
            printf("周期: %d个月\n", duration->valueint);
            printf("完成状态: %s\n", cJSON_IsTrue(completed) ? "已完成" : "进行中");

            if (techs) {
                printf("技术栈: ");
                cJSON *tech;
                cJSON_ArrayForEach(tech, techs) {
                    printf("%s ", tech->valuestring);
                }
                printf("\n");
            }
        }
    }

    // 释放内存
    cJSON_Delete(root);
}

//加密
string base_json::base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len)
{
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);//每次取出三个bytes
        if (i == 3) {
                            /*取第一个byte的高六位*/
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                                /*取第一个byte的底二位*/           /*取第二个byte的高四位*/
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                                /*取第二个byte的底二位*/           /*取第三个byte的高四位*/
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                                /*取第三个byte的底六位*/
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)//四个六位byte， 2^6=64 ,从base64表里面查下标对应的字符
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)//bytes不是3的倍数时
    {
        for(j = i; j < 3; j++)//填充0凑够倍数
            char_array_3[j] = '\0';

        //剩余的byte继续编码
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        // 这里 i+1 byte编码总有低位在下一个char_array_4中
        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];
        
        //剩余没有编码的填充=
        while((i++ < 3))
            ret += '=';

    }

    return ret;
}

bool base_json::is_base64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

//解密
string base_json::base64_decode(string const &encoded_string)
{
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) 
    {

        char_array_4[i++] = encoded_string[in_]; 
        in_++;
        
        if (i == 4)//四个base64字符为一组 
        {
            //找到对应base64字符在base64的下标值
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            //通过下标值移位还原原来字符串字符对应的值
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) //填充的=最后处理
    {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < i ; j++)
        {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}