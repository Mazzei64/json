#include"json.h"

#define TOKEN_KEY_FINAL 0x3a22
#define TOKEN_INIT 0x222c
#define ASCII_TRUE 0x65757274
#define ASCII_FALSE 0x736c6166
#define CHECKTK_OVERFLOW(count) if(count % DEFAULT_TOKEN_LEN == 0 && count > 0) {                  \
            token = (string)realloc(token, sizeof(char)*(DEFAULT_TOKEN_LEN+count));         \
        }

static const unsigned int DEFAULT_TOKEN_LEN = 16;

static void AddDataToJArray(JsonArray*, void*);
static JsonArray *CreateJArrayFromStream(byte*);
static bool istokenformat(string);
static int findkey(string, string, int *);
static void findvalue(string, string, int *, int);
static string filterJson(string, int*);
static string extracttokenAt(const string __restrict__ , int *);
static void FreeTokenList(TokenList *);
static JToken *ParseJTokenFromString(string);
static JsonArray *NewJsonArray();
static TokenList *NewTokenList();
static void AppendTokenList(TokenList*, JToken*);

/*
    This is the main Public deserializing function for the Json library.
    This is meant for passing a stringified Json object and return the reference for
    the JObject object instantiated in memory.
*/
JObject *JsonDeserialize(string jsonstr) {
    int jsonLen = 0, len = 0;
    string filteredJson;
    JObject* jsonObject;

    /*
        First filter the json string with the filterJson private method and return the filtered version or
        NULL in case of error.
    */
    if((filteredJson = filterJson(jsonstr, &len)) == NULL) return NULL;

    /*
        Instantiate the Object.         
    */
    jsonObject = (JObject*)calloc(1, sizeof(JObject));
    
    
    jsonObject->jtoken_list = GetTokenList(filteredJson);

    return jsonObject;
}

// ------------------------------------------------------------------

/*
    This is the 
*/

TokenList *GetTokenList(string jsonstr) {
    int index = 1, count = 0, debug = 0;
    string tk = NULL;
    JToken *token = NULL;
    TokenList *tklst = NewTokenList();
    while (jsonstr[index] != '\0') {
        if((tk = extracttokenAt(jsonstr, &index)) == NULL) {
            FreeTokenList(tklst);
            return NULL;
        }
        // if(!istokenformat(tk)) {
        //     FreeTokenList(tklst);
        //     return NULL;
        // }
        token = ParseJTokenFromString(tk);
        AppendTokenList(tklst, token);
        index++;
        debug++;
    }
    return tklst;
}

// ---------------------------- PRIVATE --------------------------------------

static void AddDataToJArray(JsonArray *array, void *data) {
    int index;
    byte initVal = ((byte*)data)[0];
    if(initVal == '\"') {
        index = 1;
        array->data_type[array->count] = STRING;
        while (((byte*)data)[index] != '\"') index++;
        array->data[array->count] = (unsigned long int)calloc(index - 1, sizeof(char));
        index = 1;
        while (((byte*)data)[index] != '\"') {
            ((string)array->data[array->count])[index - 1] = ((byte*)data)[index];
            index++;
        }
    }
    else if(initVal == '{') {
        array->data_type[array->count] = JSON_OBJ;
        array->data[array->count] = (size_t)JsonDeserialize((string)data);
    }
    else if (initVal == '[') {
        array->data_type[array->count] = ARRAY;
        array->data[array->count] = (size_t)CreateJArrayFromStream((byte*)data);
    }
    else if (initVal > 0x29 && initVal < 0x40) {
        index = 0;
        array->data_type[array->count] = INTEGER;
        array->data[array->count] = (size_t)calloc(32, sizeof(byte));
        while (((byte*)data)[index] != '\0') {
            if(index % 32 == 0)
                array->data[array->count] = (size_t)realloc((void*)array->data[array->count], sizeof(byte)*(index + 32));

            ((byte*)array->data[array->count])[index] = ((byte*)data)[index];
            index++;
        }
    }
    else if (initVal == 't' || initVal == 'f') {
        index = 0;
        array->data_type[array->count] = BOOLEAN;
        array->data[array->count] = initVal == 't' ? ASCII_TRUE : ASCII_FALSE;
    }

    array->count++;
    if(array->count == array->elements_size) {
        array->elements_size += 32;
        array->data = realloc(array->data, sizeof(size_t)*array->elements_size);
        array->data_type = (jsontype*)realloc(array->data, sizeof(jsontype)*array->elements_size);
    }
        
}
static JsonArray *CreateJArrayFromStream(byte* jastr) {
    int valIndex = 0;
    JsonArray* array = NewJsonArray();
    void *valueBuffer = (byte*)calloc(256, sizeof(char));
    int index = 1;
    while (*((unsigned short*)(&jastr[index])) != 0x5d ) { 
        if((valIndex + 1) % 256 == 0)
            valueBuffer = (byte*)realloc(valueBuffer, sizeof(char)*((valIndex + 1) + 256));
        
        ((byte*)valueBuffer)[valIndex] = jastr[index];
        valIndex++;
        index++;
        if(jastr[index] == ',' || *((unsigned short*)(&jastr[index])) != 0x5d) {
            AddDataToJArray(array, valueBuffer);
            memset(valueBuffer, 0x00, valIndex+1);
            valIndex = 0;
        }
    }
}

static bool istokenformat(string jsonstr) {
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
                *((unsigned short*)(&jsonstr[index + 1])) != 0x663a &&
                *((unsigned short*)(&jsonstr[index + 1])) != 0x2d3a) &&
                (jsonstr[index + 2] < 0x30 || jsonstr[index + 2] > 0x39)) marker = -1;
            else {
                marker++;
                index++;
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
        else if (((jsonstr[index] >= 0x30 && jsonstr[index] <= 0x39) || jsonstr[index] == '-') && marker == 2){
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


static int findkey(string jsonstr, string token, int *index){
    bool flag = true;
    int count = 0;
    while (flag) {
        CHECKTK_OVERFLOW(count)
        if(*((unsigned short*)(&jsonstr[*index])) == TOKEN_KEY_FINAL){
            token[count] = jsonstr[*index];
            count++;
            *index = *index + 1;
            flag = false;
        }
        token[count] = jsonstr[*index];
        count++;
        *index = *index + 1;
    }
    flag = true;
    return count;
}

static void findvalue(string jsonstr, string token, int *index, int count) {
    bool flag = true, llevel = false;
    int marker = 1;
    while (flag) {
        if(jsonstr[*index] == '{') {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            token[count + 1] = '\0';
            *index = *index + 1; 
            count++;
            while (true) {
                CHECKTK_OVERFLOW(count)
                if(jsonstr[*index] == '{' && !llevel) llevel = true;
                else if(jsonstr[*index] == '}' && llevel) llevel = false;
                else if(jsonstr[*index] == '}' && !llevel) {
                    token[count] = jsonstr[*index];
                    token[count + 1] = '\0';
                    *index = *index + 1; 
                    count++;
                    break;
                }
                else if (jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    token[count + 1] = '\0';
                    break;
                }
                token[count] = jsonstr[*index];
                token[count + 1] = '\0';
                *index = *index + 1; 
                count++;
            }
            break;
        }
        else if(jsonstr[*index] == '[') {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            token[count + 1] = '\0';
            *index = *index + 1; 
            count++;
            while (true) {
                CHECKTK_OVERFLOW(count)
                if(jsonstr[*index - 1] == '[' && !llevel) llevel = true;
                else if(jsonstr[*index] == ']' && llevel) {
                    llevel = false;
                    continue;
                }
                else if(jsonstr[*index] == ']' && !llevel) {
                    token[count] = jsonstr[*index];
                    token[count + 1] = '\0';
                    *index = *index + 1; 
                    count++;
                    break;
                }
                else if (jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    token[count + 1] = '\0';
                    break;
                }
                token[count] = jsonstr[*index];
                token[count + 1] = '\0';
                *index = *index + 1; 
                count++;
            }
            break;
        }
        else if(jsonstr[*index] == '\"') {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            token[count + 1] = '\0';
            *index = *index + 1; 
            count++;
            while (jsonstr[*index] != '\"') {
                CHECKTK_OVERFLOW(count)
                token[count] = jsonstr[*index];
                token[count + 1] = '\0';
                *index = *index + 1; 
                count++;
                if(jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    token[count + 1] = '\0';
                    break;
                }
            }
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            token[count + 1] = '\0';
            *index = *index + 1; 
            count++;
            break;
        }
        else if(*((unsigned int*)&jsonstr[*index]) == ASCII_TRUE) {
                    CHECKTK_OVERFLOW(count)
                    *((unsigned int*)&token[count]) = ASCII_TRUE;
                    count += 4;
                    token[count + 1] = '\0';
                    *index = *index + 4;
                    CHECKTK_OVERFLOW(count)
                    break;
        }
        else if (*((unsigned int*)&jsonstr[*index]) == ASCII_FALSE) {
            CHECKTK_OVERFLOW(count)
            *((unsigned int*)&token[count]) = ASCII_FALSE;
            count += 4;
            token[count] = 'e';
            token[count + 1] = '\0';
            *index = *index + 5;
            CHECKTK_OVERFLOW(count)
            break;
        }
        else if((jsonstr[*index] >= 0x30 && jsonstr[*index] <= 0x39) ||
                 (jsonstr[*index] == '-' && (jsonstr[*index + 1] >= 0x30 && jsonstr[*index + 1] <= 0x39))) {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            token[count + 1] = '\0';
            *index = *index + 1; 
            count++;
            while (jsonstr[*index] != ',' && jsonstr[*index] != '}') {
                CHECKTK_OVERFLOW(count)
                token[count] = jsonstr[*index];
                token[count + 1] = '\0';
                *index = *index + 1; 
                count++;
                if (jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    token[count + 1] = '\0';
                    break;
                }
            }
            break;
        }
    }
    
}

static string filterJson(string jsonStr, int* len) {
    int filteredIndex = 0;
    bool llevel = false;
    *len = strlen(jsonStr);

    if(jsonStr[0] != '{' && jsonStr[*len -1] != '}' && jsonStr[0] != '[' && jsonStr[*len -1] != ']') return NULL;

    string filteredJson = (string)calloc(*len,sizeof(char));
    for (int i = 0; i < *len; i++){  
        if(jsonStr[i] == '\"' && !llevel) {
            llevel = true;
            filteredJson[filteredIndex] = jsonStr[i];
            filteredIndex++;
            continue;
        }
        if(jsonStr[i] == '\"' && llevel) {
            llevel = false;
            filteredJson[filteredIndex] = jsonStr[i];
            filteredIndex++;
            continue;
        }
        if((jsonStr[i] == ' ' || jsonStr[i] == '\n' ||
            jsonStr[i] == '\t' || jsonStr[i] == '\r' ||
            jsonStr[i] == '\b') && !llevel) continue;
        filteredJson[filteredIndex] = jsonStr[i];
        filteredIndex++;
    }
    return filteredJson;
}

static string extracttokenAt(const string __restrict__ jsonstr, int *index) {
    bool flag = true;
    int count = 0;
    int debug = strlen(jsonstr);
    string tk = (string)calloc(16,sizeof(char));

    if(jsonstr[*index] == '{') *index = *index + 1;

    count = findkey(jsonstr, tk, index);

    findvalue(jsonstr, tk, index, count);
    
    return tk;
}
static void FreeTokenList(TokenList *tklst){
    free(tklst->tokens);
    tklst->count = 0;
    tklst->size = 0;
    free(tklst);
}
static JToken *ParseJTokenFromString(string tkstr) {
    int index = 1, valIndex = 0, tkstrLen = strlen(tkstr);
    bool isDecimal = false;
    void *valueBuffer = (byte*)calloc(tkstrLen, sizeof(char));
    string _key = (string)calloc(16, sizeof(char));
    JToken *token = (JToken*)malloc(sizeof(JToken));
    while (tkstr[index] != '\"') {
        if(index % 16 == 0)
            _key = (string)realloc(_key, sizeof(char)*(index + 16));

        _key[valIndex] = tkstr[index];
        index++;
        valIndex++;
    }
    token->key = _key;
    token->key_len = index - 1;
    index += 2;
    valIndex = 0;

    if(tkstr[index] == '\"'){
        token->value_type = STRING;
        index++;
        while (*((unsigned short*)(&tkstr[index])) != 0x22 ) { 
            if((valIndex + 1) % tkstrLen == 0)
                valueBuffer = (byte*)realloc(valueBuffer, sizeof(char)*((valIndex + 1) + tkstrLen));
            
            ((byte*)valueBuffer)[valIndex] = tkstr[index];
            valIndex++;
            index++;
        }
        
    }
    else if(tkstr[index] == '{'){
        JObject* addressJobj = NULL;
        token->value_type = JSON_OBJ;
        while (*((unsigned short*)(&tkstr[index])) != 0x7d ) { 
            if((valIndex + 1) % tkstrLen == 0)
                valueBuffer = (byte*)realloc(valueBuffer, sizeof(char)*((valIndex + 1) + tkstrLen));
            
            ((byte*)valueBuffer)[valIndex] = tkstr[index];
            valIndex++;
            index++;
        }
        ((byte*)valueBuffer)[valIndex] = tkstr[index];
        addressJobj = JsonDeserialize((string)valueBuffer);
        free((string)valueBuffer);
        valueBuffer = addressJobj;
    }
    /*
        !BUG: Array parsing isn't treating each comma separated element as a different type and recursively instantiating new objects.

    */
    else if(tkstr[index] == '['){
        token->value_type = ARRAY;
        JsonArray *array = NewJsonArray();
        index++;
        while (*((unsigned short*)(&tkstr[index])) != 0x5d ) { 
            if((valIndex + 1) % tkstrLen == 0)
                valueBuffer = (byte*)realloc(valueBuffer, sizeof(char)*((valIndex + 1) + tkstrLen));
            
            ((byte*)valueBuffer)[valIndex] = tkstr[index];
            valIndex++;
            index++;
            if(tkstr[index] == ',' || *((unsigned short*)(&tkstr[index])) == 0x5d) {
                AddDataToJArray(array, valueBuffer);
                memset(valueBuffer, 0x00, valIndex+1);
                valIndex = 0;
                if(tkstr[index] == ',') index++;
            }
        }
        free((string)valueBuffer);
        valueBuffer = array;
    }
    else if((tkstr[index] >= 0x30 && tkstr[index] <= 0x39) ||
             (tkstr[index] == '-' && (tkstr[index + 1] >= 0x30 && tkstr[index + 1] <= 0x39))){
        token->value_type = INTEGER;
        while (tkstr[index] != '\0' ) { // '\0' + ']'
            if((valIndex + 1) % tkstrLen == 0)
                valueBuffer = (byte*)realloc(valueBuffer, sizeof(char)*((valIndex + 1) + tkstrLen));
            
            ((byte*)valueBuffer)[valIndex] = tkstr[index];
            valIndex++;
            index++;
        }
    }
    else if(tkstr[index] == 't' || tkstr[index] == 'f'){
        token->value_type = BOOLEAN;
        while (tkstr[index] != '\0' ) { // '\0' + ']'
            if((valIndex + 1) % tkstrLen == 0)
                valueBuffer = (byte*)realloc(valueBuffer, sizeof(char)*((valIndex + 1) + tkstrLen));
            
            ((byte*)valueBuffer)[valIndex] = tkstr[index];
            valIndex++;
            index++;
        }
    }

    token->value = valueBuffer;
    token->value_byte_count = valIndex + 1;

    return token;
    
}
static JsonArray *NewJsonArray() {
    JsonArray* array = (JsonArray*)calloc(1, sizeof(JsonArray));
    array->elements_size = 32;
    array->data = calloc(array->elements_size, sizeof(size_t));
    array->data_type = (jsontype*)calloc(array->elements_size, sizeof(jsontype));
    array->count = 0;
    return array;
}
static TokenList *NewTokenList() {
    static const unsigned char tokenListInitialSize = 16;
    TokenList *tklst = (TokenList*)calloc(1, sizeof(TokenList));
    tklst->tokens = (JToken**)calloc(tokenListInitialSize, sizeof(JToken*));
    tklst->count = 0;
    tklst->size = 16;
}
static void AppendTokenList(TokenList *tklst, JToken *token) {
    if(tklst->count == tklst->size) {
        tklst->size = tklst->size + 16;
        tklst->tokens = (JToken**)realloc(tklst->tokens, sizeof(JToken*) * (tklst->size + 16));
    }
    tklst->tokens[tklst->count] = token;
    tklst->count++;
}