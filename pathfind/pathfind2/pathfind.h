#ifndef PATHFIND_H
#define PATHFIND_H

#include "memory.h"

#define JPS_PLUS

#define SQRT2_OVER_1 0.41421356237309515
#define SQRT2 1.4142135623730951
#define NODE_EQUALS(node1, node2) (node1->X == node2->X && node1->Y == node2->Y && node1->Z == node2->Z)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(x)(((x) < 0) ? -(x) : (x))

#define FLATTEN_3D_INDEX(x,y,z, width, height) (x + (y*width) + (z * width * height))

#define PUSH_NEIGHBOR(x, y, z, NeighborList) {                      \
        neighbor *N = NULL;                                                \
        if (NeighborList->Free->Next != NeighborList->Sentinel) {   \
            N = NeighborList->Free->Next;                           \
            NeighborList->Free->Next = N->Next;                     \
        } else {                                                    \
            N = PUSH_STRUCT(Arena, neighbor);                       \
        }                                                           \
        N->X = x;                                                   \
        N->Y = y;                                                   \
        N->Z = z;                                                   \
        DLIST_ADDFIRST(NeighborList, N);                            \
    }                                                               \

#define FIND_NEIGHBORS_NORTH_SOUTH(Grid, x, y, z, List) {   \
        if (GridWalkableAt(Grid, x, y-1, z)) {              \
            PUSH_NEIGHBOR(x, y-1, z, List);                 \
        }                                                   \
        if (GridWalkableAt(Grid, x, y+1, z)) {              \
            PUSH_NEIGHBOR(x, y+1, z, List);                 \
        }                                                   \
    }                                                       \

#define FIND_NEIGHBORS_EAST_WEST(Grid, x, y, z, List) { \
        if (GridWalkableAt(Grid, x-1, y, z)) {          \
            PUSH_NEIGHBOR(x-1, y, z, List);             \
        }                                               \
        if (GridWalkableAt(Grid, x+1, y, z)) {          \
            PUSH_NEIGHBOR(x+1, y, z, List);             \
        }                                               \
    }                                                   \

#define FIND_NEIGHBORS_UP_DOWN(Grid, x, y, z, List) {   \
        if (GridCanGoUpFrom(Grid, x,y,z)) {             \
            PUSH_NEIGHBOR(x, y, z+1, List);             \
        }                                               \
        if (GridCanGoDownFrom(Grid, x,y,z)) {           \
            PUSH_NEIGHBOR(x, y, z-1, List);             \
        }                                               \
    }                                                   \




#define PUSH_COORD(x, y, CoordList) {                      \
        coord2d *N = NULL;                                                \
        if (CoordList->Free->Next != CoordList->Sentinel) {   \
            N = CoordList->Free->Next;                           \
            CoordList->Free->Next = N->Next;                     \
        } else {                                                    \
            N = PUSH_STRUCT(Arena, coord2d);                       \
        }                                                           \
        N->X = x;                                                   \
        N->Y = y;                                                   \
        DLIST_ADDLAST(CoordList, N);                            \
    }                                                               \




typedef struct {
    int width;
    int height;
    int depth;
    u8 *data;
    int startX, startY, startZ;
    int endX, endY, endZ;
} map_file;

typedef struct path_node {
    u8 X, Y, Z;
    struct path_node * Next;
    struct path_node * Prev;
} path_node;


typedef struct path_list {
    path_node * Sentinel;
} path_list;

typedef struct grid_node {
    u8 X, Y, Z;
    u8 walkable; // 0 is impassible, 1 = walkable, 2 = walkable&going UP too, 3 = walkable& going DOWN too, 4 = walkable* UP/DOWN too.
    float g;
    float f;
    float h;
    u8 opened;
    u8 closed;
    struct grid_node *parent; //used for backtracking

    struct grid_node *Next; // used when in neighbour slist; // not needed in in jpsplus

#ifdef JPS_PLUS
    b32 isJumpNode;
    s8 distance[10]; // 4 cardinal, 4 diagonal, 2 up/down / only used when doing jps plus
#endif
} grid_node;

typedef struct grid_node_list {
    grid_node *Sentinel;
} grid_node_list;

typedef struct {
    u32 size;
    u32 count;
    grid_node** data;
} grid_node_heap;

typedef struct grid {
    int width;
    int height;
    int depth;
    grid_node* nodes;
} grid;

typedef enum diagonal_movement {
    Never,
    OnlyWhenNoObstacles,
    IfAtMostOneObstacle,
    Always
} diagonal_movement;

typedef struct neighbor {
    int X, Y, Z;
    struct neighbor * Next;
    struct neighbor * Prev;
} neighbor;

typedef struct neighbor_list {
    neighbor * Sentinel;
    neighbor * Free;
} neighbor_list;

typedef struct jump_point {
    int X, Y, Z;
} jump_point;


typedef struct coord2d {
    int X;
    int Y;
    struct coord2d * Next;
    struct coord2d * Prev;
} coord2d;

typedef struct coord_list {
    coord2d * Sentinel;
    coord2d * Free;
} coord_list;



typedef jump_point (*jump_func) (grid *, grid_node *,int, int, int, int, int, int);
typedef neighbor_list * (*find_neighbors_func) (grid *, neighbor_list *, grid_node_list *,grid_node*, memory_arena*);

typedef struct JPS {
    jump_func jump;
    find_neighbors_func find_neighbors;
} JPS;

typedef enum jump_direction {
    north,
    east,
    south,
    west,
    ne,
    se,
    sw,
    nw,
    up,
    down,
} jump_direction;




path_list* FindPathAStar(grid_node *Start, grid_node *  End, grid * Grid, memory_arena * Arena, diagonal_movement Diagonal);

path_list* FindPathJPS(grid_node *Start, grid_node *  End, grid * Grid, memory_arena * Arena, diagonal_movement Diagonal);


void InitGrid(grid * g, memory_arena * Arena, map_file *m);
grid_node* GetNodeAt(grid *Grid, int x, int y, int z);
path_list * FindPathPlus(grid_node * startNode, grid_node * endNode, grid * Grid, memory_arena * Arena);
void PreProcessGrid(grid *g);
void DisplayPreProcessed(grid *g);

path_list * ExpandPath(path_list *compressed, memory_arena * Arena);
path_list * SmoothenPath(path_list *compressed, memory_arena * Arena, grid * pg);
void DrawGrid(grid * g, jump_point p, path_list *path );
#endif
