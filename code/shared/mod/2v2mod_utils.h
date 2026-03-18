#ifndef TVT_UTILS_H
#define TVT_UTILS_H

#include "../../game/q_shared.h"

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} tableAlign_t;

typedef struct {
    char       *text;
    const char *color;
    int         rawLen;
    int         visLen;
} tableCell_t;

typedef struct {
    tableCell_t *cells;
    qboolean     sepBefore;
} tableRow_t;

typedef struct {
    char        *header;
    tableAlign_t align;
    int          hdrVisLen;
    int          maxRawLen;
    int          minWidth;
    qboolean     hidden;
} tableCol_t;

typedef struct table_s table_t;
typedef qboolean (*tableFilter_t)(table_t *t, tableRow_t *row, void *ctx);

struct table_s {
    tableCol_t   *cols;
    int           numCols;
    int           colCap;
    tableRow_t   *rows;
    int           numRows;
    int           rowCap;
    qboolean      drawBorder;
    qboolean      drawHeaderSep;
    const char   *accentColor;
    qboolean      accentColorExplicit;
    tableFilter_t filterFn;
    void         *filterCtx;
};

typedef struct {
    const char *colName;
    const char *search;
} tvt_FilterCtx_t;

table_t    *TvT_Table_Create(void);
void        TvT_Table_Destroy(table_t *t);
void        TvT_Table_AddCol(table_t *t, const char *header, tableAlign_t align);
tableRow_t *TvT_Table_AddRow(table_t *t);
void        TvT_Table_SetCell(table_t *t, tableRow_t *row, int col, const char *text);
void        TvT_Table_SetCellColor(tableRow_t *row, int col, const char *color);
void        TvT_Table_SetRowSep(tableRow_t *row, qboolean enabled);
void        TvT_Table_SetBorder(table_t *t, qboolean enabled);
void        TvT_Table_SetHeaderSep(table_t *t, qboolean enabled);
void        TvT_Table_SetAccentColor(table_t *t, const char *color);
int         TvT_Table_FindCol(table_t *t, const char *colName);
void        TvT_Table_HideCol(table_t *t, const char *colName, qboolean hidden);
void        TvT_Table_Sort(table_t *t, const char *colName, qboolean ascending);
void        TvT_Table_Filter(table_t *t, tableFilter_t keep, void *ctx);
qboolean    TvT_Table_FilterSubstring(table_t *t, tableRow_t *row, void *ctx);
void        TvT_Table_SyncWidths(table_t *a, table_t *b);
char       *TvT_Table_ToString(table_t *t);

extern char tvt_defaultAccentColor[3];

void        TvT_TokenizeString(const char *text);
int         TvT_Argc(void);
const char *TvT_Argv(int index);

int TvT_RemoveWhitespace(char *str);

#endif // TVT_UTILS_H
