#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h> //mmap


typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef bool b32;

typedef size_t memory_index;

#define internal static

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

#define ASSERT(expression) if(!(expression)) {printf("\nASSERT FAIL: '%s' in function %s on line:%d (%s)\n\n",#expression, __FUNCTION__, __LINE__, __FILE__);exit(0);}

typedef struct memory
{
    b32 IsInitialized;
    u32 PermanentStorageSize;
    void *PermanentStorage;
    u32 ScratchStorageSize;
    void *ScratchStorage;
} memory;

typedef struct memory_arena
{
    memory_index Size;
    u8 *Base;
    memory_index Used;
} memory_arena;

typedef struct temp_memory
{
    memory_arena *Arena;
    memory_index Used;
} temp_memory;

typedef struct state {
    memory_arena Arena;
} state;

#define PUSH_STRUCT(arena, type) (type *)push_size_(arena, sizeof(type))
#define PUSH_ARRAY(arena, count, type) (type *)push_size_(arena, (count)*sizeof(type))
#define PUSH_SIZE(arena, size) push_size_(arena, size)

void reserve_memory(memory *m);
void* push_size_(memory_arena *arena, memory_index size);
void initialize_arena(memory_arena *arena, memory_index size, u8 *Base);
void end_temporary_memory(temp_memory temp);
temp_memory begin_temporary_memory(memory_arena* arena);

typedef struct trans_state
{
    memory_arena ScratchArena;
} trans_state;

#endif
