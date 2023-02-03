#ifndef _JSON_H_
#define _JSON_H_

#include<stdlib.h>
#include<string.h>

typedef char* string;
typedef unsigned char byte;
typedef enum {false = 0, true} bool;
typedef enum {STRING = 0, JSON_OBJ, ARRAY, INTEGER, DECIMAL, BOOLEAN} jsontype;

typedef struct {
    string key;
    void *value;
    jsontype value_type;
    unsigned int key_len;
    unsigned int value_byte_count;
} JToken;

typedef struct {
    JToken **tokens;
    unsigned int count;
    unsigned int size;
} TokenList;

typedef struct {
    TokenList *jtoken_list;
} JObject;

extern JObject *JsonDeserialize(string);
extern TokenList *GetTokenList(string);

#endif