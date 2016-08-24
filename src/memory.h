#ifndef MEMORY_H
#define MEMORY_H

#include <sys/mman.h> //mmap
#include "types.h"
#include <sys/stat.h>   //struct stat

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

#define PERF_DICT_SIZE 64
typedef struct  {
    const char* key;
    int times_counted;
    u64 total_time;
    u64 min;
    u64 max;
} PerfDictEntry;

typedef struct {
    PerfDictEntry data[PERF_DICT_SIZE];
} PerfDict;;




#define BEGIN_PERFORMANCE_COUNTER(name) u64 name##_begin = SDL_GetPerformanceCounter()
#define END_PERFORMANCE_COUNTER(name)  u64 name##_end = SDL_GetPerformanceCounter();perf_dict_set(perf_dict, #name, name##_end - name##_begin);



typedef struct Shared_Library
{
    void *handle;
    const char *name;
    intmax_t creation_time;
    u32 id;
    u32 size;
    const char *fn_name;
    struct stat stats;
} Shared_Library;


typedef size_t memory_index;

typedef struct {
    b32 is_initialized;
    u32 permanent_size;
    void *permanent;
    u32 scratch_size;
    void *scratch;
} Memory;

typedef struct {
    memory_index size;
    u8 *base;
    memory_index used;
} MemoryArena;

typedef struct {
    MemoryArena *arena;
    memory_index used;
} TempMemory;

typedef struct {
    MemoryArena scratch_arena;
} TransState;

typedef struct {
    u16 x;
    u16 y;
    u16 z;
    u16 frame;
} Wall;

typedef struct {
    r32 x;
    r32 y;
    r32 z;
    u16 frame;
    s16 dx;
    s16 dy;
    float palette_index;
} Actor;

typedef struct {
    u16 x;
    u16 y;
    u16 sx;
    u16 sy;
    u16 w;
    u16 h;
} Glyph;


typedef struct {
    u32 x;
    u32 y;
    u32 z_level;
} WorldDims;

typedef struct {
    MemoryArena arena;
    Wall walls[16384];
    u32 wall_count;
    Actor actors[16384*4];
    u32 actor_count;
    Glyph glyphs[16384];
    u32 glyph_count;
    ///
    s32 x_view_offset;
    s32 y_view_offset;
    /////
    WorldDims dims;
    WorldDims block_size;
    ////

} GameState;


void perf_dict_set(PerfDict *d,  const char *key, u64 add);
PerfDictEntry perf_dict_get(PerfDict *d, char* key);
void perf_dict_reset(PerfDict *d);
void perf_dict_sort_clone(PerfDict *source, PerfDict *clone);

void actor_remove(GameState *state, u32 index);
void actor_add(GameState *state);

#define PUSH_STRUCT(arena, type) (type *) push_size_(arena, sizeof(type))
#define PUSH_ARRAY(arena, count, type) (type *) push_size_(arena, (count) * sizeof(type))
#define PUSH_SIZE(arena, size) push_size_(arena, size)

void reserve_memory(Memory *m);
void *push_size_(MemoryArena *arena, memory_index size);
void initialize_arena(MemoryArena *arena, memory_index size, u8 *Base);
void end_temporary_memory(TempMemory temp);
TempMemory begin_temporary_memory(MemoryArena *arena);



#endif
