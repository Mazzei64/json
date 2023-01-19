#ifndef _JSON_H_
#define _JSON_H_

#include<stdlib.h>
#include<string.h>

typedef char* string;
typedef unsigned char byte;
typedef enum {false = 0, true} bool;

typedef struct {
    string key;
    void *value;
    string value_type;
    unsigned int key_len;
    unsigned int value_byte_count;
} JToken;

typedef struct {
    // JObject **jobj_list;
    JToken **jtoken_list;
    unsigned int jobj_list_count;
    unsigned int jtoken_list_count;
} JObject;

extern JObject *JsonDeserialize(string jsonstr);

#endif