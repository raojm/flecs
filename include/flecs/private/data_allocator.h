/**
 * @file private/data_allocator.h
 * @brief Forward definition of the custom data allocator struct.
 *
 * This header exists so that flecs/datastructures/vec.h can declare the
 * ecs_vec_*_d API without creating a circular include with
 * flecs/datastructures/allocator.h.
 */

#ifndef FLECS_DATA_ALLOCATOR_H
#define FLECS_DATA_ALLOCATOR_H

#include "api_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Custom data allocator for component column memory.
 * When configured on a world, all component column allocations (Position[],
 * Velocity[], etc.) and the entity id array are routed through this allocator
 * instead of the default world allocator. This keeps Flecs metadata in the
 * default allocator while allowing component data to live in a separate memory
 * region (e.g., a shared memory pool that can be mapped read-only by clients).
 *
 * All callbacks must be provided. contains_fn is used internally to decide
 * whether an existing vector array was allocated from the data pool.
 */
typedef struct ecs_data_allocator_t {
    void* (*malloc_fn)(void *ctx, ecs_size_t size);
    void (*free_fn)(void *ctx, void *ptr);
    void* (*realloc_fn)(void *ctx, void *ptr, ecs_size_t size);
    void* (*calloc_fn)(void *ctx, ecs_size_t size);
    bool (*contains_fn)(void *ctx, const void *ptr); /**< Optional ownership test. */
    void *ctx;
} ecs_data_allocator_t;

#ifdef __cplusplus
}
#endif

#endif
