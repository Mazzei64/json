#include"json.h"

#define TOKEN_KEY_FINAL 0x3a22
#define ASCII_TRUE 0x65757274
#define ASCII_FALSE 0x736c6166

static bool istokenformat(string jsonstr, int len) {
    bool eol;
    bool symbolsFound[3] = {false, false, false};
    int index = 0, marker = 0;
    while (jsonstr[index] != '\0') {
        eol = jsonstr[index + 1] == '\0' && marker == 3;
        // key section
        if(jsonstr[index] == '\"' && marker == 0)
            marker++;
        else if(*((unsigned short*)(&jsonstr[index])) == TOKEN_KEY_FINAL && marker == 1) {
            if((*((unsigned short*)(&jsonstr[index + 1])) != 0x7b3a &&
               *((unsigned short*)(&jsonstr[index + 1])) != 0x5b3a &&
               *((unsigned short*)(&jsonstr[index + 1])) != 0x223a &&
               *((unsigned short*)(&jsonstr[index + 1])) != 0x743a &&
               *((unsigned short*)(&jsonstr[index + 1])) != 0x663a) &&
               (jsonstr[index + 2] < 0x30 || jsonstr[index + 2] > 0x39)) marker = -1;
            else {
                marker++;
                index ++;
            }
        }
        // value section
        else if(jsonstr[index] == '\"' && marker == 2){
            marker++;
            symbolsFound[0] = true;
        }
        else if(jsonstr[index] == '{' && marker == 2){
            marker++;
            symbolsFound[1] = true;
        }
        else if(jsonstr[index] == '[' && marker == 2){
            marker++;
            symbolsFound[2] = true;
        }
        else if (jsonstr[index] >= 0x30 && jsonstr[index] <= 0x39 && marker == 2){
            index++;
            while (jsonstr[index] != '\0') {
                if((jsonstr[index] < 0x30 || jsonstr[index] > 0x39) && jsonstr[index] != '.') marker = -1;
                index++;
            }
            if(marker == 2) marker = 4;
            index--;
        }
        else if(*((unsigned int*)(&jsonstr[index])) == ASCII_TRUE && marker == 2){
            if(jsonstr[index + sizeof(unsigned int)] == '\0') {
                index += sizeof(unsigned int) - 1;
                marker = 4;
            }
            else
                marker = -1;

        }
        else if(*((unsigned int*)(&jsonstr[index])) == ASCII_FALSE && marker == 2){
            if(jsonstr[index + sizeof(unsigned int)] != 'e') marker = -1;
            else if(jsonstr[index + sizeof(unsigned int) + 1] != '\0') marker = -1;
            else {
                index += sizeof(unsigned int);
                marker = 4;
            }
        }
        else if(jsonstr[index] == '\"' && symbolsFound[0] && eol) marker++;
        else if(jsonstr[index] == '}' && symbolsFound[1] && eol) marker++;
        else if(jsonstr[index] == ']' && symbolsFound[2] && eol) marker++;

        index++;
    }
    if(marker == 4) return true;
    return false;
}
JObject *JsonDeserialize(string jsonstr) {
    if(jsonstr[0] != '{') return NULL;
    int len = strlen(jsonstr);
    if(jsonstr[len - 1] != '}') return NULL;
    if(!istokenformat(jsonstr, len)){}
}