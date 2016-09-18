#include <stdio.h>
#include "memory.h"
#include "data_structures.h"

#include "pathfind.h"
#include "readMap.c"



#ifdef __MACH__
#include <sys/time.h>
#define CLOCK_MONOTONIC 1
//clock_gettime is not implemented on OSX
int clock_gettime(int unused, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

int main() {

    memory _memory;
    memory *Memory = &_memory;
    reserve_memory(Memory);

    ASSERT(sizeof(state) <= Memory->PermanentStorageSize);
    state *State = (state *)Memory->PermanentStorage;
    ASSERT(sizeof(trans_state) <= Memory->ScratchStorageSize);
    trans_state *TransState = (trans_state *)Memory->ScratchStorage;
    initialize_arena(&State->Arena,
                     Memory->PermanentStorageSize - sizeof(state),
                     (u8 *)Memory->PermanentStorage + sizeof(state) );

    initialize_arena(&TransState->ScratchArena,
                         Memory->ScratchStorageSize - sizeof(trans_state),
                         (u8 *)Memory->ScratchStorage + sizeof(trans_state));

    Memory->IsInitialized = true;

    temp_memory scratch = begin_temporary_memory(&TransState->ScratchArena);
    map_file m;
    readMapFile("maps/three.txt", &m);

    grid * g = PUSH_STRUCT(&TransState->ScratchArena, grid);
    InitGrid(g, &TransState->ScratchArena,  &m);

    grid_node * Start = GetNodeAt(g, m.startX, m.startY, m.startZ);
    grid_node * End = GetNodeAt(g, m.endX, m.endY, m.endZ);

    PreProcessGrid(g);

    DisplayPreProcessed(g);


    path_list * Path = FindPathPlus(Start, End, g, &TransState->ScratchArena);
    //path_list * Path = FindPathJPS(Start, End, g, &TransState->ScratchArena, OnlyWhenNoObstacles);
    path_list * Smoothed = SmoothenPath(Path,  &TransState->ScratchArena, g);
    path_list * Expanded = ExpandPath(Smoothed, &TransState->ScratchArena);
    DrawGrid(g, (jump_point){-1,-1,-1}, Expanded);

}
