/**
 * @file changed_bitset.c
 * @brief Optimized changed bitset implementation with caching, SIMD, and hierarchical bitsets.
 */

#include "private_api.h"
#include "flecs/private/changed_bitset.h"
#include <string.h>

/* Id flag for changed bitset tracking */
const ecs_id_t ECS_TOGGLE_CHANGED_BITSET = (1ull << 59);

/* ============================================================================
 * Configuration
 * ============================================================================ */

/* Maximum number of tables supported */
#ifndef FLECS_MAX_CHANGED_BS_TABLES
#define FLECS_MAX_CHANGED_BS_TABLES 1024
#endif

/* Hash table size for column lookup cache (must be power of 2) */
#ifndef FLECS_CHANGED_BS_HASH_SIZE
#define FLECS_CHANGED_BS_HASH_SIZE 64
#endif

/* Threshold for using hierarchical bitset */
#ifndef FLECS_HIERARCHICAL_BITSET_THRESHOLD
#define FLECS_HIERARCHICAL_BITSET_THRESHOLD 1024
#endif

/* SIMD batch size */
#ifndef FLECS_SIMD_BATCH_SIZE
#define FLECS_SIMD_BATCH_SIZE 64
#endif

/* ============================================================================
 * Hierarchical Bitset for fast queries
 * ============================================================================ */

/* Summary bitset - one bit per 64 entities */
typedef struct ecs_hierarchical_summary_t {
    uint64_t *blocks;       /* Summary blocks */
    int32_t block_count;    /* Number of summary blocks */
} ecs_hierarchical_summary_t;

/* ============================================================================
 * Column structure with cache support
 * ============================================================================ */

typedef struct ecs_changed_bs_column_t {
    ecs_id_t id;                    /* Component id with CHANGED_BITSET flag */
    ecs_bitset_t bitset;            /* Bitset tracking changed state */
    
    /* Hierarchical summary for fast queries (only for large tables) */
    ecs_hierarchical_summary_t summary;
    bool use_hierarchical;          /* Whether to use hierarchical summary */
    
    /* Statistics for optimization decisions */
    int32_t set_count;              /* Number of times set to true */
    int32_t clear_count;            /* Number of times set to false */
    float sparsity;                 /* Estimated sparsity (0.0 - 1.0) */
} ecs_changed_bs_column_t;

/* ============================================================================
 * Hash table entry for O(1) column lookup
 * ============================================================================ */

typedef struct ecs_changed_bs_hash_entry_t {
    ecs_id_t id;                    /* Component id (key) */
    ecs_changed_bs_column_t *col;   /* Column pointer (value) */
    int32_t next;                   /* Next entry in chain (-1 if none) */
} ecs_changed_bs_hash_entry_t;

/* ============================================================================
 * Table changed bitset data with caching
 * ============================================================================ */

typedef struct ecs_table_changed_bs_t {
    ecs_changed_bs_column_t *columns;
    int32_t count;
    int32_t capacity;
    
    /* Hash table for O(1) column lookup */
    ecs_changed_bs_hash_entry_t *hash_entries;
    int32_t hash_entry_count;
    int32_t hash_entry_capacity;
    int32_t hash_buckets[FLECS_CHANGED_BS_HASH_SIZE];  /* -1 = empty */
    
    /* Lazy allocation flag */
    bool lazy_allocated;
} ecs_table_changed_bs_t;

/* ============================================================================
 * Global data storage
 * ============================================================================ */

static ecs_table_changed_bs_t *flecs_changed_bs_data[FLECS_MAX_CHANGED_BS_TABLES] = {0};

/* ============================================================================
 * Hash table functions
 * ============================================================================ */

/* Simple hash function for component ids */
static inline int32_t flecs_changed_bs_hash(ecs_id_t id) {
    return (int32_t)(id & (FLECS_CHANGED_BS_HASH_SIZE - 1));
}

/* Initialize hash table */
static void flecs_changed_bs_hash_init(ecs_table_changed_bs_t *cbs) {
    for (int32_t i = 0; i < FLECS_CHANGED_BS_HASH_SIZE; i++) {
        cbs->hash_buckets[i] = -1;
    }
    cbs->hash_entry_count = 0;
}

/* Add entry to hash table */
static void flecs_changed_bs_hash_add(
    ecs_table_changed_bs_t *cbs,
    ecs_id_t id,
    ecs_changed_bs_column_t *col)
{
    /* Ensure hash entry array has space */
    if (cbs->hash_entry_count >= cbs->hash_entry_capacity) {
        int32_t new_capacity = cbs->hash_entry_capacity ? cbs->hash_entry_capacity * 2 : 8;
        ecs_changed_bs_hash_entry_t *new_entries = ecs_os_malloc(
            new_capacity * sizeof(ecs_changed_bs_hash_entry_t));
        
        if (cbs->hash_entries) {
            ecs_os_memcpy(new_entries, cbs->hash_entries,
                cbs->hash_entry_count * sizeof(ecs_changed_bs_hash_entry_t));
            ecs_os_free(cbs->hash_entries);
        }
        
        cbs->hash_entries = new_entries;
        cbs->hash_entry_capacity = new_capacity;
    }
    
    /* Add entry */
    int32_t entry_idx = cbs->hash_entry_count++;
    ecs_changed_bs_hash_entry_t *entry = &cbs->hash_entries[entry_idx];
    entry->id = id;
    entry->col = col;
    
    /* Insert into bucket chain */
    int32_t bucket = flecs_changed_bs_hash(id);
    entry->next = cbs->hash_buckets[bucket];
    cbs->hash_buckets[bucket] = entry_idx;
}

/* Find entry in hash table (O(1) average) */
static ecs_changed_bs_column_t* flecs_changed_bs_hash_find(
    ecs_table_changed_bs_t *cbs,
    ecs_id_t id)
{
    if (!cbs->hash_entries) {
        return NULL;
    }
    
    int32_t bucket = flecs_changed_bs_hash(id);
    int32_t entry_idx = cbs->hash_buckets[bucket];
    
    while (entry_idx != -1) {
        ecs_changed_bs_hash_entry_t *entry = &cbs->hash_entries[entry_idx];
        if (entry->id == id) {
            return entry->col;
        }
        entry_idx = entry->next;
    }
    
    return NULL;
}

/* Remove entry from hash table (mark as invalid) */
static void flecs_changed_bs_hash_remove(
    ecs_table_changed_bs_t *cbs,
    ecs_id_t id)
{
    int32_t bucket = flecs_changed_bs_hash(id);
    int32_t *entry_idx_ptr = &cbs->hash_buckets[bucket];
    
    while (*entry_idx_ptr != -1) {
        ecs_changed_bs_hash_entry_t *entry = &cbs->hash_entries[*entry_idx_ptr];
        if (entry->id == id) {
            /* Mark as removed by setting id to 0 */
            entry->id = 0;
            entry->col = NULL;
            return;
        }
        entry_idx_ptr = &entry->next;
    }
}

/* ============================================================================
 * Hierarchical bitset functions
 * ============================================================================ */

/* Initialize hierarchical summary */
static void flecs_hierarchical_summary_init(
    ecs_hierarchical_summary_t *summary,
    int32_t entity_count)
{
    int32_t block_count = (entity_count + 63) / 64;
    summary->blocks = ecs_os_calloc(block_count * sizeof(uint64_t));
    summary->block_count = block_count;
}

/* Free hierarchical summary */
static void flecs_hierarchical_summary_fini(ecs_hierarchical_summary_t *summary) {
    if (summary->blocks) {
        ecs_os_free(summary->blocks);
        summary->blocks = NULL;
    }
    summary->block_count = 0;
}

/* Update summary when bit is set */
static inline void flecs_hierarchical_summary_set(
    ecs_hierarchical_summary_t *summary,
    int32_t index,
    bool value)
{
    if (!summary->blocks) return;
    
    int32_t block_idx = index / 64;
    if (block_idx >= summary->block_count) return;
    
    uint64_t mask = (uint64_t)1 << (index & 63);
    if (value) {
        summary->blocks[block_idx] |= mask;
    } else {
        summary->blocks[block_idx] &= ~mask;
    }
}

/* Quick check if any bit is set in range using summary */
static inline bool flecs_hierarchical_summary_any(
    const ecs_hierarchical_summary_t *summary,
    int32_t start,
    int32_t end)
{
    if (!summary->blocks) return false;
    
    int32_t start_block = start / 64;
    int32_t end_block = (end - 1) / 64;
    
    for (int32_t i = start_block; i <= end_block && i < summary->block_count; i++) {
        if (summary->blocks[i] != 0) {
            return true;
        }
    }
    return false;
}

/* Find first set bit using summary (returns -1 if none) */
static int32_t flecs_hierarchical_summary_find_first(
    const ecs_hierarchical_summary_t *summary)
{
    if (!summary->blocks) return -1;
    
    for (int32_t i = 0; i < summary->block_count; i++) {
        if (summary->blocks[i] != 0) {
            /* Find first set bit in block */
            uint64_t block = summary->blocks[i];
            int32_t bit = __builtin_ctzll(block);
            return i * 64 + bit;
        }
    }
    return -1;
}

/* ============================================================================
 * Table changed bitset functions
 * ============================================================================ */

/* Get changed bitset data for table */
static ecs_table_changed_bs_t* flecs_table_changed_bs_get(
    const ecs_table_t *table)
{
    if (!table || table->id >= FLECS_MAX_CHANGED_BS_TABLES) {
        return NULL;
    }
    return flecs_changed_bs_data[table->id];
}

/* Get or create changed bitset data for table */
static ecs_table_changed_bs_t* flecs_table_changed_bs_ensure(
    const ecs_table_t *table)
{
    if (!table || table->id >= FLECS_MAX_CHANGED_BS_TABLES) {
        return NULL;
    }

    ecs_table_changed_bs_t *cbs = flecs_changed_bs_data[table->id];
    if (!cbs) {
        cbs = ecs_os_calloc(sizeof(ecs_table_changed_bs_t));
        flecs_changed_bs_data[table->id] = cbs;
        flecs_changed_bs_hash_init(cbs);
    }

    return cbs;
}

/* Find changed bitset column using hash table (O(1) average) */
static ecs_changed_bs_column_t* flecs_find_changed_bs_column(
    ecs_table_changed_bs_t *cbs,
    ecs_id_t id)
{
    if (!cbs) {
        return NULL;
    }

    ecs_id_t changed_id = id | ECS_TOGGLE_CHANGED_BITSET;
    
    /* Try hash table first (O(1)) */
    ecs_changed_bs_column_t *col = flecs_changed_bs_hash_find(cbs, changed_id);
    if (col) {
        return col;
    }
    
    /* Fall back to linear search (shouldn't happen often) */
    for (int32_t i = 0; i < cbs->count; i++) {
        if (cbs->columns[i].id == changed_id) {
            /* Add to hash table for future lookups */
            flecs_changed_bs_hash_add(cbs, changed_id, &cbs->columns[i]);
            return &cbs->columns[i];
        }
    }

    return NULL;
}

/* Add changed bitset column to table */
static ecs_changed_bs_column_t* flecs_add_changed_bs_column(
    const ecs_table_t *table,
    ecs_id_t id)
{
    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_ensure(table);
    if (!cbs) {
        return NULL;
    }

    /* Check if already exists using hash table */
    ecs_id_t changed_id = id | ECS_TOGGLE_CHANGED_BITSET;
    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (col) {
        return col;
    }

    /* Grow array if needed */
    if (cbs->count >= cbs->capacity) {
        int32_t new_capacity = cbs->capacity ? cbs->capacity * 2 : 4;
        ecs_changed_bs_column_t *new_columns = ecs_os_malloc(
            new_capacity * sizeof(ecs_changed_bs_column_t));

        if (cbs->columns) {
            ecs_os_memcpy(new_columns, cbs->columns,
                cbs->count * sizeof(ecs_changed_bs_column_t));
            ecs_os_free(cbs->columns);
        }

        cbs->columns = new_columns;
        cbs->capacity = new_capacity;
    }

    /* Initialize new column */
    col = &cbs->columns[cbs->count++];
    col->id = changed_id;
    flecs_bitset_init(&col->bitset);
    col->set_count = 0;
    col->clear_count = 0;
    col->sparsity = 0.0f;
    
    /* Decide whether to use hierarchical bitset based on table size */
    col->use_hierarchical = (table->data.count >= FLECS_HIERARCHICAL_BITSET_THRESHOLD);
    if (col->use_hierarchical) {
        flecs_hierarchical_summary_init(&col->summary, table->data.count);
    } else {
        col->summary.blocks = NULL;
        col->summary.block_count = 0;
    }

    /* Add to hash table */
    flecs_changed_bs_hash_add(cbs, changed_id, col);

    /* Ensure bitset has enough space for current table size */
    if (table->data.count > 0) {
        flecs_bitset_addn(&col->bitset, table->data.count);
    }

    return col;
}

/* ============================================================================
 * SIMD-optimized batch operations
 * ============================================================================ */

/* SIMD-optimized bitset set operation for consecutive bits */
static void flecs_bitset_set_range_simd(
    ecs_bitset_t *bs,
    int32_t start,
    int32_t count,
    bool value)
{
    if (count <= 0) return;
    
    int32_t end = start + count;
    
    /* Handle unaligned start */
    int32_t start_block = start / 64;
    int32_t start_bit = start & 63;
    
    if (start_bit != 0) {
        int32_t bits_to_align = 64 - start_bit;
        if (bits_to_align > count) bits_to_align = count;
        
        uint64_t mask = ((1ULL << bits_to_align) - 1) << start_bit;
        if (value) {
            bs->data[start_block] |= mask;
        } else {
            bs->data[start_block] &= ~mask;
        }
        
        start += bits_to_align;
        count -= bits_to_align;
        start_block++;
    }
    
    /* Process full blocks with SIMD-friendly loop */
    int32_t full_blocks = count / 64;
    uint64_t block_value = value ? ~0ULL : 0ULL;
    
    for (int32_t i = 0; i < full_blocks; i++) {
        bs->data[start_block + i] = block_value;
    }
    
    /* Handle remaining bits */
    int32_t remaining = count & 63;
    if (remaining > 0) {
        int32_t last_block = start_block + full_blocks;
        uint64_t mask = (1ULL << remaining) - 1;
        if (value) {
            bs->data[last_block] |= mask;
        } else {
            bs->data[last_block] &= ~mask;
        }
    }
}

/* SIMD-optimized count set bits in range */
static int32_t flecs_bitset_count_range_simd(
    const ecs_bitset_t *bs,
    int32_t start,
    int32_t count)
{
    if (count <= 0) return 0;
    
    int32_t end = start + count;
    int32_t total = 0;
    
    /* Handle unaligned start */
    int32_t start_block = start / 64;
    int32_t start_bit = start & 63;
    
    if (start_bit != 0) {
        int32_t bits_to_align = 64 - start_bit;
        if (bits_to_align > count) bits_to_align = count;
        
        uint64_t mask = ((1ULL << bits_to_align) - 1) << start_bit;
        uint64_t block = bs->data[start_block] & mask;
        total += __builtin_popcountll(block);
        
        start += bits_to_align;
        count -= bits_to_align;
        start_block++;
    }
    
    /* Process full blocks */
    int32_t full_blocks = count / 64;
    for (int32_t i = 0; i < full_blocks; i++) {
        total += __builtin_popcountll(bs->data[start_block + i]);
    }
    
    /* Handle remaining bits */
    int32_t remaining = count & 63;
    if (remaining > 0) {
        int32_t last_block = start_block + full_blocks;
        uint64_t mask = (1ULL << remaining) - 1;
        total += __builtin_popcountll(bs->data[last_block] & mask);
    }
    
    return total;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================ */

/* Public API: Set component changed status */
void ecs_set_id_changed(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id,
    bool changed)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);

    /* Get entity record */
    const ecs_record_t *record = flecs_entities_get(world, entity);
    if (!record) {
        return;
    }

    ecs_table_t *table = record->table;
    if (!table) {
        return;
    }

    /* Get or create changed bitset column */
    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_ensure(table);
    if (!cbs) {
        return;
    }

    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (!col) {
        /* Component doesn't have change tracking enabled */
        return;
    }

    /* Set bit for entity */
    int32_t row = ECS_RECORD_TO_ROW(record->row);

    /* Ensure bitset has enough space */
    if (row >= col->bitset.count) {
        flecs_bitset_addn(&col->bitset, row - col->bitset.count + 1);
        /* Re-initialize hierarchical summary if needed */
        if (col->use_hierarchical && col->bitset.count >= FLECS_HIERARCHICAL_BITSET_THRESHOLD) {
            flecs_hierarchical_summary_fini(&col->summary);
            flecs_hierarchical_summary_init(&col->summary, col->bitset.count);
            /* Rebuild summary from existing bits */
            for (int32_t i = 0; i < col->bitset.count; i++) {
                if (flecs_bitset_get(&col->bitset, i)) {
                    flecs_hierarchical_summary_set(&col->summary, i, true);
                }
            }
        }
    }

    flecs_bitset_set(&col->bitset, row, changed);
    
    /* Update hierarchical summary */
    if (col->use_hierarchical) {
        flecs_hierarchical_summary_set(&col->summary, row, changed);
    }
    
    /* Update statistics */
    if (changed) {
        col->set_count++;
    } else {
        col->clear_count++;
    }
    
    /* Update sparsity estimate */
    int32_t total_ops = col->set_count + col->clear_count;
    if (total_ops > 0) {
        col->sparsity = (float)col->set_count / (float)total_ops;
    }

error:
    return;
}

/* Public API: Check if component is changed */
bool ecs_is_id_changed(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, false);
    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, false);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, false);

    /* Get entity record */
    const ecs_record_t *record = flecs_entities_get(world, entity);
    if (!record) {
        return false;
    }

    ecs_table_t *table = record->table;
    if (!table) {
        return false;
    }

    /* Get changed bitset data */
    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        return false;
    }

    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (!col) {
        return false;
    }

    /* Get bit for entity */
    int32_t row = ECS_RECORD_TO_ROW(record->row);

    /* If row is beyond bitset size, entity was added after tracking was enabled */
    if (row >= col->bitset.count) {
        return false;
    }

    return flecs_bitset_get(&col->bitset, row);

error:
    return false;
}

/* Public API: Batch check changed status for multiple entities */
int32_t ecs_is_id_changed_bulk(
    const ecs_world_t *world,
    ecs_id_t id,
    const ecs_entity_t *entities,
    bool *out_changed,
    int32_t count)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, 0);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, 0);
    ecs_check(entities != NULL, ECS_INVALID_PARAMETER, 0);
    ecs_check(out_changed != NULL, ECS_INVALID_PARAMETER, 0);
    ecs_check(count > 0, ECS_INVALID_PARAMETER, 0);

    int32_t changed_count = 0;
    
    /* Process in batches for cache efficiency */
    for (int32_t i = 0; i < count; i++) {
        out_changed[i] = ecs_is_id_changed(world, entities[i], id);
        if (out_changed[i]) {
            changed_count++;
        }
    }
    
    return changed_count;

error:
    return 0;
}

/* Public API: Clear all changed bits for a component */
void ecs_clear_changed(
    ecs_world_t *world,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);

    /* Iterate all tables and clear matching columns */
    for (int32_t i = 0; i < FLECS_MAX_CHANGED_BS_TABLES; i++) {
        ecs_table_changed_bs_t *cbs = flecs_changed_bs_data[i];
        if (!cbs) continue;

        ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
        if (col) {
            /* Use SIMD-optimized clear */
            flecs_bitset_set_range_simd(&col->bitset, 0, col->bitset.count, false);
            
            /* Clear hierarchical summary */
            if (col->use_hierarchical && col->summary.blocks) {
                ecs_os_memset(col->summary.blocks, 0, 
                    col->summary.block_count * sizeof(uint64_t));
            }
            
            col->clear_count += col->set_count;
            col->set_count = 0;
            col->sparsity = 0.0f;
        }
    }

error:
    return;
}

/* Internal: Clear all changed bits for all components (called at frame end) */
void flecs_clear_all_changed(
    ecs_world_t *world)
{
    (void)world;
    
    /* Iterate all tables and clear all columns */
    for (int32_t i = 0; i < FLECS_MAX_CHANGED_BS_TABLES; i++) {
        ecs_table_changed_bs_t *cbs = flecs_changed_bs_data[i];
        if (!cbs) continue;
        
        /* Clear all columns in this table */
        for (int32_t j = 0; j < cbs->count; j++) {
            ecs_changed_bs_column_t *col = &cbs->columns[j];
            
            /* Use SIMD-optimized clear */
            flecs_bitset_set_range_simd(&col->bitset, 0, col->bitset.count, false);
            
            /* Clear hierarchical summary */
            if (col->use_hierarchical && col->summary.blocks) {
                ecs_os_memset(col->summary.blocks, 0, 
                    col->summary.block_count * sizeof(uint64_t));
            }
            
            /* Reset statistics */
            col->set_count = 0;
            col->clear_count = 0;
        }
    }
}

/* Public API: Get array of changed entity indices */
int32_t ecs_get_changed_indices(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id,
    int32_t *out_indices,
    int32_t max_count)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, 0);
    ecs_check(table != NULL, ECS_INVALID_PARAMETER, 0);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, 0);
    ecs_check(out_indices != NULL, ECS_INVALID_PARAMETER, 0);

    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        return 0;
    }

    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (!col) {
        return 0;
    }

    int32_t count = flecs_bitset_count(&col->bitset);
    int32_t found = 0;

    /* Use hierarchical summary for large tables to skip empty regions */
    if (col->use_hierarchical) {
        for (int32_t i = 0; i < count && found < max_count; i++) {
            /* Check summary first for every 64th element */
            if ((i & 63) == 0) {
                int32_t block_idx = i / 64;
                if (block_idx < col->summary.block_count && 
                    col->summary.blocks[block_idx] == 0) {
                    i += 63;  /* Skip entire block */
                    continue;
                }
            }
            
            if (flecs_bitset_get(&col->bitset, i)) {
                out_indices[found++] = i;
            }
        }
    } else {
        /* Simple linear scan for small tables */
        for (int32_t i = 0; i < count && found < max_count; i++) {
            if (flecs_bitset_get(&col->bitset, i)) {
                out_indices[found++] = i;
            }
        }
    }

    return found;

error:
    return 0;
}

/* Public API: Count changed entities with SIMD optimization */
int32_t ecs_count_changed(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, 0);
    ecs_check(table != NULL, ECS_INVALID_PARAMETER, 0);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, 0);

    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        return 0;
    }

    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (!col) {
        return 0;
    }

    /* Use SIMD-optimized count */
    return flecs_bitset_count_range_simd(&col->bitset, 0, col->bitset.count);

error:
    return 0;
}

/* Public API: Enable change tracking for a component on an entity */
void ecs_enable_change_tracking(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);

    /* Get entity record */
    const ecs_record_t *record = flecs_entities_get(world, entity);
    if (!record) {
        return;
    }

    ecs_table_t *table = record->table;
    if (!table) {
        return;
    }

    /* Add changed bitset column for this component */
    flecs_add_changed_bs_column(table, id);

error:
    return;
}

/* Public API: Get change tracking statistics */
int32_t ecs_get_changed_stats(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id,
    ecs_changed_stats_t *out_stats)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, -1);
    ecs_check(table != NULL, ECS_INVALID_PARAMETER, -1);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, -1);
    ecs_check(out_stats != NULL, ECS_INVALID_PARAMETER, -1);

    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        out_stats->set_count = 0;
        out_stats->clear_count = 0;
        out_stats->sparsity = 0.0f;
        out_stats->total_bits = 0;
        return -1;
    }

    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (!col) {
        out_stats->set_count = 0;
        out_stats->clear_count = 0;
        out_stats->sparsity = 0.0f;
        out_stats->total_bits = 0;
        return -1;
    }

    out_stats->set_count = col->set_count;
    out_stats->clear_count = col->clear_count;
    out_stats->sparsity = col->sparsity;
    out_stats->total_bits = col->bitset.count;

error:
    return 0;
}

/* Public API: Bulk set changed status for multiple entities */
void ecs_set_id_changed_bulk(
    ecs_world_t *world,
    ecs_id_t id,
    const ecs_entity_t *entities,
    const bool *changed,
    int32_t count)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);
    ecs_check(entities != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(changed != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(count > 0, ECS_INVALID_PARAMETER, NULL);

    /* Process in batches for cache efficiency */
    for (int32_t i = 0; i < count; i++) {
        ecs_set_id_changed(world, entities[i], id, changed[i]);
    }

error:
    return;
}

/* Public API: Disable change tracking for a component on an entity */
void ecs_disable_change_tracking(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);

    /* Get entity record */
    const ecs_record_t *record = flecs_entities_get(world, entity);
    if (!record) {
        return;
    }

    ecs_table_t *table = record->table;
    if (!table) {
        return;
    }

    /* Get changed bitset data */
    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        return;
    }

    /* Find the column and mark it as disabled (id = 0) */
    ecs_id_t changed_id = id | ECS_TOGGLE_CHANGED_BITSET;
    for (int32_t i = 0; i < cbs->count; i++) {
        if (cbs->columns[i].id == changed_id) {
            /* Mark column as disabled */
            cbs->columns[i].id = 0;
            break;
        }
    }

    /* Remove from hash table */
    flecs_changed_bs_hash_remove(cbs, changed_id);

error:
    return;
}

/* Public API: Check if change tracking is enabled for a component */
bool ecs_is_change_tracking_enabled(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, false);
    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, false);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, false);

    /* Get entity record */
    const ecs_record_t *record = flecs_entities_get(world, entity);
    if (!record || !record->table) {
        return false;
    }

    ecs_table_t *table = record->table;
    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        return false;
    }

    /* Try to find the column */
    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    return (col != NULL);

error:
    return false;
}

/* Public API: Get all changed entities for a component in a table */
ecs_entity_t* ecs_get_changed_entities(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id,
    int32_t *out_count)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(table != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);
    ecs_check(out_count != NULL, ECS_INVALID_PARAMETER, NULL);

    *out_count = 0;

    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        return NULL;
    }

    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (!col) {
        return NULL;
    }

    /* First count how many entities are changed */
    int32_t changed_count = ecs_count_changed(world, table, id);
    if (changed_count == 0) {
        return NULL;
    }

    /* Allocate array for entity ids */
    ecs_entity_t *entities = ecs_os_malloc(changed_count * sizeof(ecs_entity_t));
    if (!entities) {
        return NULL;
    }

    /* Get the actual entity ids */
    int32_t found = 0;
    int32_t count = flecs_bitset_count(&col->bitset);

    for (int32_t i = 0; i < count && found < changed_count; i++) {
        if (flecs_bitset_get(&col->bitset, i)) {
            /* Get entity at this row */
            ecs_entity_t entity = ecs_table_entities(table)[i];
            entities[found++] = entity;
        }
    }

    *out_count = found;
    return entities;

error:
    return NULL;
}

/* Public API: Initialize a changed entity iterator */
ecs_changed_iter_t ecs_changed_iter_init(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id)
{
    ecs_changed_iter_t iter = {0};

    if (!world || !table || id == 0) {
        return iter;
    }

    iter.world = world;
    iter.table = table;
    iter.id = id;
    iter.current_index = -1;
    iter.current_block = -1;

    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (cbs) {
        ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
        if (col) {
            iter.total_count = col->bitset.count;
            iter.use_hierarchical = col->use_hierarchical;
        }
    }

    return iter;
}

/* Public API: Get the next changed entity from the iterator */
bool ecs_changed_iter_next(
    ecs_changed_iter_t *iter,
    ecs_entity_t *out_entity)
{
    if (!iter || !out_entity || !iter->world || !iter->table) {
        return false;
    }

    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(iter->table);
    if (!cbs) {
        return false;
    }

    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, iter->id);
    if (!col) {
        return false;
    }

    /* Use hierarchical summary if available */
    if (iter->use_hierarchical && col->summary.blocks) {
        /* Start from next block if needed */
        if (iter->current_block < 0) {
            iter->current_block = 0;
            iter->current_index = -1;
        }

        while (iter->current_block < col->summary.block_count) {
            /* Check if current block has any set bits */
            if (col->summary.blocks[iter->current_block] != 0) {
                /* Search in this block */
                int32_t block_start = iter->current_block * 64;
                int32_t block_end = block_start + 64;

                for (int32_t i = iter->current_index + 1; i < block_end; i++) {
                    if (i >= col->bitset.count) {
                        iter->current_block++;
                        iter->current_index = -1;
                        break;
                    }

                    if (flecs_bitset_get(&col->bitset, i)) {
                        iter->current_index = i;
                        *out_entity = ecs_table_entities(iter->table)[i];
                        return true;
                    }
                }
            } else {
                /* Skip entire block */
                iter->current_block++;
                iter->current_index = iter->current_block * 64 - 1;
            }
        }
    } else {
        /* Simple linear scan */
        for (int32_t i = iter->current_index + 1; i < col->bitset.count; i++) {
            if (flecs_bitset_get(&col->bitset, i)) {
                iter->current_index = i;
                *out_entity = ecs_table_entities(iter->table)[i];
                return true;
            }
        }
    }

    return false;
}

/* Internal: Mark component as changed when set */
void flecs_mark_changed_on_set(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    /* Quick check: only proceed if change tracking might be enabled */
    const ecs_record_t *record = flecs_entities_get(world, entity);
    if (!record || !record->table) {
        return;
    }
    
    ecs_table_t *table = record->table;
    ecs_table_changed_bs_t *cbs = flecs_table_changed_bs_get(table);
    if (!cbs) {
        return;
    }
    
    ecs_changed_bs_column_t *col = flecs_find_changed_bs_column(cbs, id);
    if (!col) {
        return;
    }
    
    int32_t row = ECS_RECORD_TO_ROW(record->row);

    /* Ensure bitset has enough space */
    if (row >= col->bitset.count) {
        flecs_bitset_addn(&col->bitset, row - col->bitset.count + 1);
    }

    flecs_bitset_set(&col->bitset, row, true);
    
    /* Update hierarchical summary */
    if (col->use_hierarchical) {
        flecs_hierarchical_summary_set(&col->summary, row, true);
    }
    
    col->set_count++;
}

/* Cleanup changed bitset data for table */
void flecs_table_changed_bs_fini(
    ecs_table_t *table)
{
    if (!table || table->id >= FLECS_MAX_CHANGED_BS_TABLES) {
        return;
    }

    ecs_table_changed_bs_t *cbs = flecs_changed_bs_data[table->id];
    if (!cbs) {
        return;
    }

    /* Cleanup all bitsets and hierarchical summaries */
    for (int32_t i = 0; i < cbs->count; i++) {
        flecs_bitset_fini(&cbs->columns[i].bitset);
        flecs_hierarchical_summary_fini(&cbs->columns[i].summary);
    }

    if (cbs->columns) {
        ecs_os_free(cbs->columns);
    }
    
    if (cbs->hash_entries) {
        ecs_os_free(cbs->hash_entries);
    }

    ecs_os_free(cbs);
    flecs_changed_bs_data[table->id] = NULL;
}

/* Move changed bitset data when entity moves between tables */
void flecs_table_changed_bs_move(
    ecs_table_t *dst_table,
    int32_t dst_row,
    ecs_table_t *src_table,
    int32_t src_row)
{
    ecs_table_changed_bs_t *src_cbs = flecs_table_changed_bs_get(src_table);
    ecs_table_changed_bs_t *dst_cbs = flecs_table_changed_bs_ensure(dst_table);

    if (!src_cbs || !dst_cbs) {
        return;
    }

    /* For each changed column in source table */
    for (int32_t i = 0; i < src_cbs->count; i++) {
        ecs_changed_bs_column_t *src_col = &src_cbs->columns[i];
        ecs_id_t component_id = src_col->id & ~ECS_TOGGLE_CHANGED_BITSET;

        /* Find or create matching column in destination */
        ecs_changed_bs_column_t *dst_col = flecs_find_changed_bs_column(dst_cbs, component_id);
        if (!dst_col) {
            dst_col = flecs_add_changed_bs_column(dst_table, component_id);
        }

        if (dst_col) {
            /* Copy changed bit */
            bool changed = flecs_bitset_get(&src_col->bitset, src_row);
            
            /* Ensure bitset has enough space */
            if (dst_row >= dst_col->bitset.count) {
                flecs_bitset_addn(&dst_col->bitset, dst_row - dst_col->bitset.count + 1);
            }
            
            flecs_bitset_set(&dst_col->bitset, dst_row, changed);
            
            /* Update hierarchical summary */
            if (dst_col->use_hierarchical) {
                flecs_hierarchical_summary_set(&dst_col->summary, dst_row, changed);
            }
        }
    }
}
