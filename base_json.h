#ifndef BASE_JSON_H
#define BASE_JSON_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include "cJSON.h"

using namespace std;

struct Control
{

};

class base_json
{

public:
    base_json();
    ~base_json();

    const std::string base64_chars = 
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
               "abcdefghijklmnopqrstuvwxyz"
               "0123456789+/";

    void montage_json();
    void anal_json(const char *json_str);
private:
    //编码  
    string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);


    bool is_base64(unsigned char c);

    //解码
    string base64_decode(string const& encoded_string);
    
};

#endif