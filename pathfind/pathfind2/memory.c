#include "memory.h"





void reserve_memory(memory *m)
{
    void *base_address = (void *) gigabytes(1);
    m->PermanentStorageSize = megabytes(16);
    m->ScratchStorageSize = gigabytes(1);

    u64 total_storage_size = m->PermanentStorageSize + m->ScratchStorageSize;
    m->PermanentStorage = mmap(base_address, total_storage_size,
                              PROT_READ | PROT_WRITE,
                              MAP_ANON | MAP_PRIVATE,
                              -1, 0);
    m->ScratchStorage = (u8*)(m->PermanentStorage) + m->PermanentStorageSize;
    m->IsInitialized = false;
}


temp_memory begin_temporary_memory(memory_arena* arena)
{
    temp_memory Result = {.Used= arena->Used, .Arena=arena};
    return Result;
}

void end_temporary_memory(temp_memory temp)
{
    memory_arena *Arena = temp.Arena;
    ASSERT(Arena->Used >= temp.Used)
    Arena->Used = temp.Used;
}

void* push_size_(memory_arena *arena, memory_index size){
    // TODO: this allignment should be an argument to this function with a default of 4.
    memory_index allignment = 4;

    memory_index result_pointer = (memory_index)arena->Base + arena->Used;
    memory_index allignment_offset = 0;

    memory_index allignment_mask = allignment - 1;
    if (result_pointer & allignment_mask){
        allignment_offset = allignment - (result_pointer & allignment_mask);

    }
    size += allignment_offset;

    ASSERT(arena->Used + size <= arena->Size);
    arena->Used += size;
    void *Result = (void *)(result_pointer + allignment_offset);
    return Result;
}

void initialize_arena(memory_arena *arena, memory_index size, u8 *base)
{
    arena->Size = size;
    arena->Base = base;
    arena->Used = 0;
}
