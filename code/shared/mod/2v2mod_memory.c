
#include "2v2mod_memory.h"
#include "../../game/q_shared.h"

static char     memPool[MEMPOOL_SIZE];
static block_t *free_list_head = NULL;
static qboolean memory_initialized = qfalse;

void TvT_Mem_Init(void) {
    block_t *initial_block;

    if (memory_initialized) {
        return;
    }
    initial_block = (block_t *)memPool;
    SET_SIZE_FLAGS(initial_block, MEMPOOL_SIZE, BLOCK_FREE);
    initial_block->next = NULL;
    initial_block->prev = NULL;
    SET_FOOTER(initial_block);

    free_list_head = initial_block;
    memory_initialized = qtrue;
}

void TvT_Mem_Reset(void) {
    memory_initialized = qfalse;
    free_list_head = NULL;
    TvT_Mem_Init();
}

void TvT_Mem_GetStats(tvt_MemStats_t *stats) {
    block_t *block;
    char    *pool_end;
    size_t   block_size;
    size_t   user_size;

    memset(stats, 0, sizeof(*stats));
    stats->pool_size = MEMPOOL_SIZE;

    if (!memory_initialized) {
        return;
    }

    pool_end = memPool + MEMPOOL_SIZE;
    block = (block_t *)memPool;

    while ((char *)block < pool_end) {
        block_size = GET_SIZE(block);
        if (!block_size || block_size > MEMPOOL_SIZE) {
            break;
        }

        if (IS_BLOCK_USED(block)) {
            user_size = block_size - BLOCK_OVERHEAD;
            stats->used_blocks++;
            stats->used_bytes += user_size;
            stats->used_overhead += BLOCK_OVERHEAD;
            if (user_size > stats->used_largest) {
                stats->used_largest = user_size;
            }
        }
        else {
            user_size = block_size - BLOCK_OVERHEAD;
            stats->free_blocks++;
            stats->free_bytes += user_size;
            if (user_size > stats->free_largest) {
                stats->free_largest = user_size;
            }
        }

        block = (block_t *)((char *)block + block_size);
    }
}

static void add_to_free_list(block_t *block) {
    block->next = free_list_head;
    block->prev = NULL;

    if (free_list_head) {
        free_list_head->prev = block;
    }

    free_list_head = block;
}

static void remove_from_free_list(block_t *block) {
    if (block->prev) {
        block->prev->next = block->next;
    }
    else {
        free_list_head = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }
}

static block_t *coalesce_blocks(block_t *block) {
    block_t *neighbour;
    char    *pool_end;
    size_t   new_size;

    if (!block || !IS_BLOCK_FREE(block)) {
        return block;
    }

    pool_end = memPool + MEMPOOL_SIZE;

    // Merge with next physical neighbour
    neighbour = GET_NEXT_PHYS(block);
    if ((char *)neighbour < pool_end && IS_BLOCK_FREE(neighbour)) {
        new_size = GET_SIZE(block) + GET_SIZE(neighbour);
        remove_from_free_list(neighbour);
        SET_SIZE_FLAGS(block, new_size, BLOCK_FREE);
        SET_FOOTER(block);
    }

    // Merge with previous physical neighbour
    if ((char *)block > memPool) {
        neighbour = GET_PREV_PHYS(block);
        if (IS_BLOCK_FREE(neighbour)) {
            new_size = GET_SIZE(neighbour) + GET_SIZE(block);
            remove_from_free_list(block);
            SET_SIZE_FLAGS(neighbour, new_size, BLOCK_FREE);
            SET_FOOTER(neighbour);
            block = neighbour;
        }
    }

    return block;
}

static void split_block(block_t *block, size_t needed_size) {
    size_t   remaining_size;
    size_t   aligned_needed_size;
    block_t *new_block;

    aligned_needed_size = ALIGN_SIZE(needed_size);
    remaining_size = GET_SIZE(block) - aligned_needed_size;

    if (remaining_size >= BLOCK_OVERHEAD) {

        new_block = (block_t *)((char *)block + aligned_needed_size);
        SET_SIZE_FLAGS(new_block, remaining_size, BLOCK_FREE);
        new_block->next = NULL;
        new_block->prev = NULL;
        SET_FOOTER(new_block);

        SET_SIZE_FLAGS(block, aligned_needed_size, BLOCK_USED);
        SET_FOOTER(block);

        add_to_free_list(new_block);
    }
    else {
        SET_SIZE_FLAGS(block, GET_SIZE(block), BLOCK_USED);
        SET_FOOTER(block);
    }
}

void *TvT_Mem_Alloc(size_t size) {
    size_t   total_size;
    block_t *current;

    if (!memory_initialized) {
        TvT_Mem_Init();
    }

    if (!size) {
        return NULL;
    }

    total_size = ALIGN_SIZE(size + BLOCK_OVERHEAD);
    current = free_list_head;

    if (!current) {
        return NULL;
    }

    while (current) {
        if (GET_SIZE(current) >= total_size) {
            remove_from_free_list(current);
            split_block(current, total_size);
            return (char *)current + BLOCK_HEADER_SIZE;
        }
        current = current->next;
    }

    return NULL;
}

void *TvT_Mem_Calloc(size_t num, size_t size) {
    size_t total_size;
    void  *ptr;

    if (!num || !size) {
        return NULL;
    }

    total_size = num * size;
    if (total_size / num != size) {
        return NULL;
    }

    ptr = TvT_Mem_Alloc(total_size);

    if (ptr) {
        memset(ptr, 0, total_size);
        return ptr;
    }

    return NULL;
}

void TvT_Mem_Free(void *ptr) {
    block_t *used_block;
    char    *pool_start;
    char    *pool_end;

    if (!ptr) {
        return;
    }

    used_block = (block_t *)((char *)ptr - BLOCK_HEADER_SIZE);

    pool_start = memPool;
    pool_end = pool_start + MEMPOOL_SIZE;

    if ((char *)used_block < pool_start || (char *)used_block >= pool_end) {
        return;
    }

    if (!IS_BLOCK_USED(used_block)) {
        return;
    }

    if (!GET_SIZE(used_block) || GET_SIZE(used_block) > MEMPOOL_SIZE) {
        return;
    }

    SET_SIZE_FLAGS(used_block, GET_SIZE(used_block), BLOCK_FREE);
    SET_FOOTER(used_block);
    used_block->next = NULL;
    used_block->prev = NULL;

    add_to_free_list(used_block);
    coalesce_blocks(used_block);
}

typedef struct {
    block_t *forward_block;
    block_t *backward_block;
} adjacent_blocks_t;

static adjacent_blocks_t find_adjacent_blocks(block_t *block, size_t new_total_size) {
    adjacent_blocks_t result;
    block_t          *neighbour;
    char             *pool_end;
    size_t            needed_expansion;

    result.forward_block = NULL;
    result.backward_block = NULL;

    needed_expansion = new_total_size - GET_SIZE(block);
    pool_end = memPool + MEMPOOL_SIZE;

    // Check next physical neighbour
    neighbour = GET_NEXT_PHYS(block);
    if ((char *)neighbour < pool_end && IS_BLOCK_FREE(neighbour) && GET_SIZE(neighbour) >= needed_expansion) {
        result.forward_block = neighbour;
    }

    // Check previous physical neighbour
    if ((char *)block > memPool) {
        neighbour = GET_PREV_PHYS(block);
        if (IS_BLOCK_FREE(neighbour) && GET_SIZE(neighbour) >= needed_expansion) {
            result.backward_block = neighbour;
        }
    }

    return result;
}

static void expand_forward_with_block(block_t *block, size_t new_total_size, block_t *adjacent_free) {
    block_t *new_free;
    size_t   needed_expansion;
    size_t   remaining_free;

    needed_expansion = new_total_size - GET_SIZE(block);
    remaining_free = GET_SIZE(adjacent_free) - needed_expansion;

    remove_from_free_list(adjacent_free);

    if (remaining_free >= BLOCK_OVERHEAD) {
        SET_SIZE_FLAGS(block, new_total_size, BLOCK_USED);
        SET_FOOTER(block);

        new_free = (block_t *)((char *)block + new_total_size);
        SET_SIZE_FLAGS(new_free, remaining_free, BLOCK_FREE);
        SET_FOOTER(new_free);
        add_to_free_list(new_free);
        coalesce_blocks(new_free);
    }
    else {
        SET_SIZE_FLAGS(block, GET_SIZE(block) + GET_SIZE(adjacent_free), BLOCK_USED);
        SET_FOOTER(block);
    }
}

static void *expand_backward_with_block(block_t *block, size_t new_total_size, block_t *backward_free, size_t old_user_size) {
    size_t   needed_expansion;
    size_t   remaining_free;
    size_t   backward_free_size;
    size_t   old_block_size;
    block_t *new_free;
    char    *old_data;
    char    *new_data;

    old_block_size = GET_SIZE(block);
    backward_free_size = GET_SIZE(backward_free);
    needed_expansion = new_total_size - old_block_size;
    remaining_free = backward_free_size - needed_expansion;

    old_data = (char *)block + BLOCK_HEADER_SIZE;
    new_data = (char *)backward_free + BLOCK_HEADER_SIZE;

    remove_from_free_list(backward_free);

    // Move data BEFORE writing new headers, since the new block overlaps the old data region.
    // After this, block's header may be overwritten — only use saved sizes below.
    memmove(new_data, old_data, old_user_size);

    if (remaining_free >= BLOCK_OVERHEAD) {
        SET_SIZE_FLAGS(backward_free, new_total_size, BLOCK_USED);
        SET_FOOTER(backward_free);

        new_free = (block_t *)((char *)backward_free + new_total_size);
        SET_SIZE_FLAGS(new_free, remaining_free, BLOCK_FREE);
        SET_FOOTER(new_free);
        add_to_free_list(new_free);
        coalesce_blocks(new_free);
    }
    else {
        SET_SIZE_FLAGS(backward_free, backward_free_size + old_block_size, BLOCK_USED);
        SET_FOOTER(backward_free);
    }

    return new_data;
}

void *TvT_Mem_Realloc(void *ptr, size_t new_size) {
    void             *new_ptr;
    block_t          *old_block;
    size_t            old_size;
    size_t            new_total_size;
    adjacent_blocks_t adjacent_blocks;

    if (!ptr) {
        return TvT_Mem_Alloc(new_size);
    }

    if (!new_size) {
        TvT_Mem_Free(ptr);
        return NULL;
    }

    old_block = (block_t *)((char *)ptr - BLOCK_HEADER_SIZE);

    if (!IS_BLOCK_USED(old_block)) {
        return NULL;
    }

    old_size = GET_SIZE(old_block) - BLOCK_OVERHEAD;
    new_total_size = ALIGN_SIZE(new_size + BLOCK_OVERHEAD);

    if (new_total_size <= GET_SIZE(old_block)) {
        block_t *remainder;
        char    *pool_end;

        split_block(old_block, new_total_size);

        pool_end = memPool + MEMPOOL_SIZE;
        remainder = GET_NEXT_PHYS(old_block);
        if ((char *)remainder < pool_end && IS_BLOCK_FREE(remainder)) {
            coalesce_blocks(remainder);
        }
        return ptr;
    }

    adjacent_blocks = find_adjacent_blocks(old_block, new_total_size);

    if (adjacent_blocks.forward_block != NULL) {
        expand_forward_with_block(old_block, new_total_size, adjacent_blocks.forward_block);
        return ptr;
    }

    if (adjacent_blocks.backward_block != NULL) {
        return expand_backward_with_block(old_block, new_total_size, adjacent_blocks.backward_block, old_size);
    }

    new_ptr = TvT_Mem_Alloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    memmove(new_ptr, ptr, MIN(old_size, new_size));

    TvT_Mem_Free(ptr);

    return new_ptr;
}
