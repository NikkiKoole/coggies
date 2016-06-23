#ifndef MEMORY_H
#define MEMORY_H

#include <sys/mman.h> //mmap
#include "types.h"

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

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
    u16 x;
    u16 y;
    u16 z;
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
    MemoryArena arena;
    Wall walls[16384];
    u32 wall_count;
    Actor actors[16384];
    u32 actor_count;
    Glyph glyphs[16384];
    u32 glyph_count;

    u32 world_width;
    u32 world_depth;
    u32 world_height;
} GameState;

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
