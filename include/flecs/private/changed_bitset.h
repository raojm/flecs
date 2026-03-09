/**
 * @file changed_bitset.h
 * @brief Changed bitset API for entity-level change tracking.
 *
 * This module provides ECS_TOGGLE_CHANGED_BITSET functionality for tracking
 * component changes at the entity level using bitsets.
 */

#ifndef FLECS_CHANGED_BITSET_H
#define FLECS_CHANGED_BITSET_H

#include "api_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup changed_bitset Changed Bitset
 * @brief Entity-level change tracking using bitsets
 * @{
 */

/** Id flag for changed bitset tracking */
extern const ecs_id_t ECS_TOGGLE_CHANGED_BITSET;

/**
 * @brief Set component changed status for an entity.
 *
 * @param world The world.
 * @param entity The entity.
 * @param id The component id.
 * @param changed True to mark as changed, false to mark as unchanged.
 */
FLECS_API
void ecs_set_id_changed(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id,
    bool changed);

/**
 * @brief Test if component is changed for an entity.
 *
 * @param world The world.
 * @param entity The entity.
 * @param id The component id.
 * @return True if the component is changed, otherwise false.
 */
FLECS_API
bool ecs_is_id_changed(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id);

/**
 * @brief Clear all changed bits for a component across all entities.
 *
 * @param world The world.
 * @param id The component id.
 */
FLECS_API
void ecs_clear_changed(
    ecs_world_t *world,
    ecs_id_t id);

/**
 * @brief Get array of changed entity indices for a component in a table.
 *
 * @param world The world.
 * @param table The table.
 * @param id The component id.
 * @param out_indices Output array for changed indices (must be pre-allocated).
 * @param max_count Maximum number of indices to return.
 * @return Number of changed entities found.
 */
FLECS_API
int32_t ecs_get_changed_indices(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id,
    int32_t *out_indices,
    int32_t max_count);

/**
 * @brief Count changed entities for a component in a table.
 *
 * @param world The world.
 * @param table The table.
 * @param id The component id.
 * @return Number of changed entities.
 */
FLECS_API
int32_t ecs_count_changed(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id);

/**
 * @brief Enable change tracking for a component on an entity.
 *
 * This adds the changed bitset column to the entity's table.
 *
 * @param world The world.
 * @param entity The entity.
 * @param id The component id.
 */
FLECS_API
void ecs_enable_change_tracking(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id);

/**
 * @brief Bulk query for multiple entities' changed status.
 *
 * Efficiently checks changed status for multiple entities in a batch.
 * This is optimized for performance using SIMD operations.
 *
 * @param world The world.
 * @param id The component id.
 * @param entities Array of entity ids to check.
 * @param changed Array to store results (must be pre-allocated, same size as entities).
 * @param count Number of entities.
 * @return Number of entities that are changed.
 */
FLECS_API
int32_t ecs_is_id_changed_bulk(
    const ecs_world_t *world,
    ecs_id_t id,
    const ecs_entity_t *entities,
    bool *changed,
    int32_t count);

/**
 * @brief Statistics for changed bitset tracking.
 */
typedef struct ecs_changed_stats_t {
    int32_t set_count;       /* Number of set operations */
    int32_t clear_count;     /* Number of clear operations */
    float sparsity;          /* Ratio of set bits to total bits (0.0 - 1.0) */
    int32_t total_bits;      /* Total number of bits tracked */
} ecs_changed_stats_t;

/**
 * @brief Get statistics for changed bitset tracking.
 *
 * @param world The world.
 * @param table The table (NULL for all tables).
 * @param id The component id.
 * @param stats Output statistics structure.
 * @return 0 on success, -1 on failure.
 */
FLECS_API
int32_t ecs_get_changed_stats(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id,
    ecs_changed_stats_t *stats);

/**
 * @brief Bulk set changed status for multiple entities.
 *
 * Efficiently sets changed status for multiple entities in a batch.
 *
 * @param world The world.
 * @param id The component id.
 * @param entities Array of entity ids.
 * @param changed Array of changed values (true/false for each entity).
 * @param count Number of entities.
 */
FLECS_API
void ecs_set_id_changed_bulk(
    ecs_world_t *world,
    ecs_id_t id,
    const ecs_entity_t *entities,
    const bool *changed,
    int32_t count);

/**
 * @brief Disable change tracking for a component on an entity.
 *
 * @param world The world.
 * @param entity The entity.
 * @param id The component id.
 */
FLECS_API
void ecs_disable_change_tracking(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id);

/**
 * @brief Check if change tracking is enabled for a component.
 *
 * @param world The world.
 * @param entity The entity.
 * @param id The component id.
 * @return True if tracking is enabled, false otherwise.
 */
FLECS_API
bool ecs_is_change_tracking_enabled(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id);

/**
 * @brief Get all changed entities for a component in a table.
 *
 * Returns an array of entity ids that have the changed flag set.
 * The caller is responsible for freeing the returned array.
 *
 * @param world The world.
 * @param table The table.
 * @param id The component id.
 * @param out_count Output parameter for the number of entities.
 * @return Array of entity ids (must be freed by caller), or NULL on error.
 */
FLECS_API
ecs_entity_t* ecs_get_changed_entities(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id,
    int32_t *out_count);

/**
 * @brief Iterator for changed entities.
 *
 * Provides an efficient way to iterate over all changed entities
 * without allocating a large array.
 */
typedef struct ecs_changed_iter_t {
    const ecs_world_t *world;
    const ecs_table_t *table;
    ecs_id_t id;
    int32_t current_index;
    int32_t total_count;
    bool use_hierarchical;
    int32_t current_block;
} ecs_changed_iter_t;

/**
 * @brief Initialize a changed entity iterator.
 *
 * @param world The world.
 * @param table The table.
 * @param id The component id.
 * @return Iterator structure.
 */
FLECS_API
ecs_changed_iter_t ecs_changed_iter_init(
    const ecs_world_t *world,
    const ecs_table_t *table,
    ecs_id_t id);

/**
 * @brief Get the next changed entity from the iterator.
 *
 * @param iter The iterator.
 * @param out_entity Output parameter for the entity id.
 * @return True if an entity was returned, false if no more entities.
 */
FLECS_API
bool ecs_changed_iter_next(
    ecs_changed_iter_t *iter,
    ecs_entity_t *out_entity);

/** @} */

/* Internal functions - not part of public API */

/**
 * @brief Cleanup changed bitset data for a table.
 * @param table The table being cleaned up.
 */
void flecs_table_changed_bs_fini(
    ecs_table_t *table);

/**
 * @brief Move changed bitset data when entity moves between tables.
 * @param dst_table Destination table.
 * @param dst_row Destination row.
 * @param src_table Source table.
 * @param src_row Source row.
 */
void flecs_table_changed_bs_move(
    ecs_table_t *dst_table,
    int32_t dst_row,
    ecs_table_t *src_table,
    int32_t src_row);

/**
 * @brief Mark component as changed when set (internal).
 * @param world The world.
 * @param entity The entity.
 * @param id The component id.
 */
void flecs_mark_changed_on_set(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id);

/**
 * @brief Clear all changed bits for all components (called at frame end).
 * @param world The world.
 */
void flecs_clear_all_changed(
    ecs_world_t *world);

#ifdef __cplusplus
}
#endif

#endif
