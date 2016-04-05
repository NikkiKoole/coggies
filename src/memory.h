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
    int x;
    int y;
    int z;
    int frame;
} Wall;

typedef struct {
    int x;
    int y;
    int z;
    int frame;
    int dx;
    int dy;
    float palette_index;
} Actor;

typedef struct {
    MemoryArena arena;
    Wall walls[2048];
    Actor actors[2048];
} GameState;


#define PUSH_STRUCT(arena, type) (type *) push_size_(arena, sizeof(type))
#define PUSH_ARRAY(arena, count, type) (type *) push_size_(arena, (count) * sizeof(type))
#define PUSH_SIZE(arena, size) push_size_(arena, size)

void reserve_memory(Memory *m);
void *push_size_(MemoryArena *arena, memory_index size);
void initialize_arena(MemoryArena *arena, memory_index size, u8 *Base);
void end_temporary_memory(TempMemory temp);
TempMemory begin_temporary_memory(MemoryArena *arena);



#endif
