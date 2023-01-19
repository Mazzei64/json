#include"json.h"

static bool istokenformat(string jsonstr, int len) {
    char buffer[len];
    bool toggle = false;
    int index = 0, marker = 0, i = 0;

    memset(buffer, 0x00, len);

    if(jsonstr[i] == '{') i++;
    for (; i < len; i++) {
        buffer[index] = jsonstr[i];
        index++;
    }
    // {"key":"valor"}
    index = 0;
    while (buffer[index] != '\0') {
        if(jsonstr[index] == '\"' && (marker == 0 || marker == 1))
            marker++;
        // após segunda áspas espera-se :
        else if(jsonstr[index] == ':' && marker == 2) marker++;

        else if((jsonstr[index] == '\"' || 
                 jsonstr[index] == '{' ||
                 jsonstr[index] == '[') && marker == 3) marker++;

        else if((jsonstr[index] == '\"' || 
                 jsonstr[index] == '}' ||
                 jsonstr[index] == ']') && marker == 4) marker++;
        index++;
    }
    if(marker == 5) return true;
    return false;
}
JObject *JsonDeserialize(string jsonstr) {
    if(jsonstr[0] != '{') return NULL;
    int len = strlen(jsonstr);
    if(jsonstr[len - 1] != '}') return NULL;
    if(!istokenformat(jsonstr)){}
}