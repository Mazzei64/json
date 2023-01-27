#include"json.h"

#define TOKEN_KEY_FINAL 0x3a22
#define TOKEN_INIT 0x222c
#define ASCII_TRUE 0x65757274
#define ASCII_FALSE 0x736c6166
#define CHECKTK_OVERFLOW(count) if(count % DEFAULT_TOKEN_LEN == 0 && count > 0) {                  \
            token = (string)realloc(token, sizeof(char)*(DEFAULT_TOKEN_LEN+count));         \
        }

static const unsigned int DEFAULT_TOKEN_LEN = 16;

static bool istokenformat(string);
static int findkey(string, string, int *);
static void findvalue(string, string, int *, int);
static string extracttokenAt(const string __restrict__ , int *);
static void FreeTokenList(TokenList *);
static JToken *ParseJTokenFromString(string);
static TokenList *NewTokenList();
static void AppendTokenList(TokenList*, JToken*);

JObject *JsonDeserialize(string jsonstr) {
    TokenList *tokenlst;
    if(jsonstr[0] != '{') return NULL;
    int len = strlen(jsonstr);
    if(jsonstr[len - 1] != '}') return NULL;
    tokenlst = GetTokenList(jsonstr);
    // if(!istokenformat(jsonstr, len)){}
}

// ------------------------------------------------------------------


TokenList *GetTokenList(string jsonstr) {
    int index = 1, count = 0;
    string tk = NULL;
    JToken *token = NULL;
    TokenList *tklst = NewTokenList();
    while (jsonstr[index] != '\0') {
        if((tk = extracttokenAt(jsonstr, &index)) == NULL) {
            FreeTokenList(tklst);
            return NULL;
        }
        if(!istokenformat(tk)) {
            FreeTokenList(tklst);
            return NULL;
        }
        token = ParseJTokenFromString(tk);
        AppendTokenList(tklst, token);
    }
    return tklst;
}

// ------------------------------------------------------------------

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
                *((unsigned short*)(&jsonstr[index + 1])) != 0x663a) &&
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
            *index = *index + 1; 
            count++;
            while (true) {
                CHECKTK_OVERFLOW(count)
                if(jsonstr[*index] == '{' && !llevel) llevel = true;
                else if(jsonstr[*index] == '}' && llevel) llevel = false;
                else if(jsonstr[*index] == '}' && !llevel) {
                    token[count] = jsonstr[*index];
                    *index = *index + 1; 
                    count++;
                    break;
                }
                else if (jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    break;
                }
                token[count] = jsonstr[*index];
                *index = *index + 1; 
                count++;
            }
            break;
        }
        else if(jsonstr[*index] == '[') {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            *index = *index + 1; 
            count++;
            while (true) {
                CHECKTK_OVERFLOW(count)
                if(jsonstr[*index] == '[' && !llevel) llevel = true;
                else if(jsonstr[*index] == ']' && llevel) llevel = false;
                else if(jsonstr[*index] == ']' && !llevel) {
                    token[count] = jsonstr[*index];
                    *index = *index + 1; 
                    count++;
                    break;
                }
                else if (jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    break;
                }
                token[count] = jsonstr[*index];
                *index = *index + 1; 
                count++;
            }
            break;
        }
        else if(jsonstr[*index] == '\"') {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            *index = *index + 1; 
            count++;
            while (jsonstr[*index] != '\"') {
                CHECKTK_OVERFLOW(count)
                token[count] = jsonstr[*index];
                *index = *index + 1; 
                count++;
                if(jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    break;
                }
            }
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            *index = *index + 1; 
            count++;
            break;
        }
        else if(*((unsigned int*)&jsonstr[*index]) == ASCII_TRUE) {
                    CHECKTK_OVERFLOW(count)
                    token[count] = jsonstr[*index];
                    *index = *index + 1; 
                    count++;
                    *((unsigned int*)&jsonstr[*index]) = ASCII_TRUE;
                    *index = *index + 4;
                    count +=4;
                    CHECKTK_OVERFLOW(count)
                    break;
        }
        else if (*((unsigned int*)&jsonstr[*index]) == ASCII_FALSE) {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            *index = *index + 1; 
            count++;
            *((unsigned int*)&jsonstr[*index]) = ASCII_FALSE;
            *index = *index + 4;
            count +4;
            CHECKTK_OVERFLOW(count)
            break;
        }
        else if(jsonstr[*index] >= 0x30 && jsonstr[*index] <= 0x39) {
            CHECKTK_OVERFLOW(count)
            token[count] = jsonstr[*index];
            *index = *index + 1; 
            count++;
            while (jsonstr[*index] != ',' && jsonstr[*index] != '}') {
                CHECKTK_OVERFLOW(count)
                token[count] = jsonstr[*index];
                *index = *index + 1; 
                count++;
                if (jsonstr[*index] == '\0') {
                    token[count] = jsonstr[*index];
                    break;
                }
            }
            break;
        }
    }
    
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
    byte *valueBuffer = (byte*)calloc(tkstrLen, sizeof(char));
    memset(valueBuffer, 0x00, sizeof(byte)*tkstrLen);
    string _key = (string)calloc(16, sizeof(char));
    JToken *token = (JToken*)malloc(sizeof(JToken));
    while (tkstr[index] == '\"') {
        if((index - 1) % 16 == 0)
            _key = (string)realloc(_key, sizeof(char)*(16 + (index - 1)));

        _key[index - 1] = tkstr[index];
        index++;
    }
    token->key = _key;
    token->key_len = index;
    index = index + 2;
    if(tkstr[index] == '\"'){
        token->value_type = STRING;
        while (tkstr[index + 1] != '\0' && tkstr[index] == '\"') {
            valueBuffer[valIndex] = tkstr[index];
            index++;
            valIndex++;
        }    
    }
    else if(tkstr[index] == '{') {
        token->value_type = JSON_OBJ;
        while (tkstr[index + 1] != '\0' && tkstr[index] == '}') {
            valueBuffer[valIndex] = tkstr[index];
            index++;
            valIndex++;
        }
    }
    else if(tkstr[index] == '[') {
        token->value_type = ARRAY;
        while (tkstr[index + 1] != '\0' && tkstr[index] == ']') {
            valueBuffer[valIndex] = tkstr[index];
            index++;
            valIndex++;
        }
    }
    else if(tkstr[index] >= 0x30 || tkstr[index] <= 0x39) {
        while (tkstr[index] != '\0') {
            if(tkstr[index] == '.') isDecimal = true;
            valueBuffer[valIndex] = tkstr[index];
            index++;
            valIndex++;
        }
        if(!isDecimal) {
            token->value_type = INTEGER;
        }
        else
            token->value_type = DECIMAL;
    }
    else if(tkstr[index] == 't' || tkstr[index] == 'f') {
        token->value_type = BOOLEAN;
        while (tkstr[index] != '\0') {
            valueBuffer[valIndex] = tkstr[index];
            index++;
            valIndex++;
        }
    }
    token->value_byte_count = valIndex + 1;
    token->value = (void*)valueBuffer;
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
        tklst->size = tklst->size + tklst->size;
        tklst->tokens = (JToken**)realloc(tklst->tokens, sizeof(JToken*) * tklst->size);
    }
    tklst->tokens[tklst->count] = token;
}