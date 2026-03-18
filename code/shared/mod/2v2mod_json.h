#ifndef TVT_JSON_H
#define TVT_JSON_H

#include "../../game/q_shared.h"

typedef enum {
    JSON_ERROR_NONE,
    JSON_ERROR_INVALID_INPUT,
    JSON_ERROR_UNEXPECTED_EOF,
    JSON_ERROR_UNEXPECTED_CHARACTER,
    JSON_ERROR_INVALID_NUMBER,
    JSON_ERROR_INVALID_STRING,
    JSON_ERROR_INVALID_ESCAPE,
    JSON_ERROR_MEMORY_ALLOCATION,
    JSON_ERROR_MISSING_COLON,
    JSON_ERROR_MISSING_COMMA,
    JSON_ERROR_MISSING_QUOTE,
    JSON_ERROR_MISSING_BRACKET,
    JSON_ERROR_DUPLICATE_KEY
} JSONError_t;

typedef struct {
    JSONError_t type;
    char        context[64];
    char        expected[32];
    char        found[32];
} JSONErrorInfo_t;

typedef enum {
    TYPE_OBJECT,
    TYPE_ARRAY,
    TYPE_FLOAT,
    TYPE_INT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_NULL,
} JSONtype_t;

typedef union {
    char    *str;
    int      numberInt;
    float    numberFloat;
    qboolean boolean;
} JSONValue_t;

typedef struct JSON_s {
    char *key;
    int   keyLen;

    JSONtype_t  type;
    JSONValue_t value;
    int         valueLen;

    struct JSON_s *child;
    struct JSON_s *next;
    struct JSON_s *tail;
} JSON_t;

char   *TvT_JSON_Serialize(JSON_t *value, qboolean pretty, JSONErrorInfo_t *error);
JSON_t *TvT_JSON_Deserialize(char *data, int length, JSONErrorInfo_t *error);
void    TvT_JSON_FreeValue(JSON_t *value);
void    TvT_JSON_PrintError(const JSONErrorInfo_t *error);

JSON_t *TvT_JSON_CreateObject(void);
JSON_t *TvT_JSON_CreateArray(void);
JSON_t *TvT_JSON_CreateString(const char *string);
JSON_t *TvT_JSON_CreateNumber(int number);
JSON_t *TvT_JSON_CreateFloat(float number);
JSON_t *TvT_JSON_CreateBool(qboolean boolean);
JSON_t *TvT_JSON_CreateNull(void);

qboolean TvT_JSON_AddItemToObject(JSON_t *object, const char *key, JSON_t *item);
qboolean TvT_JSON_AddItemToArray(JSON_t *array, JSON_t *item);
JSON_t  *TvT_JSON_GetObjectItem(JSON_t *object, const char *key);
JSON_t  *TvT_JSON_GetArrayItem(JSON_t *array, int index);
int      TvT_JSON_GetArraySize(JSON_t *array);
JSON_t  *TvT_JSON_RemoveObjectItem(JSON_t *object, const char *key);
JSON_t  *TvT_JSON_RemoveArrayItem(JSON_t *array, int index);

const char *TvT_JSON_GetString(JSON_t *object, const char *key, const char *fallback);
int         TvT_JSON_GetInt(JSON_t *object, const char *key, int fallback);
float       TvT_JSON_GetFloat(JSON_t *object, const char *key, float fallback);
qboolean    TvT_JSON_GetBool(JSON_t *object, const char *key, qboolean fallback);

void TvT_JSON_PrintTree(JSON_t *json, int indent);

#endif // TVT_JSON_H
