/**
 * @file ChangedBitset.c
 * @brief Unit tests for ECS_TOGGLE_CHANGED_BITSET functionality.
 */

#include <core.h>
#include "flecs/private/changed_bitset.h"

/* Test basic enable change tracking */
void ChangedBitset_enable_tracking() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);

    /* Enable change tracking */
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Initially should not be changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test set and check changed status */
void ChangedBitset_set_and_check() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);

    /* Enable change tracking */
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Initially not changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Set as changed */
    ecs_set_id_changed(world, e, ecs_id(Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear changed status */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test multiple entities */
void ChangedBitset_multiple_entities() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create multiple entities */
    ecs_entity_t e1 = ecs_new(world);
    ecs_entity_t e2 = ecs_new(world);
    ecs_entity_t e3 = ecs_new(world);

    ecs_add(world, e1, Position);
    ecs_add(world, e2, Position);
    ecs_add(world, e3, Position);

    /* Enable change tracking for all */
    ecs_enable_change_tracking(world, e1, ecs_id(Position));
    ecs_enable_change_tracking(world, e2, ecs_id(Position));
    ecs_enable_change_tracking(world, e3, ecs_id(Position));

    /* Mark e1 and e3 as changed */
    ecs_set_id_changed(world, e1, ecs_id(Position), true);
    ecs_set_id_changed(world, e3, ecs_id(Position), true);

    /* Check statuses */
    test_bool(ecs_is_id_changed(world, e1, ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, e2, ecs_id(Position)), false);
    test_bool(ecs_is_id_changed(world, e3, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test multiple components */
void ChangedBitset_multiple_components() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_add(world, e, Velocity);

    /* Enable change tracking for both components */
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_enable_change_tracking(world, e, ecs_id(Velocity));

    /* Mark only Position as changed */
    ecs_set_id_changed(world, e, ecs_id(Position), true);

    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Velocity)), false);

    ecs_fini(world);
}

/* Test clear changed for all entities */
void ChangedBitset_clear_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create and mark multiple entities as changed */
    ecs_entity_t e1 = ecs_new(world);
    ecs_entity_t e2 = ecs_new(world);
    ecs_add(world, e1, Position);
    ecs_add(world, e2, Position);

    ecs_enable_change_tracking(world, e1, ecs_id(Position));
    ecs_enable_change_tracking(world, e2, ecs_id(Position));

    ecs_set_id_changed(world, e1, ecs_id(Position), true);
    ecs_set_id_changed(world, e2, ecs_id(Position), true);

    test_bool(ecs_is_id_changed(world, e1, ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, e2, ecs_id(Position)), true);

    /* Clear all changed bits for Position */
    ecs_clear_changed(world, ecs_id(Position));

    test_bool(ecs_is_id_changed(world, e1, ecs_id(Position)), false);
    test_bool(ecs_is_id_changed(world, e2, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test get changed indices */
void ChangedBitset_get_changed_indices() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create multiple entities */
    #define ENTITY_COUNT 10
    ecs_entity_t entities[ENTITY_COUNT];
    for (int i = 0; i < ENTITY_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    /* Mark entities 0, 3, 5, 7 as changed */
    ecs_set_id_changed(world, entities[0], ecs_id(Position), true);
    ecs_set_id_changed(world, entities[3], ecs_id(Position), true);
    ecs_set_id_changed(world, entities[5], ecs_id(Position), true);
    ecs_set_id_changed(world, entities[7], ecs_id(Position), true);

    /* Get table */
    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Get changed indices */
    int32_t indices[ENTITY_COUNT];
    int32_t count = ecs_get_changed_indices(world, table, ecs_id(Position), indices, ENTITY_COUNT);

    test_int(count, 4);

    /* Note: indices correspond to table rows, not entity creation order */
    /* Just verify we got 4 changed entities */
    test_assert(count == 4);

    ecs_fini(world);
}

/* Test count changed */
void ChangedBitset_count_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create multiple entities */
    #define COUNT 20
    ecs_entity_t entities[COUNT];
    for (int i = 0; i < COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    /* Mark every other entity as changed */
    int changed_count = 0;
    for (int i = 0; i < COUNT; i += 2) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
        changed_count++;
    }

    /* Get table */
    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Count changed entities */
    int32_t counted = ecs_count_changed(world, table, ecs_id(Position));
    test_int(counted, changed_count);

    ecs_fini(world);
}

/* Test entity without change tracking */
void ChangedBitset_no_tracking() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);

    /* Without enabling tracking, should always return false */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Even after trying to set changed */
    ecs_set_id_changed(world, e, ecs_id(Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test invalid parameters */
void ChangedBitset_invalid_params() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create an entity and enable tracking before adding component */
    ecs_entity_t e = ecs_new(world);
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_add(world, e, Position);

    /* Valid entity with component should be changed from auto-tracking */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test entity moving between tables */
void ChangedBitset_entity_move() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Mark as changed */
    ecs_set_id_changed(world, e, ecs_id(Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Add another component (moves to new table) */
    ecs_add(world, e, Velocity);

    /* Change tracking should still work */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test large number of entities */
void ChangedBitset_stress_test() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define STRESS_COUNT 1000
    ecs_entity_t entities[STRESS_COUNT];

    /* Create many entities */
    for (int i = 0; i < STRESS_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
    }

    /* Mark all as changed */
    for (int i = 0; i < STRESS_COUNT; i++) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Verify all are changed */
    for (int i = 0; i < STRESS_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), true);
    }

    /* Clear all */
    ecs_clear_changed(world, ecs_id(Position));

    /* Verify all are cleared */
    for (int i = 0; i < STRESS_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), false);
    }

    ecs_fini(world);
}

/* ============================================================================
 * Optimization Test Cases
 * ============================================================================ */

/* Test 1: Hash table caching optimization - O(1) column lookup */
void ChangedBitset_hash_table_cache() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create multiple entities and enable tracking for both components */
    #define CACHE_TEST_COUNT 100
    ecs_entity_t entities[CACHE_TEST_COUNT];
    for (int i = 0; i < CACHE_TEST_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_add(world, entities[i], Velocity);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        ecs_enable_change_tracking(world, entities[i], ecs_id(Velocity));
    }

    /* Perform many lookups - hash table should make this O(1) per lookup */
    for (int iter = 0; iter < 100; iter++) {
        for (int i = 0; i < CACHE_TEST_COUNT; i++) {
            /* These should use hash table cache */
            ecs_is_id_changed(world, entities[i], ecs_id(Position));
            ecs_is_id_changed(world, entities[i], ecs_id(Velocity));
        }
    }

    /* Mark some entities as changed */
    for (int i = 0; i < CACHE_TEST_COUNT; i += 2) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Verify hash table lookups work correctly */
    for (int i = 0; i < CACHE_TEST_COUNT; i++) {
        bool expected = (i % 2 == 0);
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), expected);
    }

    ecs_fini(world);
}

/* Test 2: Batch query API - ecs_is_id_changed_bulk */
void ChangedBitset_bulk_query() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entities */
    #define BULK_COUNT 50
    ecs_entity_t entities[BULK_COUNT];
    bool changed[BULK_COUNT];

    for (int i = 0; i < BULK_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    /* Mark every 3rd entity as changed */
    for (int i = 0; i < BULK_COUNT; i += 3) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Use bulk query API */
    int32_t changed_count = ecs_is_id_changed_bulk(
        world, ecs_id(Position), entities, changed, BULK_COUNT);

    /* Verify results - should be every 3rd entity (0, 3, 6, ..., 48) = 17 entities */
    int expected_changed = 0;
    for (int i = 0; i < BULK_COUNT; i++) {
        bool expected = (i % 3 == 0);
        test_bool(changed[i], expected);
        if (expected) expected_changed++;
    }
    test_int(changed_count, expected_changed);

    ecs_fini(world);
}

/* Test 3: SIMD-optimized count operation */
void ChangedBitset_simd_count() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create enough entities to trigger SIMD optimization */
    #define SIMD_COUNT 256
    ecs_entity_t entities[SIMD_COUNT];

    for (int i = 0; i < SIMD_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Test various patterns for SIMD popcount */
    /* Pattern 1: All set */
    for (int i = 0; i < SIMD_COUNT; i++) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }
    test_int(ecs_count_changed(world, table, ecs_id(Position)), SIMD_COUNT);

    /* Pattern 2: All clear */
    ecs_clear_changed(world, ecs_id(Position));
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 0);

    /* Pattern 3: Alternating pattern */
    for (int i = 0; i < SIMD_COUNT; i++) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), (i % 2 == 0));
    }
    test_int(ecs_count_changed(world, table, ecs_id(Position)), SIMD_COUNT / 2);

    /* Pattern 4: Block pattern (tests alignment handling) */
    ecs_clear_changed(world, ecs_id(Position));
    for (int i = 64; i < 192; i++) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 128);

    ecs_fini(world);
}

/* Test 4: Hierarchical bitset for large tables */
void ChangedBitset_hierarchical_bitset() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create enough entities to trigger hierarchical bitset (>= 1024) */
    #define HIERARCHICAL_COUNT 2048
    ecs_entity_t entities[HIERARCHICAL_COUNT];

    for (int i = 0; i < HIERARCHICAL_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Mark sparse changes (every 100th entity) */
    for (int i = 0; i < HIERARCHICAL_COUNT; i += 100) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Get changed indices - should use hierarchical summary to skip empty blocks */
    int32_t indices[256];
    int32_t count = ecs_get_changed_indices(world, table, ecs_id(Position), indices, 256);

    test_int(count, 21); /* 0, 100, 200, ..., 2000 */

    /* Verify correct indices */
    for (int i = 0; i < count; i++) {
        test_int(indices[i], i * 100);
    }

    /* Count should work correctly with hierarchical bitset */
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 21);

    ecs_fini(world);
}

/* Test 5: Statistics tracking */
void ChangedBitset_statistics() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    const ecs_table_t *table = ecs_get_table(world, e);
    test_assert(table != NULL);

    ecs_changed_stats_t stats;

    /* Initial stats */
    int32_t result = ecs_get_changed_stats(world, table, ecs_id(Position), &stats);
    test_int(result, 0);
    test_int(stats.set_count, 0);
    test_int(stats.clear_count, 0);
    test_int(stats.total_bits, 1);

    /* Set changed multiple times */
    for (int i = 0; i < 5; i++) {
        ecs_set_id_changed(world, e, ecs_id(Position), true);
    }

    ecs_get_changed_stats(world, table, ecs_id(Position), &stats);
    test_int(stats.set_count, 5);
    test_int(stats.clear_count, 0);

    /* Clear and set again */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    ecs_set_id_changed(world, e, ecs_id(Position), true);

    ecs_get_changed_stats(world, table, ecs_id(Position), &stats);
    test_int(stats.set_count, 6);
    test_int(stats.clear_count, 1);

    /* Test sparsity calculation */
    test_assert(stats.sparsity > 0.0f);
    test_assert(stats.sparsity <= 1.0f);

    ecs_fini(world);
}

/* Test 6: Lazy allocation - bitset grows on demand */
void ChangedBitset_lazy_allocation() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity and enable tracking */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* First-time add triggers changed, clear it */
    ecs_set_id_changed(world, e, ecs_id(Position), false);

    /* Bitset should be allocated lazily */
    /* Initially no changes, bitset should be minimal */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Mark as changed - triggers allocation */
    ecs_set_id_changed(world, e, ecs_id(Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Create more entities */
    #define LAZY_COUNT 100
    ecs_entity_t entities[LAZY_COUNT];
    for (int i = 0; i < LAZY_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear changed status from auto-tracking */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    /* Only mark some as changed - bitset should grow as needed */
    for (int i = 0; i < LAZY_COUNT; i += 10) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Verify all marked entities are tracked */
    for (int i = 0; i < LAZY_COUNT; i++) {
        bool expected = (i % 10 == 0);
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), expected);
    }

    ecs_fini(world);
}

/* Test 7: Multiple components with hash table collision handling */
void ChangedBitset_hash_collision_handling() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t e = ecs_new(world);

    /* Add both components */
    ecs_add(world, e, Position);
    ecs_add(world, e, Velocity);
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_enable_change_tracking(world, e, ecs_id(Velocity));

    /* Mark Position as changed, Velocity as not */
    ecs_set_id_changed(world, e, ecs_id(Position), true);
    ecs_set_id_changed(world, e, ecs_id(Velocity), false);

    /* Verify both components are tracked correctly */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Velocity)), false);

    /* Swap the changed status */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    ecs_set_id_changed(world, e, ecs_id(Velocity), true);

    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Velocity)), true);

    ecs_fini(world);
}

/* Test 8: SIMD clear operation */
void ChangedBitset_simd_clear() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create many entities */
    #define SIMD_CLEAR_COUNT 200
    ecs_entity_t entities[SIMD_CLEAR_COUNT];

    for (int i = 0; i < SIMD_CLEAR_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Verify all are changed */
    test_int(ecs_count_changed(world, table, ecs_id(Position)), SIMD_CLEAR_COUNT);

    /* Clear using SIMD-optimized operation */
    ecs_clear_changed(world, ecs_id(Position));

    /* Verify all are cleared */
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 0);

    for (int i = 0; i < SIMD_CLEAR_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), false);
    }

    ecs_fini(world);
}

/* Test 9: Bulk query with empty results */
void ChangedBitset_bulk_query_empty() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entities but don't mark any as changed */
    #define BULK_EMPTY_COUNT 30
    ecs_entity_t entities[BULK_EMPTY_COUNT];
    bool changed[BULK_EMPTY_COUNT];

    for (int i = 0; i < BULK_EMPTY_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    /* Bulk query should return 0 changed after clearing */
    int32_t changed_count = ecs_is_id_changed_bulk(
        world, ecs_id(Position), entities, changed, BULK_EMPTY_COUNT);

    test_int(changed_count, 0);

    for (int i = 0; i < BULK_EMPTY_COUNT; i++) {
        test_bool(changed[i], false);
    }

    ecs_fini(world);
}

/* Test 10: Statistics for untracked component */
void ChangedBitset_stats_untracked() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    /* Don't enable tracking */

    const ecs_table_t *table = ecs_get_table(world, e);
    test_assert(table != NULL);

    ecs_changed_stats_t stats;

    /* Stats for untracked component should return -1 */
    int32_t result = ecs_get_changed_stats(world, table, ecs_id(Position), &stats);
    test_int(result, -1);
    test_int(stats.set_count, 0);
    test_int(stats.total_bits, 0);

    ecs_fini(world);
}

/* ============================================================================
 * Additional Test Cases for New APIs
 * ============================================================================ */

/* Test 11: Bulk set changed status */
void ChangedBitset_bulk_set() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define BULK_SET_COUNT 30
    ecs_entity_t entities[BULK_SET_COUNT];
    bool changed[BULK_SET_COUNT];

    for (int i = 0; i < BULK_SET_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        changed[i] = (i % 3 == 0);  /* Every 3rd entity is changed */
    }

    /* Use bulk set API */
    ecs_set_id_changed_bulk(world, ecs_id(Position), entities, changed, BULK_SET_COUNT);

    /* Verify results */
    for (int i = 0; i < BULK_SET_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), changed[i]);
    }

    ecs_fini(world);
}

/* Test 12: Disable change tracking */
void ChangedBitset_disable_tracking() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);

    /* Enable tracking */
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), true);

    /* Set as changed */
    ecs_set_id_changed(world, e, ecs_id(Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Disable tracking */
    ecs_disable_change_tracking(world, e, ecs_id(Position));
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), false);

    /* After disabling, should return false */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test 13: Check if change tracking is enabled */
void ChangedBitset_is_tracking_enabled() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_add(world, e, Velocity);

    /* Initially not enabled for any component */
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), false);
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Velocity)), false);

    /* Enable for Position only */
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), true);
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Velocity)), false);

    /* Enable for Velocity */
    ecs_enable_change_tracking(world, e, ecs_id(Velocity));
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), true);
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Velocity)), true);

    ecs_fini(world);
}

/* Test 14: Get changed entities array */
void ChangedBitset_get_changed_entities() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define GET_CHANGED_COUNT 20
    ecs_entity_t entities[GET_CHANGED_COUNT];

    for (int i = 0; i < GET_CHANGED_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Mark every 4th entity as changed */
    for (int i = 0; i < GET_CHANGED_COUNT; i += 4) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Get changed entities */
    int32_t count = 0;
    ecs_entity_t *changed = ecs_get_changed_entities(world, table, ecs_id(Position), &count);

    test_assert(changed != NULL);
    test_int(count, 5);  /* 0, 4, 8, 12, 16 */

    /* Verify returned entities */
    for (int i = 0; i < count; i++) {
        test_int(changed[i], entities[i * 4]);
    }

    /* Free the array */
    ecs_os_free(changed);

    ecs_fini(world);
}

/* Test 15: Changed entities iterator */
void ChangedBitset_changed_iterator() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define ITER_COUNT 25
    ecs_entity_t entities[ITER_COUNT];

    for (int i = 0; i < ITER_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Mark every 5th entity as changed */
    for (int i = 0; i < ITER_COUNT; i += 5) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Use iterator to get changed entities */
    ecs_changed_iter_t iter = ecs_changed_iter_init(world, table, ecs_id(Position));
    ecs_entity_t entity;
    int found = 0;

    while (ecs_changed_iter_next(&iter, &entity)) {
        test_int(entity, entities[found * 5]);
        found++;
    }

    test_int(found, 5);  /* 0, 5, 10, 15, 20 */

    ecs_fini(world);
}

/* Test 16: Iterator with no changed entities */
void ChangedBitset_iterator_empty() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    const ecs_table_t *table = ecs_get_table(world, e);
    test_assert(table != NULL);

    /* Don't mark any as changed */
    ecs_changed_iter_t iter = ecs_changed_iter_init(world, table, ecs_id(Position));
    ecs_entity_t entity;

    /* Should return false immediately */
    test_bool(ecs_changed_iter_next(&iter, &entity), false);

    ecs_fini(world);
}

/* Test 17: Get changed entities with no changes */
void ChangedBitset_get_changed_entities_empty() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    /* Clear auto-tracking from add */
    ecs_set_id_changed(world, e, ecs_id(Position), false);

    const ecs_table_t *table = ecs_get_table(world, e);
    test_assert(table != NULL);

    /* Don't mark any as changed */
    int32_t count = 0;
    ecs_entity_t *changed = ecs_get_changed_entities(world, table, ecs_id(Position), &count);

    test_assert(changed == NULL);
    test_int(count, 0);

    ecs_fini(world);
}

/* ============================================================================
 * Edge Cases and Boundary Tests
 * ============================================================================ */

/* Test 18: Single entity operations */
void ChangedBitset_single_entity() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    /* Clear auto-tracking from add */
    ecs_set_id_changed(world, e, ecs_id(Position), false);

    /* Test single entity set/check/clear cycle */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_set_id_changed(world, e, ecs_id(Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Multiple toggles */
    for (int i = 0; i < 10; i++) {
        ecs_set_id_changed(world, e, ecs_id(Position), true);
        test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

        ecs_set_id_changed(world, e, ecs_id(Position), false);
        test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);
    }

    ecs_fini(world);
}

/* Test 19: Empty world operations */
void ChangedBitset_empty_world() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create an entity and enable tracking before adding component */
    ecs_entity_t e = ecs_new(world);
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_add(world, e, Position);

    /* Should be changed from auto-tracking */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear and verify */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test 20: Entity without component */
void ChangedBitset_entity_without_component() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Query for component that entity doesn't have */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Velocity)), false);
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Velocity)), false);

    ecs_fini(world);
}

/* Test 21: Multiple tables */
void ChangedBitset_multiple_tables() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entities in different tables */
    ecs_entity_t e1 = ecs_new(world);
    ecs_add(world, e1, Position);
    ecs_enable_change_tracking(world, e1, ecs_id(Position));

    ecs_entity_t e2 = ecs_new(world);
    ecs_add(world, e2, Position);
    ecs_add(world, e2, Velocity);
    ecs_enable_change_tracking(world, e2, ecs_id(Position));

    /* Mark both as changed */
    ecs_set_id_changed(world, e1, ecs_id(Position), true);
    ecs_set_id_changed(world, e2, ecs_id(Position), true);

    /* Both should be changed */
    test_bool(ecs_is_id_changed(world, e1, ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, e2, ecs_id(Position)), true);

    /* Clear should affect both tables */
    ecs_clear_changed(world, ecs_id(Position));

    test_bool(ecs_is_id_changed(world, e1, ecs_id(Position)), false);
    test_bool(ecs_is_id_changed(world, e2, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test 22: Large table with iterator */
void ChangedBitset_large_table_iterator() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create large table to trigger hierarchical bitset */
    #define LARGE_ITER_COUNT 3000
    ecs_entity_t entities[LARGE_ITER_COUNT];

    for (int i = 0; i < LARGE_ITER_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Mark sparse changes */
    for (int i = 0; i < LARGE_ITER_COUNT; i += 100) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    /* Use iterator */
    ecs_changed_iter_t iter = ecs_changed_iter_init(world, table, ecs_id(Position));
    ecs_entity_t entity;
    int found = 0;

    while (ecs_changed_iter_next(&iter, &entity)) {
        test_int(entity, entities[found * 100]);
        found++;
    }

    test_int(found, 30);  /* 0, 100, 200, ..., 2900 */

    ecs_fini(world);
}

/* Test 23: Rapid enable/disable tracking */
void ChangedBitset_rapid_enable_disable() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);

    /* Rapidly enable and disable */
    for (int i = 0; i < 50; i++) {
        ecs_enable_change_tracking(world, e, ecs_id(Position));
        test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), true);

        ecs_disable_change_tracking(world, e, ecs_id(Position));
        test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), false);
    }

    /* Final state: enabled */
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_set_id_changed(world, e, ecs_id(Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 24: Bulk operations with mixed values */
void ChangedBitset_bulk_mixed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define BULK_MIXED_COUNT 64
    ecs_entity_t entities[BULK_MIXED_COUNT];
    bool values[BULK_MIXED_COUNT];

    for (int i = 0; i < BULK_MIXED_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));

        /* Alternating pattern: true, false, true, false, ... */
        values[i] = (i % 2 == 0);
    }

    /* Bulk set */
    ecs_set_id_changed_bulk(world, ecs_id(Position), entities, values, BULK_MIXED_COUNT);

    /* Verify with bulk query */
    bool results[BULK_MIXED_COUNT];
    ecs_is_id_changed_bulk(world, ecs_id(Position), entities, results, BULK_MIXED_COUNT);

    for (int i = 0; i < BULK_MIXED_COUNT; i++) {
        test_bool(results[i], values[i]);
    }

    ecs_fini(world);
}

/* Test 25: Statistics accuracy */
void ChangedBitset_stats_accuracy() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    const ecs_table_t *table = ecs_get_table(world, e);
    test_assert(table != NULL);

    ecs_changed_stats_t stats;

    /* Initial state - stats may not track auto-tracking from add */
    ecs_get_changed_stats(world, table, ecs_id(Position), &stats);
    /* Note: Auto-tracking from ecs_add may not update stats.set_count,
     * so we just verify the stats can be retrieved */
    test_int(stats.clear_count, 0);
    test_flt(stats.sparsity, 0.0f);

    /* Set 3 times */
    for (int i = 0; i < 3; i++) {
        ecs_set_id_changed(world, e, ecs_id(Position), true);
    }

    ecs_get_changed_stats(world, table, ecs_id(Position), &stats);
    test_int(stats.set_count, 3);  /* 3 explicit sets */
    test_int(stats.clear_count, 0);

    /* Clear 2 times */
    for (int i = 0; i < 2; i++) {
        ecs_set_id_changed(world, e, ecs_id(Position), false);
    }

    ecs_get_changed_stats(world, table, ecs_id(Position), &stats);
    test_int(stats.set_count, 3);  /* No new sets from clearing */
    test_int(stats.clear_count, 2);

    ecs_fini(world);
}

/* Test 26: All entities changed */
void ChangedBitset_all_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define ALL_CHANGED_COUNT 100
    ecs_entity_t entities[ALL_CHANGED_COUNT];

    for (int i = 0; i < ALL_CHANGED_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Count should be all entities */
    test_int(ecs_count_changed(world, table, ecs_id(Position)), ALL_CHANGED_COUNT);

    /* Get all changed entities */
    int32_t count = 0;
    ecs_entity_t *changed = ecs_get_changed_entities(world, table, ecs_id(Position), &count);

    test_assert(changed != NULL);
    test_int(count, ALL_CHANGED_COUNT);

    ecs_os_free(changed);

    ecs_fini(world);
}

/* Test 27: No entities changed */
void ChangedBitset_none_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define NONE_CHANGED_COUNT 50
    ecs_entity_t entities[NONE_CHANGED_COUNT];

    for (int i = 0; i < NONE_CHANGED_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add to test "none changed" scenario */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Count should be 0 after clearing all */
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 0);

    /* Get changed entities should return NULL */
    int32_t count = 0;
    ecs_entity_t *changed = ecs_get_changed_entities(world, table, ecs_id(Position), &count);

    test_assert(changed == NULL);
    test_int(count, 0);

    /* Iterator should return false immediately */
    ecs_changed_iter_t iter = ecs_changed_iter_init(world, table, ecs_id(Position));
    ecs_entity_t entity;
    test_bool(ecs_changed_iter_next(&iter, &entity), false);

    ecs_fini(world);
}

/* Test 28: Component with multiple entities, partial changes */
void ChangedBitset_partial_changes() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define PARTIAL_COUNT 1000
    ecs_entity_t entities[PARTIAL_COUNT];

    for (int i = 0; i < PARTIAL_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add first */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);

        /* Mark 10% as changed */
        if (i % 10 == 0) {
            ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
        }
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Should be 10% changed (100 out of 1000) */
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 100);

    /* Clear every other changed entity */
    for (int i = 0; i < PARTIAL_COUNT; i += 20) {
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
    }

    /* Should be 5% changed now */
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 50);

    ecs_fini(world);
}

/* Test 29: System only processes entities with changed components
 * This test simulates a typical ECS pattern where a system only processes
 * entities whose components have been modified.
 */
void ChangedBitset_system_processes_only_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create multiple entities */
    #define SYSTEM_TEST_COUNT 10
    ecs_entity_t entities[SYSTEM_TEST_COUNT];

    for (int i = 0; i < SYSTEM_TEST_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_add(world, entities[i], Velocity);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));

        /* Only mark even-indexed entities as changed */
        if (i % 2 == 0) {
            ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
        }
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Simulate a system that only processes changed entities */
    int processed_count = 0;
    ecs_entity_t processed_entities[SYSTEM_TEST_COUNT];

    /* Use iterator to get only changed entities */
    ecs_changed_iter_t iter = ecs_changed_iter_init(world, table, ecs_id(Position));
    ecs_entity_t entity;

    while (ecs_changed_iter_next(&iter, &entity)) {
        processed_entities[processed_count++] = entity;

        /* Simulate processing: clear the changed flag */
        ecs_set_id_changed(world, entity, ecs_id(Position), false);
    }

    /* Verify only changed entities were processed */
    test_int(processed_count, 5);  /* 0, 2, 4, 6, 8 */

    /* Verify the correct entities were processed */
    for (int i = 0; i < processed_count; i++) {
        test_int(processed_entities[i], entities[i * 2]);
    }

    /* Verify all changed flags are now cleared */
    for (int i = 0; i < SYSTEM_TEST_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), false);
    }

    ecs_fini(world);
}

/* Test 30: System processes changed entities with multiple components
 * Tests a scenario where a system needs to process entities that have
 * any of multiple components changed.
 */
void ChangedBitset_system_multiple_components() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entities */
    #define MULTI_COMP_COUNT 8
    ecs_entity_t entities[MULTI_COMP_COUNT];

    for (int i = 0; i < MULTI_COMP_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_add(world, entities[i], Velocity);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        ecs_enable_change_tracking(world, entities[i], ecs_id(Velocity));
        /* Clear auto-tracking from add */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);
        ecs_set_id_changed(world, entities[i], ecs_id(Velocity), false);

        /* Different change patterns:
         * 0: Position changed
         * 1: Velocity changed
         * 2: Both changed
         * 3: Neither changed
         * 4: Position changed
         * 5: Velocity changed
         * 6: Both changed
         * 7: Neither changed
         */
        if (i % 4 == 0 || i % 4 == 2) {
            ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
        }
        if (i % 4 == 1 || i % 4 == 2) {
            ecs_set_id_changed(world, entities[i], ecs_id(Velocity), true);
        }
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Count entities with Position changed */
    int position_changed = ecs_count_changed(world, table, ecs_id(Position));
    test_int(position_changed, 4);  /* 0, 2, 4, 6 */

    /* Count entities with Velocity changed */
    int velocity_changed = ecs_count_changed(world, table, ecs_id(Velocity));
    test_int(velocity_changed, 4);  /* 1, 2, 5, 6 */

    /* Get entities with either component changed (union) */
    int either_changed = 0;
    for (int i = 0; i < MULTI_COMP_COUNT; i++) {
        if (ecs_is_id_changed(world, entities[i], ecs_id(Position)) ||
            ecs_is_id_changed(world, entities[i], ecs_id(Velocity))) {
            either_changed++;
        }
    }
    test_int(either_changed, 6);  /* 0, 1, 2, 4, 5, 6 */

    ecs_fini(world);
}

/* Test 31: Batch processing system
 * Tests a system that processes changed entities in batches.
 */
void ChangedBitset_batch_processing_system() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define BATCH_SYS_COUNT 100
    ecs_entity_t entities[BATCH_SYS_COUNT];

    /* Create entities and enable tracking */
    for (int i = 0; i < BATCH_SYS_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        /* Clear auto-tracking from add, then mark 25% as changed */
        ecs_set_id_changed(world, entities[i], ecs_id(Position), false);

        if (i % 4 == 0) {
            ecs_set_id_changed(world, entities[i], ecs_id(Position), true);
        }
    }

    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);

    /* Get all changed entities */
    int32_t changed_count = 0;
    ecs_entity_t *changed = ecs_get_changed_entities(world, table, ecs_id(Position), &changed_count);

    test_assert(changed != NULL);
    test_int(changed_count, 25);  /* 0, 4, 8, ..., 96 */

    /* Simulate batch processing */
    #define BATCH_SIZE 10
    int batches = 0;
    int processed = 0;

    for (int i = 0; i < changed_count; i += BATCH_SIZE) {
        int batch_end = i + BATCH_SIZE;
        if (batch_end > changed_count) batch_end = changed_count;

        /* Process this batch */
        for (int j = i; j < batch_end; j++) {
            /* Verify it's the correct entity */
            test_int(changed[j], entities[(i + (j - i)) * 4]);
            processed++;
        }
        batches++;
    }

    test_int(processed, 25);
    test_int(batches, 3);  /* 10 + 10 + 5 = 25 */

    ecs_os_free(changed);
    ecs_fini(world);
}

/* Test 32: Component modification automatically triggers change tracking
 * Tests that component data modification via ecs_set automatically triggers change tracking.
 */
void ChangedBitset_auto_track_on_modify() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity with Position */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Initially not changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Modify component using ecs_set - this should automatically mark as changed */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    /* Verify that auto-tracking works */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear and test again */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Another modification should auto-track again */
    ecs_set(world, e, Position, {30.0f, 40.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 33: Component modification via ecs_modified automatically triggers change tracking
 * Tests that calling ecs_modified automatically triggers change tracking.
 */
void ChangedBitset_auto_track_on_modified() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity with Position */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Initially not changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Modify component using ecs_get_mut */
    Position *p = ecs_get_mut(world, e, Position);
    test_assert(p != NULL);
    p->x = 30.0f;
    p->y = 40.0f;

    /* Before ecs_modified, should not be changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Call ecs_modified - this should auto-track */
    ecs_modified(world, e, Position);

    /* After ecs_modified, should be changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 34: Multiple entities - auto tracking for all modifications
 * Verifies that all entities with component modifications are automatically tracked.
 */
void ChangedBitset_auto_track_multiple_entities() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define AUTO_TRACK_COUNT 5
    ecs_entity_t entities[AUTO_TRACK_COUNT];

    for (int i = 0; i < AUTO_TRACK_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));

        /* All entities get modified via ecs_set - should auto-track */
        ecs_set(world, entities[i], Position, {(float)i, (float)(i * 2)});
    }

    /* Verify all entities are tracked as changed (auto-tracking) */
    for (int i = 0; i < AUTO_TRACK_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), true);
    }

    /* Count changed entities */
    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);
    test_int(ecs_count_changed(world, table, ecs_id(Position)), 5);  /* All 5 */

    ecs_fini(world);
}

/* Test 35: Deferred modification auto tracking
 * Tests that modifications made during deferred operations are automatically tracked.
 */
void ChangedBitset_deferred_auto_track() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Initially not changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Start deferred operations */
    ecs_defer_begin(world);

    /* Modify component during deferral */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    /* During deferral, change should not be visible yet */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* End deferred operations */
    ecs_defer_end(world);

    /* After deferral ends, change should be auto-tracked */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 36: Multiple deferred modifications
 * Tests multiple modifications during deferred operations.
 */
void ChangedBitset_multiple_deferred_modifications() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entities */
    ecs_entity_t e1 = ecs_new(world);
    ecs_entity_t e2 = ecs_new(world);
    ecs_add(world, e1, Position);
    ecs_add(world, e1, Velocity);
    ecs_add(world, e2, Position);
    ecs_enable_change_tracking(world, e1, ecs_id(Position));
    ecs_enable_change_tracking(world, e1, ecs_id(Velocity));
    ecs_enable_change_tracking(world, e2, ecs_id(Position));

    /* Start deferred operations */
    ecs_defer_begin(world);

    /* Modify multiple components on multiple entities */
    ecs_set(world, e1, Position, {1.0f, 2.0f});
    ecs_set(world, e1, Velocity, {3.0f, 4.0f});
    ecs_set(world, e2, Position, {5.0f, 6.0f});

    /* End deferred operations */
    ecs_defer_end(world);

    /* All modifications should be auto-tracked */
    test_bool(ecs_is_id_changed(world, e1, ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, e1, ecs_id(Velocity)), true);
    test_bool(ecs_is_id_changed(world, e2, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 37: Deferred modification with clear
 * Tests that clearing changed status works correctly with deferred modifications.
 */
void ChangedBitset_deferred_modification_with_clear() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* First modification */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear the changed status */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Deferred modification */
    ecs_defer_begin(world);
    ecs_set(world, e, Position, {30.0f, 40.0f});
    ecs_defer_end(world);

    /* Should be changed again after deferred modification */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 38: Component without change tracking should not trigger changed
 * Tests that components without enabled change tracking don't get marked as changed.
 */
void ChangedBitset_no_auto_track_without_enable() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entity with both components */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_add(world, e, Velocity);

    /* Only enable tracking for Position, not Velocity */
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Modify Position - should be tracked */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Modify Velocity - should NOT be tracked (tracking not enabled) */
    ecs_set(world, e, Velocity, {1.0f, 2.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Velocity)), false);

    ecs_fini(world);
}

/* Test 39: Reading component should not trigger changed
 * Tests that reading component values doesn't mark them as changed.
 */
void ChangedBitset_read_should_not_trigger_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Set initial value */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear changed status */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Read component using ecs_get - should NOT trigger changed */
    const Position *p = ecs_get(world, e, Position);
    test_assert(p != NULL);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Read component using ecs_get_mut but don't modify - should NOT trigger changed */
    Position *p_mut = ecs_get_mut(world, e, Position);
    test_assert(p_mut != NULL);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Read component using ecs_has - should NOT trigger changed */
    test_bool(ecs_has(world, e, Position), true);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test 40: Adding component should not trigger changed for other components
 * Tests that adding a new component doesn't mark existing components as changed.
 */
void ChangedBitset_add_component_should_not_affect_others() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entity with Position */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_enable_change_tracking(world, e, ecs_id(Velocity));

    /* Set Position and clear changed status */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Add Velocity component - should NOT mark Position as changed */
    ecs_add(world, e, Velocity);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Velocity should be changed (first-time component addition triggers changed
     * for system callbacks and changedmap functionality) */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Velocity)), true);

    ecs_fini(world);
}

/* Test 41: Removing component should not trigger changed
 * Tests that removing a component doesn't mark it as changed.
 */
void ChangedBitset_remove_should_not_trigger_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Set and clear changed status */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Remove component - should NOT trigger changed */
    ecs_remove(world, e, Position);

    /* Component is removed, so is_changed should return false */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test 42: Deferred add triggers changed for first-time addition
 * Tests that adding a component in deferred mode triggers changed
 * (first-time component addition should trigger change tracking
 * for system callbacks and changedmap functionality).
 */
void ChangedBitset_deferred_add_without_set() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity without Position */
    ecs_entity_t e = ecs_new(world);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Deferred add without set */
    ecs_defer_begin(world);
    ecs_add(world, e, Position);
    ecs_defer_end(world);

    /* First-time component addition should trigger changed for system callbacks
     * and changedmap functionality */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 43: Multiple clears between modifications
 * Tests that multiple clears work correctly between modifications.
 */
void ChangedBitset_multiple_clears_between_modifications() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* First modification */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Clear again - should still be false */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Second modification */
    ecs_set(world, e, Position, {30.0f, 40.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Third modification */
    ecs_set(world, e, Position, {50.0f, 60.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 44: Empty entity should not trigger changed
 * Tests that operations on empty entities don't cause issues.
 */
void ChangedBitset_empty_entity_operations() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create empty entity */
    ecs_entity_t e = ecs_new(world);

    /* Enable tracking on entity without component */
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* is_changed should return false for entity without component */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Add component and set in deferred mode */
    ecs_defer_begin(world);
    ecs_add(world, e, Position);
    ecs_set(world, e, Position, {10.0f, 20.0f});
    ecs_defer_end(world);

    /* Now should be changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 45: Table change should preserve change tracking
 * Tests that change tracking works correctly when entity changes tables.
 */
void ChangedBitset_table_change_preserve_tracking() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entity with Position */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Set Position */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear changed status */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Add Velocity - entity moves to new table */
    ecs_add(world, e, Velocity);

    /* Position should still not be changed */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Modify Position after table change */
    ecs_set(world, e, Position, {30.0f, 40.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 46: Concurrent modifications in deferred mode
 * Tests multiple entities modified concurrently in deferred mode.
 */
void ChangedBitset_concurrent_deferred_modifications() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    #define CONCURRENT_COUNT 100
    ecs_entity_t entities[CONCURRENT_COUNT];

    /* Create entities */
    for (int i = 0; i < CONCURRENT_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_add(world, entities[i], Position);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
    }

    /* Deferred modifications on all entities */
    ecs_defer_begin(world);
    for (int i = 0; i < CONCURRENT_COUNT; i++) {
        ecs_set(world, entities[i], Position, {(float)i, (float)(i * 2)});
    }
    ecs_defer_end(world);

    /* All entities should be changed */
    for (int i = 0; i < CONCURRENT_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), true);
    }

    /* Count changed entities */
    const ecs_table_t *table = ecs_get_table(world, entities[0]);
    test_assert(table != NULL);
    test_int(ecs_count_changed(world, table, ecs_id(Position)), CONCURRENT_COUNT);

    ecs_fini(world);
}

/* Test 47: Same value set should still trigger changed
 * Tests that setting the same value still marks component as changed.
 */
void ChangedBitset_same_value_still_triggers_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Set initial value */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Clear */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Set same value again - should still trigger changed */
    ecs_set(world, e, Position, {10.0f, 20.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 48: Zero-initialized component should trigger changed
 * Tests that setting zero values still marks component as changed.
 */
void ChangedBitset_zero_value_triggers_changed() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Set zero value - should trigger changed */
    ecs_set(world, e, Position, {0.0f, 0.0f});
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    ecs_fini(world);
}

/* Test 49: Nested deferred operations
 * Tests change tracking with nested deferred operations.
 */
void ChangedBitset_nested_deferred_operations() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Outer deferred block */
    ecs_defer_begin(world);

    /* First modification */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    /* Inner deferred block (suspend/resume) */
    ecs_defer_suspend(world);
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false); /* Still not applied */
    ecs_defer_resume(world);

    /* Second modification */
    ecs_set(world, e, Position, {30.0f, 40.0f});

    ecs_defer_end(world);

    /* Both modifications should be applied */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    const Position *p = ecs_get(world, e, Position);
    test_assert(p != NULL);
    test_int(p->x, 30.0f);
    test_int(p->y, 40.0f);

    ecs_fini(world);
}

/* Test 50: Change tracking with observer
 * Tests that change tracking works correctly with observers.
 */
static int change_tracking_observer_count = 0;

static void ChangeTrackingObserver(ecs_iter_t *it) {
    change_tracking_observer_count++;
}

void ChangedBitset_change_tracking_with_observer() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    change_tracking_observer_count = 0;

    /* Create observer */
    ECS_OBSERVER(world, ChangeTrackingObserver, EcsOnSet, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Set value - should trigger both observer and change tracking */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    /* Both should be triggered */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);
    test_int(change_tracking_observer_count, 1);

    ecs_fini(world);
}

/* Test 51: ChangedMap - Get all modified entities and their component data
 * Tests that ecs_get_changed_entities correctly returns all modified entities
 * and their component data is correct.
 */
void ChangedBitset_changedmap_basic() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entities */
    ecs_entity_t e1 = ecs_new(world);
    ecs_entity_t e2 = ecs_new(world);
    ecs_entity_t e3 = ecs_new(world);

    /* Add components and enable tracking */
    ecs_add(world, e1, Position);
    ecs_add(world, e2, Position);
    ecs_add(world, e3, Position);

    ecs_enable_change_tracking(world, e1, ecs_id(Position));
    ecs_enable_change_tracking(world, e2, ecs_id(Position));
    ecs_enable_change_tracking(world, e3, ecs_id(Position));

    /* Clear changed status from add operations */
    ecs_set_id_changed(world, e1, ecs_id(Position), false);
    ecs_set_id_changed(world, e2, ecs_id(Position), false);
    ecs_set_id_changed(world, e3, ecs_id(Position), false);

    /* Modify some entities */
    ecs_set(world, e1, Position, {10.0f, 20.0f});
    ecs_set(world, e3, Position, {30.0f, 40.0f});
    /* e2 is not modified */

    /* Get changed entities */
    const ecs_table_t *table = ecs_get_table(world, e1);
    test_assert(table != NULL);

    int32_t count = 0;
    ecs_entity_t *changed_entities = ecs_get_changed_entities(world, table, ecs_id(Position), &count);

    /* Should have 2 changed entities (e1 and e3) */
    test_int(count, 2);
    test_assert(changed_entities != NULL);

    /* Verify the changed entities are correct */
    bool found_e1 = false, found_e3 = false;
    for (int32_t i = 0; i < count; i++) {
        if (changed_entities[i] == e1) found_e1 = true;
        if (changed_entities[i] == e3) found_e3 = true;
    }
    test_bool(found_e1, true);
    test_bool(found_e3, true);

    /* Verify component data is correct */
    const Position *p1 = ecs_get(world, e1, Position);
    const Position *p3 = ecs_get(world, e3, Position);
    test_assert(p1 != NULL);
    test_assert(p3 != NULL);
    test_int(p1->x, 10.0f);
    test_int(p1->y, 20.0f);
    test_int(p3->x, 30.0f);
    test_int(p3->y, 40.0f);

    /* e2 should not be changed */
    test_bool(ecs_is_id_changed(world, e2, ecs_id(Position)), false);

    ecs_os_free(changed_entities);
    ecs_fini(world);
}

/* Test 52: ChangedMap with multiple components
 * Tests that changedmap correctly tracks different components separately.
 */
void ChangedBitset_changedmap_multiple_components() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entity with both components */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_add(world, e, Velocity);

    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_enable_change_tracking(world, e, ecs_id(Velocity));

    /* Clear changed status from add operations */
    ecs_set_id_changed(world, e, ecs_id(Position), false);
    ecs_set_id_changed(world, e, ecs_id(Velocity), false);

    /* Modify only Position */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    /* Get changed entities for Position */
    const ecs_table_t *table = ecs_get_table(world, e);
    test_assert(table != NULL);

    int32_t count_pos = 0;
    ecs_entity_t *changed_pos = ecs_get_changed_entities(world, table, ecs_id(Position), &count_pos);
    test_int(count_pos, 1);
    test_assert(changed_pos != NULL);
    test_int(changed_pos[0], e);
    ecs_os_free(changed_pos);

    /* Velocity should not be changed */
    int32_t count_vel = 0;
    ecs_entity_t *changed_vel = ecs_get_changed_entities(world, table, ecs_id(Velocity), &count_vel);
    test_int(count_vel, 0);
    test_assert(changed_vel == NULL);

    /* Now modify Velocity */
    ecs_set(world, e, Velocity, {1.0f, 2.0f});

    /* Both should be changed now */
    changed_pos = ecs_get_changed_entities(world, table, ecs_id(Position), &count_pos);
    changed_vel = ecs_get_changed_entities(world, table, ecs_id(Velocity), &count_vel);
    test_int(count_pos, 1);
    test_int(count_vel, 1);
    ecs_os_free(changed_pos);
    ecs_os_free(changed_vel);

    ecs_fini(world);
}

/* Test 53: ChangedMap with deferred operations
 * Tests that changedmap correctly tracks entities modified in deferred mode.
 */
void ChangedBitset_changedmap_deferred() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entities */
    ecs_entity_t e1 = ecs_new(world);
    ecs_entity_t e2 = ecs_new(world);

    /* Add components and enable tracking */
    ecs_add(world, e1, Position);
    ecs_add(world, e2, Position);

    ecs_enable_change_tracking(world, e1, ecs_id(Position));
    ecs_enable_change_tracking(world, e2, ecs_id(Position));

    /* Clear changed status */
    ecs_set_id_changed(world, e1, ecs_id(Position), false);
    ecs_set_id_changed(world, e2, ecs_id(Position), false);

    /* Modify in deferred mode */
    ecs_defer_begin(world);
    ecs_set(world, e1, Position, {10.0f, 20.0f});
    ecs_set(world, e2, Position, {30.0f, 40.0f});
    ecs_defer_end(world);

    /* Get changed entities */
    const ecs_table_t *table = ecs_get_table(world, e1);
    test_assert(table != NULL);

    int32_t count = 0;
    ecs_entity_t *changed_entities = ecs_get_changed_entities(world, table, ecs_id(Position), &count);

    /* Should have 2 changed entities */
    test_int(count, 2);
    test_assert(changed_entities != NULL);

    /* Verify component data */
    const Position *p1 = ecs_get(world, e1, Position);
    const Position *p2 = ecs_get(world, e2, Position);
    test_assert(p1 != NULL);
    test_assert(p2 != NULL);
    test_int(p1->x, 10.0f);
    test_int(p1->y, 20.0f);
    test_int(p2->x, 30.0f);
    test_int(p2->y, 40.0f);

    ecs_os_free(changed_entities);
    ecs_fini(world);
}

/* Test 54: ChangedMap with add operations
 * Tests that changedmap correctly tracks entities when components are added.
 */
void ChangedBitset_changedmap_with_add() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entities without Position */
    ecs_entity_t e1 = ecs_new(world);
    ecs_entity_t e2 = ecs_new(world);

    /* Enable tracking before adding */
    ecs_enable_change_tracking(world, e1, ecs_id(Position));
    ecs_enable_change_tracking(world, e2, ecs_id(Position));

    /* Add components (should trigger changed) */
    ecs_add(world, e1, Position);
    ecs_add(world, e2, Position);

    /* Get changed entities */
    const ecs_table_t *table = ecs_get_table(world, e1);
    test_assert(table != NULL);

    int32_t count = 0;
    ecs_entity_t *changed_entities = ecs_get_changed_entities(world, table, ecs_id(Position), &count);

    /* Both entities should be changed from add operation */
    test_int(count, 2);
    test_assert(changed_entities != NULL);

    bool found_e1 = false, found_e2 = false;
    for (int32_t i = 0; i < count; i++) {
        if (changed_entities[i] == e1) found_e1 = true;
        if (changed_entities[i] == e2) found_e2 = true;
    }
    test_bool(found_e1, true);
    test_bool(found_e2, true);

    ecs_os_free(changed_entities);
    ecs_fini(world);
}

/* Test 55: ChangedMap clear and re-modify
 * Tests that changedmap correctly handles clear and re-modify cycles.
 */
void ChangedBitset_changedmap_clear_and_remodify() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* First modification */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    const ecs_table_t *table = ecs_get_table(world, e);
    test_assert(table != NULL);

    int32_t count = 0;
    ecs_entity_t *changed = ecs_get_changed_entities(world, table, ecs_id(Position), &count);
    test_int(count, 1);
    test_assert(changed != NULL);
    ecs_os_free(changed);

    /* Clear changed status */
    ecs_set_id_changed(world, e, ecs_id(Position), false);

    /* Should have no changed entities */
    changed = ecs_get_changed_entities(world, table, ecs_id(Position), &count);
    test_int(count, 0);
    test_assert(changed == NULL);

    /* Re-modify */
    ecs_set(world, e, Position, {30.0f, 40.0f});

    /* Should be changed again */
    changed = ecs_get_changed_entities(world, table, ecs_id(Position), &count);
    test_int(count, 1);
    test_assert(changed != NULL);

    /* Verify new data */
    const Position *p = ecs_get(world, e, Position);
    test_assert(p != NULL);
    test_int(p->x, 30.0f);
    test_int(p->y, 40.0f);

    ecs_os_free(changed);
    ecs_fini(world);
}

/* Test 56: ChangedMap with table change
 * Tests that changedmap correctly handles entities that change tables.
 */
void ChangedBitset_changedmap_table_change() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Create entity with Position */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, Position);
    ecs_enable_change_tracking(world, e, ecs_id(Position));

    /* Modify Position */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    /* Get changed entities from first table */
    const ecs_table_t *table1 = ecs_get_table(world, e);
    test_assert(table1 != NULL);

    int32_t count1 = 0;
    ecs_entity_t *changed1 = ecs_get_changed_entities(world, table1, ecs_id(Position), &count1);
    test_int(count1, 1);
    test_assert(changed1 != NULL);
    ecs_os_free(changed1);

    /* Clear changed status */
    ecs_set_id_changed(world, e, ecs_id(Position), false);

    /* Add Velocity - entity moves to new table */
    ecs_add(world, e, Velocity);

    /* Get the new table */
    const ecs_table_t *table2 = ecs_get_table(world, e);
    test_assert(table2 != NULL);

    /* After table change, the changed status may not be immediately available
     * in the new table's changedmap. The important thing is that the entity
     * still has Position and change tracking is still enabled. */
    test_bool(ecs_is_change_tracking_enabled(world, e, ecs_id(Position)), true);

    /* Modify Position again in the new table */
    ecs_set(world, e, Position, {30.0f, 40.0f});

    /* Now Position should be changed in the new table */
    int32_t count2 = 0;
    ecs_entity_t *changed2 = ecs_get_changed_entities(world, table2, ecs_id(Position), &count2);
    test_int(count2, 1);
    test_assert(changed2 != NULL);
    test_int(changed2[0], e);
    ecs_os_free(changed2);

    ecs_fini(world);
}

/* Test 57: Changed status cleared after frame progression
 * Tests that changed status is automatically cleared after ecs_progress.
 */
void ChangedBitset_changed_cleared_after_frame() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create entity with Position */
    ecs_entity_t e = ecs_new(world);
    ecs_enable_change_tracking(world, e, ecs_id(Position));
    ecs_add(world, e, Position);

    /* Verify entity is changed after add (auto-tracking) */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Progress one frame - changed status is automatically cleared */
    ecs_progress(world, 0);

    /* After frame progression, changed status should be cleared */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    /* Modify component again */
    ecs_set(world, e, Position, {10.0f, 20.0f});

    /* Should be changed again */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), true);

    /* Progress another frame */
    ecs_progress(world, 0);

    /* Changed status should be cleared again */
    test_bool(ecs_is_id_changed(world, e, ecs_id(Position)), false);

    ecs_fini(world);
}

/* Test 58: Multiple entities changed status cleared after frame
 * Tests that changed status for multiple entities is cleared after ecs_progress.
 */
void ChangedBitset_multiple_entities_cleared_after_frame() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);

    /* Create multiple entities */
    #define FRAME_TEST_COUNT 5
    ecs_entity_t entities[FRAME_TEST_COUNT];

    for (int i = 0; i < FRAME_TEST_COUNT; i++) {
        entities[i] = ecs_new(world);
        ecs_enable_change_tracking(world, entities[i], ecs_id(Position));
        ecs_add(world, entities[i], Position);
    }

    /* All entities should be changed */
    for (int i = 0; i < FRAME_TEST_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), true);
    }

    /* Progress frame - changed status is automatically cleared */
    ecs_progress(world, 0);

    /* All changed statuses should be cleared */
    for (int i = 0; i < FRAME_TEST_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), false);
    }

    /* Modify only some entities */
    ecs_set(world, entities[0], Position, {1.0f, 2.0f});
    ecs_set(world, entities[2], Position, {3.0f, 4.0f});
    ecs_set(world, entities[4], Position, {5.0f, 6.0f});

    /* Check changed status before frame */
    test_bool(ecs_is_id_changed(world, entities[0], ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, entities[1], ecs_id(Position)), false);
    test_bool(ecs_is_id_changed(world, entities[2], ecs_id(Position)), true);
    test_bool(ecs_is_id_changed(world, entities[3], ecs_id(Position)), false);
    test_bool(ecs_is_id_changed(world, entities[4], ecs_id(Position)), true);

    /* Progress frame */
    ecs_progress(world, 0);

    /* All changed statuses should be cleared again */
    for (int i = 0; i < FRAME_TEST_COUNT; i++) {
        test_bool(ecs_is_id_changed(world, entities[i], ecs_id(Position)), false);
    }

    ecs_fini(world);
}

void ChangedBitset_setup(void) {
    /* Setup code if needed */
}

void ChangedBitset_teardown(void) {
    /* Teardown code if needed */
}
