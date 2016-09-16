#ifndef MEMORY_H
#define MEMORY_H

#include <sys/mman.h> //mmap
#include "types.h"
#include <sys/stat.h>   //struct stat


#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

#define PERF_DICT_SIZE 1024
typedef struct  {
    const char* key;
    int times_counted;
    u64 total_time;
    u64 min;
    u64 max;
} PerfDictEntry;

typedef struct {
    PerfDictEntry data[PERF_DICT_SIZE];
} PerfDict;



#define BEGIN_PERFORMANCE_COUNTER(name) u64 name##_begin = SDL_GetPerformanceCounter()
#define END_PERFORMANCE_COUNTER(name)  u64 name##_end = SDL_GetPerformanceCounter();perf_dict_set(&debug->perf_dict, #name, name##_end - name##_begin);


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
    u32 debug_size;
    void *debug;
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



void perf_dict_set(PerfDict *d,  const char *key, u64 add);
PerfDictEntry perf_dict_get(PerfDict *d, char* key);
void perf_dict_reset(PerfDict *d);
void perf_dict_sort_clone(PerfDict *source, PerfDict *clone);



#define PUSH_STRUCT(arena, type) (type *) push_size_(arena, sizeof(type))
#define PUSH_ARRAY(arena, count, type) (type *) push_size_(arena, (count) * sizeof(type))
#define PUSH_SIZE(arena, size) push_size_(arena, size)

void reserve_memory(Memory *m);
void *push_size_(MemoryArena *arena, memory_index size);
void initialize_arena(MemoryArena *arena, memory_index size, u8 *Base);
void end_temporary_memory(TempMemory temp);
TempMemory begin_temporary_memory(MemoryArena *arena);



#endif
