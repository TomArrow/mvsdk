#include "../../game/q_shared.h"

#define MEMPOOL_SIZE (1024 * 1024 * 32)

#define ALIGN_SIZE(size) (((size) + 0x03U) & ~0x03U)

#define SIZE_MASK 0x7FFFFFFF  // 31 bits for size
#define FLAGS_MASK 0x80000000 // 1 bit for used/free flag
#define BLOCK_FREE 0x00000000 // Free flag (bit cleared)
#define BLOCK_USED 0x80000000 // Used flag (bit set)

#define GET_SIZE(block) ((block)->size_and_flags & SIZE_MASK)
#define GET_FLAGS(block) ((block)->size_and_flags & FLAGS_MASK)
#define SET_SIZE_FLAGS(block, s, f) ((block)->size_and_flags = ((s) & SIZE_MASK) | ((f) & FLAGS_MASK))
#define IS_BLOCK_USED(block) (GET_FLAGS(block) == BLOCK_USED)
#define IS_BLOCK_FREE(block) (GET_FLAGS(block) == BLOCK_FREE)

#define BLOCK_HEADER_SIZE sizeof(block_t)
#define BLOCK_FOOTER_SIZE sizeof(size_t)
#define BLOCK_OVERHEAD    (BLOCK_HEADER_SIZE + BLOCK_FOOTER_SIZE)

typedef struct block {
    size_t        size_and_flags;
    struct block *next;
    struct block *prev;
} block_t;

// Boundary tag (footer) access — footer is the last sizeof(size_t) bytes of a block
#define GET_FOOTER_PTR(block)     ((size_t *)((char *)(block) + GET_SIZE(block) - BLOCK_FOOTER_SIZE))
#define SET_FOOTER(block)         (*GET_FOOTER_PTR(block) = (block)->size_and_flags)

// Physical neighbour access via boundary tags
#define GET_NEXT_PHYS(block)      ((block_t *)((char *)(block) + GET_SIZE(block)))
#define GET_PREV_FOOTER_VAL(block) (*((size_t *)((char *)(block) - BLOCK_FOOTER_SIZE)))
#define GET_PREV_PHYS(block)      ((block_t *)((char *)(block) - (GET_PREV_FOOTER_VAL(block) & SIZE_MASK)))

typedef struct {
    size_t pool_size;
    int    used_blocks;
    size_t used_bytes;
    size_t used_overhead;
    size_t used_largest;
    int    free_blocks;
    size_t free_bytes;
    size_t free_largest;
} TvT_MemStats_t;

// Memory allocator functions
void *TvT_Mem_Alloc(size_t size);
void *TvT_Mem_Calloc(size_t num, size_t size);
void *TvT_Mem_Realloc(void *ptr, size_t new_size);
void  TvT_Mem_Free(void *ptr);
void  TvT_Mem_Init(void);
void  TvT_Mem_Reset(void);
void  TvT_Mem_GetStats(TvT_MemStats_t *stats);

#ifdef Q3_VM
#undef malloc
#define malloc TvT_Mem_Alloc

#undef calloc
#define calloc TvT_Mem_Calloc

#undef realloc
#define realloc TvT_Mem_Realloc
#undef free
#define free TvT_Mem_Free
#endif // Q3_VM
