#include "memory.h"
#include "states.h"
#include <string.h>







internal u64 hash(const char *str) {
    // http://www.cse.yorku.ca/~oz/hash.html  // djb2 by dan bernstein
    u64 hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}
internal int perf_dict_cmp(const void * a, const void * b) {
    const PerfDictEntry *a2 = (const PerfDictEntry *) a;
    const PerfDictEntry *b2 = (const PerfDictEntry *) b;
    return (b2->total_time - a2->total_time);
}

void perf_dict_set(PerfDict *d,  const char *key, u64 add) {
    int index = hash(key) % PERF_DICT_SIZE;

    if (d->data[index].times_counted > 0) {
        ASSERT(d->data[index].key != NULL)
        ASSERT(key != NULL);
        ASSERT("key clash!, needs bigger dictionary." && strcmp(d->data[index].key , key) == 0);
    }

    d->data[index].key = key;
    d->data[index].times_counted++;
    d->data[index].total_time += add;
    if (add < d->data[index].min || d->data[index].min == 0) d->data[index].min = add;
    if (add > d->data[index].max) d->data[index].max = add;


}

PerfDictEntry perf_dict_get(PerfDict *d, char* key) {
    int index = hash(key) % PERF_DICT_SIZE;
    return d->data[index];
}

void perf_dict_reset(PerfDict *d) {
    memset(d->data, 0, sizeof(d->data));
}

void perf_dict_sort_clone(PerfDict *source, PerfDict *clone) {
    memcpy(&clone->data, &source->data, sizeof(source->data));
    qsort(clone->data, PERF_DICT_SIZE, sizeof(PerfDictEntry), perf_dict_cmp);
}



// actors live in a pool, to easily add and remove them into the simulation.
void actor_remove(PermanentState *state, u32 index) {
    if (state->actor_count > 0) {
        // zero out everything
        memset(&state->actors[index], 0, sizeof(state->actors[index]));
        //state->actors[index] = {0};//(Actor) {0,0,{0,0},0,0,0,0.0f}; // kill the data in here, just to be sure.

        // now swap
        Actor temp = state->actors[state->actor_count];
        state->actors[state->actor_count] = state->actors[index];
        state->actors[index] = temp;

        // because paths arent part of the actor struct but but will be found using the id, i need to reswap that data too.
        ActorPath tempPath = state->paths[state->actor_count];
        state->paths[state->actor_count] = state->paths[index];
        state->paths[index] = tempPath;


        state->actor_count--;
    }

}

void actor_add(PermanentState *state) {
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
