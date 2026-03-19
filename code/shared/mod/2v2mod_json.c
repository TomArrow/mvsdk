#include "2v2mod_json.h"
#include "../../game/q_shared.h"
#include "2v2mod_memory.h"
#include "2v2mod_utils.h"

typedef struct JSONState_s {
    const char      *data;
    const char      *dataEnd;
    const char      *pos;
    JSONErrorInfo_t *error;
} JSONState_t;

static JSONState_t state;

#define JSON_ERROR_CONTEXT_WINDOW 20

static JSON_t  *TvT_JSON_CreateValue(void);
static void     TvT_JSON_SetError(JSONError_t errorType, const char *expected, const char *found);
static void     TvT_JSON_AppendChild(JSON_t *parent, JSON_t *item);
static qboolean TvT_JSON_ParseValue(JSON_t *value);
static qboolean TvT_JSON_ParseNumber(JSON_t *value);
static qboolean TvT_JSON_ParseString(JSON_t *value);
static qboolean TvT_JSON_ParseArray(JSON_t *value);
static qboolean TvT_JSON_ParseObject(JSON_t *value);
static qboolean TvT_JSON_WriteValue(JSON_t *value, char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty);
static int      TvT_JSON_EscapedLength(const char *str, int len);
static char    *TvT_JSON_EscapeString(const char *str, int len, int *newLen);
static int      TvT_JSON_AppendToBuffer(char **buffer, int *bufferSize, int *currentPos, const char *str, int len);
static qboolean TvT_JSON_WriteString(JSON_t *value, char **buffer, int *bufferSize, int *currentPos);
static qboolean TvT_JSON_WriteNumber(JSON_t *value, char **buffer, int *bufferSize, int *currentPos);
static qboolean TvT_JSON_WriteArray(JSON_t *value, char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty);
static qboolean TvT_JSON_WriteObject(JSON_t *value, char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty);
static qboolean TvT_JSON_AppendIndent(char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty);
static qboolean TvT_JSON_AppendNewline(char **buffer, int *bufferSize, int *currentPos, qboolean pretty);

static const char *TvT_JSON_GetErrorString(JSONError_t error) {
    switch (error) {
        case JSON_ERROR_NONE:
            return "No error";
        case JSON_ERROR_INVALID_INPUT:
            return "Invalid input";
        case JSON_ERROR_UNEXPECTED_EOF:
            return "Unexpected end of file";
        case JSON_ERROR_UNEXPECTED_CHARACTER:
            return "Unexpected character";
        case JSON_ERROR_INVALID_NUMBER:
            return "Invalid number format";
        case JSON_ERROR_INVALID_STRING:
            return "Invalid string format";
        case JSON_ERROR_INVALID_ESCAPE:
            return "Invalid escape sequence";
        case JSON_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case JSON_ERROR_MISSING_COLON:
            return "Missing colon after key";
        case JSON_ERROR_MISSING_COMMA:
            return "Missing comma between items";
        case JSON_ERROR_MISSING_QUOTE:
            return "Missing quote";
        case JSON_ERROR_MISSING_BRACKET:
            return "Missing bracket";
        case JSON_ERROR_DUPLICATE_KEY:
            return "Duplicate key in object";
        default:
            return "Unknown error";
    }
}

void TvT_JSON_PrintError(const JSONErrorInfo_t *error) {
    if (!error || error->type == JSON_ERROR_NONE) {
        return;
    }

    Com_Printf("^1JSON Parse Error: %s^7\n", TvT_JSON_GetErrorString(error->type));

    if (error->context[0]) {
        Com_Printf("^3Context: %s^7\n", error->context);
    }

    if (error->expected[0] && error->found[0]) {
        Com_Printf("^3Expected: '%s', Found: '%s'^7\n", error->expected, error->found);
    }
    else if (error->expected[0]) {
        Com_Printf("^3Expected: '%s'^7\n", error->expected);
    }
    else if (error->found[0]) {
        Com_Printf("^3Found: '%s'^7\n", error->found);
    }
}

static void TvT_JSON_SetError(JSONError_t errorType, const char *expected, const char *found) {
    JSONErrorInfo_t *error = state.error;
    const char      *contextStart, *contextEnd;
    int              contextLen;
    int              currentPos;

    if (!error) {
        return;
    }

    error->type = errorType;

    if (state.data && state.pos && state.dataEnd > state.data) {
        currentPos = state.pos - state.data;
        contextStart = (currentPos >= JSON_ERROR_CONTEXT_WINDOW) ? state.pos - JSON_ERROR_CONTEXT_WINDOW : state.data;
        contextEnd = state.pos + JSON_ERROR_CONTEXT_WINDOW;
        if (contextEnd > state.dataEnd) {
            contextEnd = state.dataEnd;
        }

        contextLen = contextEnd - contextStart;
        if (contextLen >= sizeof(error->context)) {
            contextLen = sizeof(error->context) - 1;
        }

        if (contextLen > 0) {
            Q_strncpyz(error->context, contextStart, contextLen + 1);
        }
        else {
            error->context[0] = '\0';
        }
    }
    else {
        error->context[0] = '\0';
    }

    if (expected) {
        Q_strncpyz(error->expected, expected, sizeof(error->expected));
    }
    else {
        error->expected[0] = '\0';
    }

    if (found) {
        Q_strncpyz(error->found, found, sizeof(error->found));
    }
    else {
        error->found[0] = '\0';
    }
}

static void TvT_JSON_AppendChild(JSON_t *parent, JSON_t *item) {
    if (!parent->child) {
        parent->child = item;
        parent->tail = item;
    }
    else {
        parent->tail->next = item;
        parent->tail = item;
    }
}

static JSON_t *TvT_JSON_CreateValue(void) {
    JSON_t *item;
    item = calloc(1, sizeof(JSON_t));
    if (!item) {
        TvT_JSON_SetError(JSON_ERROR_MEMORY_ALLOCATION, "memory allocation", "out of memory");
        return NULL;
    }
    return item;
}

void TvT_JSON_FreeValue(JSON_t *value) {
    JSON_t *next = NULL;
    while (value) {
        next = value->next;
        if (value->key) {
            free(value->key);
        }

        if (value->type == TYPE_OBJECT || value->type == TYPE_ARRAY) {
            TvT_JSON_FreeValue(value->child);
        }
        else if (value->type == TYPE_STRING && value->value.str) {
            free(value->value.str);
        }

        free(value);
        value = next;
    }
}

JSON_t *TvT_JSON_CreateObject(void) {
    JSON_t *item = TvT_JSON_CreateValue();
    if (item) {
        item->type = TYPE_OBJECT;
    }
    return item;
}

JSON_t *TvT_JSON_CreateArray(void) {
    JSON_t *item = TvT_JSON_CreateValue();
    if (item) {
        item->type = TYPE_ARRAY;
    }
    return item;
}

JSON_t *TvT_JSON_CreateString(const char *string) {
    JSON_t *item;

    if (!string) {
        return NULL;
    }

    item = TvT_JSON_CreateValue();
    if (item) {
        item->type = TYPE_STRING;
        item->valueLen = strlen(string);
        item->value.str = malloc(item->valueLen + 1);
        if (item->value.str) {
            memcpy(item->value.str, string, item->valueLen + 1);
        }
        else {
            TvT_JSON_FreeValue(item);
            return NULL;
        }
    }
    return item;
}

JSON_t *TvT_JSON_CreateNumber(int number) {
    JSON_t *item = TvT_JSON_CreateValue();
    if (item) {
        item->type = TYPE_INT;
        item->value.numberInt = number;
    }
    return item;
}

JSON_t *TvT_JSON_CreateFloat(float number) {
    JSON_t *item = TvT_JSON_CreateValue();
    if (item) {
        item->type = TYPE_FLOAT;
        item->value.numberFloat = number;
    }
    return item;
}

JSON_t *TvT_JSON_CreateBool(qboolean boolean) {
    JSON_t *item = TvT_JSON_CreateValue();
    if (item) {
        item->type = TYPE_BOOL;
        item->value.boolean = boolean;
    }
    return item;
}

JSON_t *TvT_JSON_CreateNull(void) {
    JSON_t *item = TvT_JSON_CreateValue();
    if (item) {
        item->type = TYPE_NULL;
    }
    return item;
}

qboolean TvT_JSON_AddItemToObject(JSON_t *object, const char *key, JSON_t *item) {
    if (!object || !key || object->type != TYPE_OBJECT) {
        TvT_JSON_SetError(JSON_ERROR_INVALID_INPUT, "valid object and key", "null object or key");
        return qfalse;
    }

    if (!item) {
        TvT_JSON_SetError(JSON_ERROR_INVALID_INPUT, "valid item", "null item (creation may have failed)");
        return qfalse;
    }

    if (item->key) {
        free(item->key);
    }

    item->keyLen = strlen(key);
    item->key = malloc(item->keyLen + 1);
    if (!item->key) {
        TvT_JSON_SetError(JSON_ERROR_MEMORY_ALLOCATION, "memory allocation", "failed to allocate key");
        return qfalse;
    }
    memcpy(item->key, key, item->keyLen + 1);

    TvT_JSON_AppendChild(object, item);

    return qtrue;
}

qboolean TvT_JSON_AddItemToArray(JSON_t *array, JSON_t *item) {
    if (!array || array->type != TYPE_ARRAY) {
        TvT_JSON_SetError(JSON_ERROR_INVALID_INPUT, "valid array", "null or non-array object");
        return qfalse;
    }

    if (!item) {
        TvT_JSON_SetError(JSON_ERROR_INVALID_INPUT, "valid item", "null item (creation may have failed)");
        return qfalse;
    }

    TvT_JSON_AppendChild(array, item);

    return qtrue;
}

JSON_t *TvT_JSON_GetObjectItem(JSON_t *object, const char *key) {
    JSON_t *child;

    if (!object || object->type != TYPE_OBJECT || !key) {
        return NULL;
    }

    child = object->child;
    while (child) {
        if (child->key && !Q_stricmp(child->key, key)) {
            return child;
        }
        child = child->next;
    }

    return NULL;
}

JSON_t *TvT_JSON_GetArrayItem(JSON_t *array, int index) {
    JSON_t *child;
    int     i;

    if (!array || array->type != TYPE_ARRAY || index < 0) {
        return NULL;
    }

    child = array->child;
    i = 0;

    while (child && i < index) {
        child = child->next;
        i++;
    }

    return child;
}

static qboolean TvT_JSON_ParseNumber(JSON_t *value) {
    const char *currentPos;
    qboolean    isFloat = qfalse;
    char        numBuf[32];
    int         numLen;

    currentPos = state.pos;

    if (*currentPos == '-') {
        currentPos++;
        if (currentPos >= state.dataEnd) {
            TvT_JSON_SetError(JSON_ERROR_INVALID_NUMBER, "valid number", "no digits after sign");
            return qfalse;
        }
    }

    if (*currentPos < '0' || *currentPos > '9') {
        TvT_JSON_SetError(JSON_ERROR_INVALID_NUMBER, "valid number", "no digits after sign");
        return qfalse;
    }

    // Reject leading zeros (e.g. 007) but allow 0 and 0.x.
    if (*currentPos == '0' && (currentPos + 1) < state.dataEnd) {
        if (*(currentPos + 1) >= '0' && *(currentPos + 1) <= '9') {
            TvT_JSON_SetError(JSON_ERROR_INVALID_NUMBER, "valid number", "leading zeros not allowed");
            return qfalse;
        }
    }

    while (currentPos < state.dataEnd && *currentPos) {
        if (*currentPos == '.') {
            if (isFloat) {
                TvT_JSON_SetError(JSON_ERROR_INVALID_NUMBER, "valid number", "multiple decimal points");
                return qfalse;
            }
            isFloat = qtrue;
        }
        else if (*currentPos < '0' || *currentPos > '9') {
            break;
        }

        currentPos++;
    }

    // Reject trailing decimal point (e.g. "1.").
    if (isFloat && *(currentPos - 1) == '.') {
        TvT_JSON_SetError(JSON_ERROR_INVALID_NUMBER, "digit after decimal point", "no digits after decimal point");
        return qfalse;
    }

    numLen = currentPos - state.pos;
    if (numLen >= (int)sizeof(numBuf)) {
        TvT_JSON_SetError(JSON_ERROR_INVALID_NUMBER, "reasonable number", "number too long");
        return qfalse;
    }
    memcpy(numBuf, state.pos, numLen);
    numBuf[numLen] = '\0';

    if (isFloat) {
        value->type = TYPE_FLOAT;
        value->value.numberFloat = atof(numBuf);
    }
    else {
        value->type = TYPE_INT;
        value->value.numberInt = atoi(numBuf);
    }

    state.pos = currentPos;

    return qtrue;
}

static qboolean TvT_JSON_ParseString(JSON_t *value) {
    const char *currentPos;
    char       *strValue = NULL;
    int         len = 0, i;

    if (*state.pos != '\"') {
        TvT_JSON_SetError(JSON_ERROR_MISSING_QUOTE, "\"", state.pos);
        return qfalse;
    }

    state.pos++;
    if (state.pos >= state.dataEnd) {
        TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "more JSON data", "end of file");
        return qfalse;
    }
    currentPos = state.pos;

    while (currentPos < state.dataEnd && *currentPos != '\"') {
        if (*currentPos == '\\') {
            if ((currentPos + 1) >= state.dataEnd) {
                TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "escape character", "end of file");
                return qfalse;
            }

            currentPos += 2;
            len++;
            continue;
        }

        if (*currentPos < ' ') {
            TvT_JSON_SetError(JSON_ERROR_INVALID_STRING, "valid string character", "unescaped control character");
            return qfalse;
        }

        len++;
        currentPos++;
    }

    if (currentPos >= state.dataEnd || *currentPos != '\"') {
        TvT_JSON_SetError(JSON_ERROR_MISSING_QUOTE, "closing quote", "end of file");
        return qfalse;
    }

    strValue = malloc(len + 1);

    if (!strValue) {
        TvT_JSON_SetError(JSON_ERROR_MEMORY_ALLOCATION, "memory", "allocation failed");
        return qfalse;
    }

    currentPos = state.pos;
    i = 0;
    while (currentPos < state.dataEnd && *currentPos != '\"') {
        if (*currentPos == '\\') {
            switch (*(currentPos + 1)) {
                case 'n':
                    strValue[i++] = '\n';
                    break;
                case 'r':
                    strValue[i++] = '\r';
                    break;
                case 't':
                    strValue[i++] = '\t';
                    break;
                case 'f':
                    strValue[i++] = '\f';
                    break;
                case 'b':
                    strValue[i++] = '\b';
                    break;
                case '\"':
                case '\\':
                case '/':
                    strValue[i++] = *(currentPos + 1);
                    break;
                default:
                    free(strValue);
                    TvT_JSON_SetError(JSON_ERROR_INVALID_ESCAPE, "valid escape sequence", currentPos);
                    return qfalse;
            }
            currentPos += 2;
            continue;
        }
        strValue[i++] = *currentPos;
        currentPos++;
    }

    if (i != len) {
        free(strValue);
        TvT_JSON_SetError(JSON_ERROR_INVALID_STRING, "consistent string length", "length mismatch");
        return qfalse;
    }

    strValue[i] = '\0';

    value->type = TYPE_STRING;
    value->value.str = strValue;
    value->valueLen = i;

    state.pos = ++currentPos;

    return qtrue;
}

static qboolean TvT_JSON_ParseArray(JSON_t *value) {
    if (*state.pos != '[') {
        TvT_JSON_SetError(JSON_ERROR_MISSING_BRACKET, "[", state.pos);
        return qfalse;
    }

    state.pos++;
    if (state.pos >= state.dataEnd) {
        TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "more JSON data", "end of file");
        return qfalse;
    }

    if (*state.pos == ']') {
        state.pos++;
        value->type = TYPE_ARRAY;
        value->child = NULL;
        value->tail = NULL;
        return qtrue;
    }

    value->type = TYPE_ARRAY;

    while (1) {
        JSON_t *newValue = TvT_JSON_CreateValue();

        if (!newValue) {
            TvT_JSON_FreeValue(value->child);
            value->child = NULL;
            return qfalse;
        }

        TvT_JSON_AppendChild(value, newValue);

        if (!TvT_JSON_ParseValue(newValue)) {
            TvT_JSON_FreeValue(value->child);
            value->child = NULL;
            return qfalse;
        }

        if (*state.pos && *state.pos == ',') {
            state.pos++;
            if (state.pos >= state.dataEnd) {
                TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "more JSON data", "end of file");
                TvT_JSON_FreeValue(value->child);
                value->child = NULL;
                return qfalse;
            }
            continue;
        }

        break;
    }

    if (*state.pos != ']') {
        TvT_JSON_SetError(JSON_ERROR_MISSING_COMMA, "',' or ']'", state.pos);
        TvT_JSON_FreeValue(value->child);
        value->child = NULL;
        return qfalse;
    }

    state.pos++;

    return qtrue;
}

static qboolean TvT_JSON_ParseObject(JSON_t *value) {
    if (*state.pos != '{') {
        TvT_JSON_SetError(JSON_ERROR_MISSING_BRACKET, "{", state.pos);
        return qfalse;
    }

    state.pos++;
    if (state.pos >= state.dataEnd) {
        TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "more JSON data", "end of file");
        return qfalse;
    }

    if (*state.pos == '}') {
        state.pos++;
        value->type = TYPE_OBJECT;
        value->child = NULL;
        value->tail = NULL;
        return qtrue;
    }

    value->type = TYPE_OBJECT;

    while (1) {
        JSON_t *newValue = TvT_JSON_CreateValue();

        if (!newValue) {
            TvT_JSON_FreeValue(value->child);
            value->child = NULL;
            return qfalse;
        }

        TvT_JSON_AppendChild(value, newValue);

        if (!TvT_JSON_ParseString(newValue)) {
            TvT_JSON_FreeValue(value->child);
            value->child = NULL;
            return qfalse;
        }

        newValue->key = newValue->value.str;
        newValue->value.str = NULL;
        newValue->keyLen = newValue->valueLen;

        if (!*state.pos || *state.pos != ':') {
            if (!*state.pos) {
                TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, ":", "end of file");
            }
            else {
                TvT_JSON_SetError(JSON_ERROR_MISSING_COLON, ":", state.pos);
            }
            TvT_JSON_FreeValue(value->child);
            value->child = NULL;
            return qfalse;
        }

        state.pos++;
        if (state.pos >= state.dataEnd) {
            TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "more JSON data", "end of file");
            TvT_JSON_FreeValue(value->child);
            value->child = NULL;
            return qfalse;
        }

        if (!TvT_JSON_ParseValue(newValue)) {
            TvT_JSON_FreeValue(value->child);
            value->child = NULL;
            return qfalse;
        }

        if (*state.pos && *state.pos == ',') {
            state.pos++;
            if (state.pos >= state.dataEnd) {
                TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "more JSON data", "end of file");
                TvT_JSON_FreeValue(value->child);
                value->child = NULL;
                return qfalse;
            }
            continue;
        }

        break;
    }

    if (*state.pos != '}') {
        TvT_JSON_SetError(JSON_ERROR_MISSING_COMMA, "',' or '}'", state.pos);
        TvT_JSON_FreeValue(value->child);
        value->child = NULL;
        return qfalse;
    }

    state.pos++;

    return qtrue;
}

static qboolean TvT_JSON_ParseValue(JSON_t *value) {
    int remaining;

    if (state.pos >= state.dataEnd) {
        TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_EOF, "more JSON data", "end of file");
        return qfalse;
    }

    switch (*state.pos) {
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return TvT_JSON_ParseNumber(value);
        case '\"':
            return TvT_JSON_ParseString(value);
        case '[':
            return TvT_JSON_ParseArray(value);
        case '{':
            return TvT_JSON_ParseObject(value);
    }

    remaining = state.dataEnd - state.pos;

    if (remaining >= 4 && !Q_strncmp(state.pos, "true", 4)) {
        value->type = TYPE_BOOL;
        value->value.boolean = qtrue;
        state.pos += 4;
        return qtrue;
    }

    if (remaining >= 5 && !Q_strncmp(state.pos, "false", 5)) {
        value->type = TYPE_BOOL;
        value->value.boolean = qfalse;
        state.pos += 5;
        return qtrue;
    }

    if (remaining >= 4 && !Q_strncmp(state.pos, "null", 4)) {
        value->type = TYPE_NULL;
        state.pos += 4;
        return qtrue;
    }

    TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_CHARACTER, "valid JSON value", state.pos);
    return qfalse;
}

JSON_t *TvT_JSON_Deserialize(char *data, int length, JSONErrorInfo_t *error) {
    JSON_t *root;
    int     cleanLength;

    if (error) {
        memset(error, 0, sizeof(JSONErrorInfo_t));
        error->type = JSON_ERROR_NONE;
    }

    if (!data || !*data || length <= 0) {
        if (error) {
            error->type = JSON_ERROR_INVALID_INPUT;
            Q_strncpyz(error->expected, "valid JSON string", sizeof(error->expected));
            Q_strncpyz(error->found, "null or empty input", sizeof(error->found));
        }
        return NULL;
    }

    cleanLength = TvT_RemoveWhitespace(data);

    memset(&state, 0, sizeof(JSONState_t));

    state.data = data;
    state.pos = data;
    state.dataEnd = data + cleanLength;
    state.error = error;

    root = TvT_JSON_CreateValue();
    if (!root) {
        if (error) {
            error->type = JSON_ERROR_MEMORY_ALLOCATION;
            Q_strncpyz(error->expected, "memory allocation", sizeof(error->expected));
            Q_strncpyz(error->found, "allocation failed", sizeof(error->found));
        }
        return NULL;
    }

    if (!TvT_JSON_ParseValue(root)) {
        TvT_JSON_FreeValue(root);
        memset(&state, 0, sizeof(JSONState_t));
        return NULL;
    }

    // Reject trailing garbage after the root value.
    if (state.pos < state.dataEnd) {
        TvT_JSON_SetError(JSON_ERROR_UNEXPECTED_CHARACTER, "end of input", state.pos);
        TvT_JSON_FreeValue(root);
        memset(&state, 0, sizeof(JSONState_t));
        return NULL;
    }

    memset(&state, 0, sizeof(JSONState_t));
    return root;
}

static int TvT_JSON_EscapedLength(const char *str, int len) {
    int i, result = 0;

    for (i = 0; i < len; i++) {
        switch (str[i]) {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                result += 2;
                break;
            default:
                if ((unsigned char)str[i] >= 0x20) {
                    result++;
                }
                break;
        }
    }

    return result;
}

static char *TvT_JSON_EscapeString(const char *str, int len, int *newLen) {
    int   i;
    char *escaped = malloc(TvT_JSON_EscapedLength(str, len) + 1);
    char *p;

    if (!escaped) {
        return NULL;
    }

    *newLen = 0;
    p = escaped;

    for (i = 0; i < len; ++i) {
        switch (str[i]) {
            case '\"':
                *p++ = '\\';
                *p++ = '\"';
                *newLen += 2;
                break;
            case '\\':
                *p++ = '\\';
                *p++ = '\\';
                *newLen += 2;
                break;
            case '\b':
                *p++ = '\\';
                *p++ = 'b';
                *newLen += 2;
                break;
            case '\f':
                *p++ = '\\';
                *p++ = 'f';
                *newLen += 2;
                break;
            case '\n':
                *p++ = '\\';
                *p++ = 'n';
                *newLen += 2;
                break;
            case '\r':
                *p++ = '\\';
                *p++ = 'r';
                *newLen += 2;
                break;
            case '\t':
                *p++ = '\\';
                *p++ = 't';
                *newLen += 2;
                break;
            default:
                if ((unsigned char)str[i] >= 0x20) {
                    *p++ = str[i];
                    (*newLen)++;
                }
                break;
        }
    }
    *p = '\0';
    return escaped;
}

static int TvT_JSON_AppendToBuffer(char **buffer, int *bufferSize, int *currentPos, const char *str, int len) {
    if (len < 0) {
        len = strlen(str);
    }

    // Measuring pass: just accumulate the total size.
    if (!*buffer) {
        *currentPos += len;
        return qtrue;
    }

    if (*currentPos + len >= *bufferSize) {
        return qfalse;
    }

    memcpy(*buffer + *currentPos, str, len);
    *currentPos += len;
    (*buffer)[*currentPos] = '\0';
    return qtrue;
}

static qboolean TvT_JSON_AppendIndent(char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty) {
    char tabs[32];
    int  i;

    if (!pretty || indent <= 0) {
        return qtrue;
    }

    if (indent > (int)sizeof(tabs)) {
        indent = sizeof(tabs);
    }

    for (i = 0; i < indent; i++) {
        tabs[i] = '\t';
    }

    return TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, tabs, indent);
}

static qboolean TvT_JSON_AppendNewline(char **buffer, int *bufferSize, int *currentPos, qboolean pretty) {
    if (!pretty) {
        return qtrue;
    }

    return TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "\n", 1);
}

static qboolean TvT_JSON_WriteString(JSON_t *value, char **buffer, int *bufferSize, int *currentPos) {
    int   escapedLen;
    char *escaped;

    if (!value || value->type != TYPE_STRING || !value->value.str) {
        return qfalse;
    }

    if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "\"", 1)) {
        return qfalse;
    }

    // Measuring pass: compute escaped length without allocating.
    if (!*buffer) {
        escapedLen = TvT_JSON_EscapedLength(value->value.str, value->valueLen);
        *currentPos += escapedLen;
    }
    else {
        escaped = TvT_JSON_EscapeString(value->value.str, value->valueLen, &escapedLen);
        if (!escaped) {
            return qfalse;
        }

        if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, escaped, escapedLen)) {
            free(escaped);
            return qfalse;
        }
        free(escaped);
    }

    return TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "\"", 1);
}

static qboolean TvT_JSON_WriteNumber(JSON_t *value, char **buffer, int *bufferSize, int *currentPos) {
    char numStr[32];

    if (!value) {
        return qfalse;
    }

    if (value->type == TYPE_INT) {
        Com_sprintf(numStr, sizeof(numStr), "%d", value->value.numberInt);
    }
    else if (value->type == TYPE_FLOAT) {
        Com_sprintf(numStr, sizeof(numStr), "%f", value->value.numberFloat);
    }
    else {
        return qfalse;
    }

    return TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, numStr, -1);
}

static qboolean TvT_JSON_WriteArray(JSON_t *value, char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty) {
    JSON_t  *child;
    qboolean first;

    if (!value || value->type != TYPE_ARRAY) {
        return qfalse;
    }

    if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "[", 1)) {
        return qfalse;
    }

    child = value->child;
    first = qtrue;

    if (child && pretty) {
        if (!TvT_JSON_AppendNewline(buffer, bufferSize, currentPos, pretty)) {
            return qfalse;
        }
    }

    while (child) {
        if (!first) {
            if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, ",", 1)) {
                return qfalse;
            }
            if (!TvT_JSON_AppendNewline(buffer, bufferSize, currentPos, pretty)) {
                return qfalse;
            }
        }

        if (!TvT_JSON_AppendIndent(buffer, bufferSize, currentPos, indent + 1, pretty)) {
            return qfalse;
        }

        if (!TvT_JSON_WriteValue(child, buffer, bufferSize, currentPos, indent + 1, pretty)) {
            return qfalse;
        }

        child = child->next;
        first = qfalse;
    }

    if (value->child && pretty) {
        if (!TvT_JSON_AppendNewline(buffer, bufferSize, currentPos, pretty)) {
            return qfalse;
        }
        if (!TvT_JSON_AppendIndent(buffer, bufferSize, currentPos, indent, pretty)) {
            return qfalse;
        }
    }

    return TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "]", 1);
}

static qboolean TvT_JSON_WriteObject(JSON_t *value, char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty) {
    JSON_t  *child;
    qboolean first;
    int      escapedKeyLen;
    char    *escapedKey;

    if (!value || value->type != TYPE_OBJECT) {
        return qfalse;
    }

    if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "{", 1)) {
        return qfalse;
    }

    child = value->child;
    first = qtrue;

    if (child && pretty) {
        if (!TvT_JSON_AppendNewline(buffer, bufferSize, currentPos, pretty)) {
            return qfalse;
        }
    }

    while (child) {
        if (!first) {
            if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, ",", 1)) {
                return qfalse;
            }
            if (!TvT_JSON_AppendNewline(buffer, bufferSize, currentPos, pretty)) {
                return qfalse;
            }
        }

        if (!TvT_JSON_AppendIndent(buffer, bufferSize, currentPos, indent + 1, pretty)) {
            return qfalse;
        }

        if (!child->key) {
            return qfalse;
        }

        if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "\"", 1)) {
            return qfalse;
        }

        // Measuring pass: compute escaped key length without allocating.
        if (!*buffer) {
            escapedKeyLen = TvT_JSON_EscapedLength(child->key, child->keyLen);
            *currentPos += escapedKeyLen;
        }
        else {
            escapedKey = TvT_JSON_EscapeString(child->key, child->keyLen, &escapedKeyLen);
            if (!escapedKey) {
                return qfalse;
            }

            if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, escapedKey, escapedKeyLen)) {
                free(escapedKey);
                return qfalse;
            }
            free(escapedKey);
        }

        if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "\"", 1)) {
            return qfalse;
        }

        if (pretty) {
            if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, ": ", 2)) {
                return qfalse;
            }
        }
        else {
            if (!TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, ":", 1)) {
                return qfalse;
            }
        }

        if (!TvT_JSON_WriteValue(child, buffer, bufferSize, currentPos, indent + 1, pretty)) {
            return qfalse;
        }

        child = child->next;
        first = qfalse;
    }

    if (value->child && pretty) {
        if (!TvT_JSON_AppendNewline(buffer, bufferSize, currentPos, pretty)) {
            return qfalse;
        }
        if (!TvT_JSON_AppendIndent(buffer, bufferSize, currentPos, indent, pretty)) {
            return qfalse;
        }
    }

    return TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "}", 1);
}

static qboolean TvT_JSON_WriteValue(JSON_t *value, char **buffer, int *bufferSize, int *currentPos, int indent, qboolean pretty) {
    if (!value) {
        return qfalse;
    }

    switch (value->type) {
        case TYPE_STRING:
            return TvT_JSON_WriteString(value, buffer, bufferSize, currentPos);
        case TYPE_INT:
        case TYPE_FLOAT:
            return TvT_JSON_WriteNumber(value, buffer, bufferSize, currentPos);
        case TYPE_ARRAY:
            return TvT_JSON_WriteArray(value, buffer, bufferSize, currentPos, indent, pretty);
        case TYPE_OBJECT:
            return TvT_JSON_WriteObject(value, buffer, bufferSize, currentPos, indent, pretty);
        case TYPE_BOOL:
            return value->value.boolean
                       ? TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "true", 4)
                       : TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "false", 5);
        case TYPE_NULL:
            return TvT_JSON_AppendToBuffer(buffer, bufferSize, currentPos, "null", 4);
        default:
            return qfalse;
    }
}

char *TvT_JSON_Serialize(JSON_t *value, qboolean pretty, JSONErrorInfo_t *error) {
    int   bufferSize;
    char *buffer;
    int   currentPos;

    if (error) {
        memset(error, 0, sizeof(JSONErrorInfo_t));
    }

    memset(&state, 0, sizeof(JSONState_t));
    state.error = error;

    if (!value) {
        TvT_JSON_SetError(JSON_ERROR_INVALID_INPUT, "valid JSON value", "null value");
        memset(&state, 0, sizeof(JSONState_t));
        return NULL;
    }

    buffer = NULL;
    bufferSize = 0;
    currentPos = 0;
    if (!TvT_JSON_WriteValue(value, &buffer, &bufferSize, &currentPos, 0, pretty)) {
        memset(&state, 0, sizeof(JSONState_t));
        return NULL;
    }

    bufferSize = currentPos + 1;
    buffer = malloc(bufferSize);
    if (!buffer) {
        TvT_JSON_SetError(JSON_ERROR_MEMORY_ALLOCATION, "memory allocation", "allocation failed");
        memset(&state, 0, sizeof(JSONState_t));
        return NULL;
    }

    currentPos = 0;
    buffer[0] = '\0';
    if (!TvT_JSON_WriteValue(value, &buffer, &bufferSize, &currentPos, 0, pretty)) {
        free(buffer);
        memset(&state, 0, sizeof(JSONState_t));
        return NULL;
    }

    memset(&state, 0, sizeof(JSONState_t));
    return buffer;
}

int TvT_JSON_GetArraySize(JSON_t *array) {
    JSON_t *child;
    int     count = 0;

    if (!array || array->type != TYPE_ARRAY) {
        return 0;
    }

    child = array->child;
    while (child) {
        count++;
        child = child->next;
    }

    return count;
}

JSON_t *TvT_JSON_RemoveObjectItem(JSON_t *object, const char *key) {
    JSON_t *child, *prev;

    if (!object || object->type != TYPE_OBJECT || !key) {
        return NULL;
    }

    prev = NULL;
    child = object->child;
    while (child) {
        if (child->key && !Q_stricmp(child->key, key)) {
            if (prev) {
                prev->next = child->next;
            }
            else {
                object->child = child->next;
            }

            if (object->tail == child) {
                object->tail = prev;
            }

            child->next = NULL;
            return child;
        }
        prev = child;
        child = child->next;
    }

    return NULL;
}

JSON_t *TvT_JSON_RemoveArrayItem(JSON_t *array, int index) {
    JSON_t *child, *prev;
    int     i;

    if (!array || array->type != TYPE_ARRAY || index < 0) {
        return NULL;
    }

    prev = NULL;
    child = array->child;
    i = 0;

    while (child && i < index) {
        prev = child;
        child = child->next;
        i++;
    }

    if (!child) {
        return NULL;
    }

    if (prev) {
        prev->next = child->next;
    }
    else {
        array->child = child->next;
    }

    if (array->tail == child) {
        array->tail = prev;
    }

    child->next = NULL;
    return child;
}

const char *TvT_JSON_GetString(JSON_t *object, const char *key, const char *fallback) {
    JSON_t *item = TvT_JSON_GetObjectItem(object, key);

    if (!item || item->type != TYPE_STRING) {
        return fallback;
    }

    return item->value.str;
}

int TvT_JSON_GetInt(JSON_t *object, const char *key, int fallback) {
    JSON_t *item = TvT_JSON_GetObjectItem(object, key);

    if (!item) {
        return fallback;
    }

    // Accept both int and float for convenience.
    if (item->type == TYPE_INT) {
        return item->value.numberInt;
    }

    if (item->type == TYPE_FLOAT) {
        return (int)item->value.numberFloat;
    }

    return fallback;
}

float TvT_JSON_GetFloat(JSON_t *object, const char *key, float fallback) {
    JSON_t *item = TvT_JSON_GetObjectItem(object, key);

    if (!item) {
        return fallback;
    }

    // Accept both float and int for convenience.
    if (item->type == TYPE_FLOAT) {
        return item->value.numberFloat;
    }

    if (item->type == TYPE_INT) {
        return (float)item->value.numberInt;
    }

    return fallback;
}

qboolean TvT_JSON_GetBool(JSON_t *object, const char *key, qboolean fallback) {
    JSON_t *item = TvT_JSON_GetObjectItem(object, key);

    if (!item || item->type != TYPE_BOOL) {
        return fallback;
    }

    return item->value.boolean;
}

void TvT_JSON_PrintTree(JSON_t *json, int indent) {
    const char *name;
    JSON_t     *child;
    int         i;

    if (!json) {
        return;
    }

    name = (indent == 0) ? "root" : (json->key ? json->key : "");

    for (i = 0; i < indent; i++) {
        Com_Printf("        ");
    }

    switch (json->type) {
        case TYPE_STRING:
            Com_Printf("%s(string): %s\n", name, json->value.str);
            break;

        case TYPE_INT:
            Com_Printf("%s(integer): %d\n", name, json->value.numberInt);
            break;

        case TYPE_FLOAT:
            Com_Printf("%s(float): %f\n", name, json->value.numberFloat);
            break;

        case TYPE_BOOL:
            Com_Printf("%s(bool): %s\n", name,
                       json->value.boolean == qtrue ? "true" : "false");
            break;

        case TYPE_OBJECT:
            Com_Printf("%s(object):\n", name);
            child = json->child;
            while (child) {
                TvT_JSON_PrintTree(child, indent + 1);
                child = child->next;
            }
            break;

        case TYPE_ARRAY:
            Com_Printf("%s(array):\n", name);
            child = json->child;
            while (child) {
                TvT_JSON_PrintTree(child, indent + 1);
                child = child->next;
            }
            break;

        case TYPE_NULL:
            Com_Printf("%s(null)\n", name);
            break;

        default:
            Com_Printf("%s(unknown type)\n", name);
            break;
    }
}
