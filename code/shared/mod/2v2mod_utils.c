#include "2v2mod_utils.h"
#include "2v2mod_memory.h"
#include "../../game/bg_multiversion.h"

static char *TvT_Table_Strdup(const char *s) {
    int   len;
    char *out;

    len = strlen(s) + 1;
    out = malloc(len);
    if (!out) {
        Com_Error(ERR_FATAL, "TvT_Table_Strdup: out of memory");
    }
    memcpy(out, s, len);
    return out;
}

static char *TvT_Table_WriteColor(char *pos, const char *color) {
    if (color) {
        *pos++ = color[0];
        *pos++ = color[1];
    }
    return pos;
}

// Write aligned cell content directly to output: [pad] [color] text [reset] [pad].
static char *TvT_Table_PadCell(char *pos, const char *text, const char *color, int visLen, int colWidth, tableAlign_t align) {
    int padding;
    int padLeft;
    int padRight;
    int textLen;

    if (!text) {
        text = "";
    }

    textLen = strlen(text);
    padding = colWidth - visLen;

    if (padding < 0) {
        padding = 0;
    }

    switch (align) {
        case ALIGN_RIGHT:
            padLeft = padding;
            padRight = 0;
            break;
        case ALIGN_CENTER:
            padLeft = padding / 2;
            padRight = padding - padLeft;
            break;
        case ALIGN_LEFT:
        default:
            padLeft = 0;
            padRight = padding;
            break;
    }

    if (padLeft > 0) {
        memset(pos, ' ', padLeft);
        pos += padLeft;
    }

    if (color) {
        *pos++ = color[0];
        *pos++ = color[1];
    }

    memcpy(pos, text, textLen);
    pos += textLen;

    *pos++ = '^';
    *pos++ = '7';

    if (padRight > 0) {
        memset(pos, ' ', padRight);
        pos += padRight;
    }

    return pos;
}

table_t *TvT_Table_Create(void) {
    table_t *t;

    t = malloc(sizeof(table_t));
    if (!t) {
        Com_Error(ERR_FATAL, "TvT_Table_Create: out of memory");
    }
    memset(t, 0, sizeof(table_t));

    t->drawBorder = qtrue;
    t->drawHeaderSep = qtrue;
    t->accentColor = S_COLOR_MAGENTA;

    return t;
}

void TvT_Table_Destroy(table_t *t) {
    int i, j;

    if (!t) {
        return;
    }

    for (i = 0; i < t->numCols; i++) {
        free(t->cols[i].header);
    }
    free(t->cols);

    for (i = 0; i < t->numRows; i++) {
        for (j = 0; j < t->numCols; j++) {
            free(t->rows[i].cells[j].text);
        }
        free(t->rows[i].cells);
    }
    free(t->rows);

    free(t);
}

void TvT_Table_AddCol(table_t *t, const char *header, tableAlign_t align) {
    if (t->numCols == t->colCap) {
        int oldCap = t->colCap;
        t->colCap = t->colCap ? t->colCap * 2 : 8;
        t->cols = realloc(t->cols, sizeof(tableCol_t) * t->colCap);
        if (!t->cols) {
            Com_Error(ERR_FATAL, "TvT_Table_AddCol: out of memory");
        }
        memset(&t->cols[oldCap], 0, sizeof(tableCol_t) * (t->colCap - oldCap));
    }

    t->cols[t->numCols].header = TvT_Table_Strdup(header);
    t->cols[t->numCols].align = align;
    t->cols[t->numCols].hdrVisLen = Q_PrintStrlen(header, (qboolean)(jk2startversion == VERSION_1_02));
    t->cols[t->numCols].maxRawLen = strlen(header);
    t->cols[t->numCols].minWidth = 0;
    t->cols[t->numCols].hidden = qfalse;

    t->numCols++;
}

tableRow_t *TvT_Table_AddRow(table_t *t) {
    int row = t->numRows;

    if (t->numRows == t->rowCap) {
        int oldCap = t->rowCap;
        t->rowCap = t->rowCap ? t->rowCap * 2 : 8;
        t->rows = realloc(t->rows, sizeof(tableRow_t) * t->rowCap);
        if (!t->rows) {
            Com_Error(ERR_FATAL, "TvT_Table_AddRow: out of memory");
        }
        memset(&t->rows[oldCap], 0, sizeof(tableRow_t) * (t->rowCap - oldCap));
    }
    t->rows[row].cells = malloc(sizeof(tableCell_t) * t->numCols);
    if (!t->rows[row].cells) {
        Com_Error(ERR_FATAL, "TvT_Table_AddRow: out of memory");
    }
    memset(t->rows[row].cells, 0, sizeof(tableCell_t) * t->numCols);

    t->numRows++;

    return &t->rows[row];
}

void TvT_Table_SetCell(table_t *t, tableRow_t *row, int col, const char *text) {
    free(row->cells[col].text);

    row->cells[col].text = TvT_Table_Strdup(text);
    row->cells[col].rawLen = strlen(text);
    row->cells[col].visLen = Q_PrintStrlen(text, (qboolean)(jk2startversion == VERSION_1_02));

    if (row->cells[col].rawLen > t->cols[col].maxRawLen) {
        t->cols[col].maxRawLen = row->cells[col].rawLen;
    }
}

void TvT_Table_SetCellColor(tableRow_t *row, int col, const char *color) {
    row->cells[col].color = color;
}

void TvT_Table_SetRowSep(tableRow_t *row, qboolean enabled) {
    row->sepBefore = enabled;
}

void TvT_Table_SetBorder(table_t *t, qboolean enabled) {
    t->drawBorder = enabled;
}

void TvT_Table_SetHeaderSep(table_t *t, qboolean enabled) {
    t->drawHeaderSep = enabled;
}

void TvT_Table_SetAccentColor(table_t *t, const char *color) {
    t->accentColor = color;
}

// Find column index by name, returns -1 if not found.
int TvT_Table_FindCol(table_t *t, const char *colName) {
    int i;

    for (i = 0; i < t->numCols; i++) {
        if (!Q_stricmp(t->cols[i].header, colName)) {
            return i;
        }
    }
    return -1;
}

// Show or hide a column by header name (data preserved for sort/filter).
void TvT_Table_HideCol(table_t *t, const char *colName, qboolean hidden) {
    int col = TvT_Table_FindCol(t, colName);

    if (col >= 0) {
        t->cols[col].hidden = hidden;
    }
}

// Sort state for qsort.
static int tvt_table_sortCol;
static int tvt_table_sortAsc;

static int TvT_Table_SortCompare(const void *a, const void *b) {
    const tableRow_t *ra = (const tableRow_t *)a;
    const tableRow_t *rb = (const tableRow_t *)b;
    const char       *ta = ra->cells[tvt_table_sortCol].text ? ra->cells[tvt_table_sortCol].text : "";
    const char       *tb = rb->cells[tvt_table_sortCol].text ? rb->cells[tvt_table_sortCol].text : "";
    int               result = Q_stricmp(ta, tb);
    return tvt_table_sortAsc ? result : -result;
}

// Sort rows by column name (case-insensitive string compare).
void TvT_Table_Sort(table_t *t, const char *colName, qboolean ascending) {
    int col;

    if (!t || t->numRows < 2) {
        return;
    }

    col = TvT_Table_FindCol(t, colName);
    if (col >= 0) {
        tvt_table_sortCol = col;
        tvt_table_sortAsc = ascending;
        qsort(t->rows, t->numRows, sizeof(tableRow_t), TvT_Table_SortCompare);
    }
}

// Set row filter callback, rows where keep() returns qfalse are hidden.
void TvT_Table_Filter(table_t *t, tableFilter_t keep, void *ctx) {
    if (!t) {
        return;
    }
    t->filterFn = keep;
    t->filterCtx = ctx;
}

static char *TvT_Table_WriteSepLine(char *pos, table_t *t, int *colWidths, char junction) {
    int i;

    if (t->drawBorder) {
        pos = TvT_Table_WriteColor(pos, t->accentColor);
        *pos++ = junction;
    }
    else {
        pos = TvT_Table_WriteColor(pos, t->accentColor);
    }

    for (i = 0; i < t->numCols; i++) {
        if (t->cols[i].hidden) {
            continue;
        }
        if (t->drawBorder) {
            memset(pos, '-', colWidths[i] + 2);
            pos += colWidths[i] + 2;
            *pos++ = junction;
        }
        else {
            memset(pos, '-', colWidths[i] + 2);
            pos += colWidths[i] + 2;
        }
    }

    if (t->accentColor) {
        pos = TvT_Table_WriteColor(pos, "^7");
    }
    *pos++ = '\n';
    return pos;
}

// Write a single padded cell: ' content |' (bordered) or ' content ' (borderless).
static char *TvT_Table_WriteCell(char *pos, table_t *t, int *colWidths,
                                 const char *text, const char *color, int visLen, int col) {
    *pos++ = ' ';
    pos = TvT_Table_PadCell(pos, text, color, visLen, colWidths[col], t->cols[col].align);
    *pos++ = ' ';
    if (t->drawBorder) {
        pos = TvT_Table_WriteColor(pos, t->accentColor);
        *pos++ = '|';
        pos = TvT_Table_WriteColor(pos, t->accentColor ? "^7" : NULL);
    }
    return pos;
}

static qboolean TvT_Table_IsRowVisible(table_t *t, int row) {
    if (!t->filterFn) {
        return qtrue;
    }
    return t->filterFn(t, &t->rows[row], t->filterCtx);
}

// Calculate output buffer size.
static int TvT_Table_CalcBufSize(table_t *t, int *colWidths, int visibleRows, int numSepRows) {
    int i;
    int accentColorLen = t->accentColor ? strlen(t->accentColor) : 0;
    int rowWidth = 2;
    int bufSize;

    for (i = 0; i < t->numCols; i++) {
        if (t->cols[i].hidden) {
            continue;
        }
        rowWidth += colWidths[i] + t->cols[i].maxRawLen + 7;
    }

    // Assume that we always have tables with borders, so we always allocate enough memory.
    bufSize = rowWidth * (visibleRows + 4 + numSepRows);

    if (accentColorLen) {
        int numBorderLines = (t->drawBorder ? 2 : 0) + (t->drawHeaderSep ? 1 : 0) + numSepRows;
        bufSize += numBorderLines * (accentColorLen + 2);
        bufSize += (1 + visibleRows) * (t->numCols + 1) * (accentColorLen + 2);
    }

    return bufSize + 1;
}

// Compute effective column width (max of header, minWidth, and all visible cell widths).
static int TvT_Table_ColWidth(table_t *t, int col) {
    int i;
    int w;

    w = t->cols[col].hdrVisLen;
    if (t->cols[col].minWidth > w) {
        w = t->cols[col].minWidth;
    }
    for (i = 0; i < t->numRows; i++) {
        if (TvT_Table_IsRowVisible(t, i) && t->rows[i].cells[col].visLen > w) {
            w = t->rows[i].cells[col].visLen;
        }
    }
    return w;
}

// Sync column widths between two tables so they render with matching alignment.
// Both tables must have the same number of columns.
void TvT_Table_SyncWidths(table_t *a, table_t *b) {
    int i, wa, wb, maxW;
    int numCols;

    if (!a || !b) {
        return;
    }
    assert(a->numCols == b->numCols);

    numCols = a->numCols < b->numCols ? a->numCols : b->numCols;

    for (i = 0; i < numCols; i++) {
        wa = TvT_Table_ColWidth(a, i);
        wb = TvT_Table_ColWidth(b, i);
        maxW = wa > wb ? wa : wb;
        a->cols[i].minWidth = maxW;
        b->cols[i].minWidth = maxW;
    }
}

// Render table to a string, caller must free the returned buffer.
char *TvT_Table_ToString(table_t *t) {
    int  *colWidths;
    int   i, j;
    int   visibleRows;
    int   numSepRows;
    char *buf;
    char *pos;

    if (!t || t->numCols == 0) {
        return NULL;
    }

    // Seed colWidths from header visible lengths.
    colWidths = malloc(sizeof(int) * t->numCols);
    if (!colWidths) {
        Com_Error(ERR_FATAL, "TvT_Table_ToString: out of memory");
    }
    for (i = 0; i < t->numCols; i++) {
        if (t->cols[i].hidden) {
            colWidths[i] = 0;
        }
        else {
            colWidths[i] = t->cols[i].hdrVisLen;
            if (t->cols[i].minWidth > colWidths[i]) {
                colWidths[i] = t->cols[i].minWidth;
            }
        }
    }

    // Scan visible rows for tight colWidths and count them for buffer sizing.
    visibleRows = 0;
    numSepRows = 0;
    for (i = 0; i < t->numRows; i++) {
        if (TvT_Table_IsRowVisible(t, i)) {
            for (j = 0; j < t->numCols; j++) {
                if (!t->cols[j].hidden && t->rows[i].cells[j].visLen > colWidths[j]) {
                    colWidths[j] = t->rows[i].cells[j].visLen;
                }
            }
            if (t->rows[i].sepBefore) {
                numSepRows++;
            }
            visibleRows++;
        }
    }

    buf = malloc(TvT_Table_CalcBufSize(t, colWidths, visibleRows, numSepRows));
    if (!buf) {
        Com_Error(ERR_FATAL, "TvT_Table_ToString: out of memory");
    }
    pos = buf;

    if (t->drawBorder) {
        pos = TvT_Table_WriteSepLine(pos, t, colWidths, '+');
    }

    // Write header row.
    if (t->drawBorder) {
        pos = TvT_Table_WriteColor(pos, t->accentColor);
        *pos++ = '|';
        pos = TvT_Table_WriteColor(pos, t->accentColor ? "^7" : NULL);
    }
    for (j = 0; j < t->numCols; j++) {
        if (t->cols[j].hidden) {
            continue;
        }
        pos = TvT_Table_WriteCell(pos, t, colWidths,
                                  t->cols[j].header, NULL, t->cols[j].hdrVisLen, j);
    }
    *pos++ = '\n';

    if (t->drawHeaderSep) {
        pos = TvT_Table_WriteSepLine(pos, t, colWidths, '|');
    }

    // Write data rows.
    for (i = 0; i < t->numRows; i++) {
        if (!TvT_Table_IsRowVisible(t, i)) {
            continue;
        }
        if (t->rows[i].sepBefore) {
            pos = TvT_Table_WriteSepLine(pos, t, colWidths, '|');
        }
        if (t->drawBorder) {
            pos = TvT_Table_WriteColor(pos, t->accentColor);
            *pos++ = '|';
            pos = TvT_Table_WriteColor(pos, t->accentColor ? "^7" : NULL);
        }
        for (j = 0; j < t->numCols; j++) {
            if (t->cols[j].hidden) {
                continue;
            }
            pos = TvT_Table_WriteCell(pos, t, colWidths,
                                      t->rows[i].cells[j].text, t->rows[i].cells[j].color,
                                      t->rows[i].cells[j].visLen, j);
        }
        *pos++ = '\n';
    }

    if (t->drawBorder) {
        pos = TvT_Table_WriteSepLine(pos, t, colWidths, '+');
    }

    *pos = '\0';

    free(colWidths);

    return buf;
}
