
#ifndef PATHFIND_H
#define PATHFIND_H
#include "types.h"
#include "memory.h"
#include "states.h"

typedef struct path_node {
    u8 X, Y, Z;
    struct path_node * Next;
    struct path_node * Prev;
} path_node;

typedef struct path_list {
    path_node * Sentinel;
} path_list;

typedef struct grid_node_list {
    grid_node *Sentinel;
} grid_node_list;

typedef struct {
    u32 size;
    u32 count;
    grid_node** data;
} grid_node_heap;

typedef struct jump_point {
    u32 X, Y, Z;
} jump_point;

void init_grid(Grid * g, MemoryArena * Arena, LevelData * m);
void preprocess_grid(Grid *g);
grid_node *GetNodeAt(Grid *Grid, int x, int y, int z);
path_list * FindPathPlus(grid_node * startNode, grid_node * endNode, Grid * Grid, MemoryArena * Arena);
path_list * SmoothenPath(path_list *compressed, MemoryArena * Arena, Grid * pg);
path_list * ExpandPath(path_list *compressed, MemoryArena * Arena);
#endif
