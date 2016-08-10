#include "memory.h"


// actors live in a pool, to easily add and remove them into the simulation.
void actor_remove(GameState *state, u32 index) {
    if (state->actor_count > 0) {
        state->actors[index] = (Actor) {0,0,0,0,0,0,0.0f}; // kill the data in here, just to be sure.

        // now swap
        Actor temp = state->actors[state->actor_count];
        state->actors[state->actor_count] = state->actors[index];
        state->actors[index] = temp;
        state->actor_count--;
    }

}

void actor_add(GameState *state) {
    state->actor_count++;
}


void reserve_memory(Memory *m) {
    void *base_address = (void *)GIGABYTES(0);
    m->permanent_size = MEGABYTES(16);
    m->scratch_size = MEGABYTES(16);

    u64 total_storage_size = m->permanent_size + m->scratch_size;
    m->permanent = mmap(base_address, total_storage_size,
                        PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE,
                        -1, 0);
    m->scratch = (u8 *)(m->permanent) + m->permanent_size;
    m->is_initialized = false;
}


TempMemory begin_temporary_memory(MemoryArena *arena) {
    TempMemory result = {.used = arena->used, .arena = arena};
    return result;
}

void end_temporary_memory(TempMemory temp) {
    MemoryArena *arena = temp.arena;
    ASSERT(arena->used >= temp.used);
    arena->used = temp.used;
}

void *push_size_(MemoryArena *arena, memory_index size) {
    memory_index allignment = 4;
    memory_index result_pointer = (memory_index)arena->base + arena->used;
    memory_index allignment_offset = 0;
    memory_index allignment_mask = allignment - 1;

    if (result_pointer & allignment_mask) {
        allignment_offset = allignment - (result_pointer & allignment_mask);
    }

    size += allignment_offset;

    ASSERT(arena->used + size <= arena->size);
    arena->used += size;

    void *result = (void *)(result_pointer + allignment_offset);
    return result;
}

void initialize_arena(MemoryArena *arena, memory_index size, u8 *base) {
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}
