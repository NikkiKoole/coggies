#ifndef STATES_H
#define STATES_H
#include "memory.h"

typedef struct {
    MemoryArena arena;
} ScratchState;

typedef struct {
    MemoryArena arena;
    PerfDict perf_dict;
} DebugState;

typedef struct {
    int x_pos;
    int y_pos;
} BlockTextureAtlasPosition;

typedef struct {
    u16 x;
    u16 y;
    u16 z;
    BlockTextureAtlasPosition frame;
} Wall; //64

typedef struct {
    r32 x;
    r32 y;
    r32 z;
    u16 frame;
    s16 dx;
    s16 dy;
    r32 palette_index;
} Actor; //176

typedef struct {
    u16 x;
    u16 y;
    u16 sx;
    u16 sy;
    u16 w;
    u16 h;
} Glyph; //96

typedef struct {
    u16 x1, y1, z1;
    u16 x2, y2, z2;
    r32 r,g,b;
} ColoredLine; //198

typedef struct {
    u32 x;
    u32 y;
    u32 z_level;
} WorldDims;


typedef enum {
    Nothing,
    WallBlock,
    Floor, Grass, Wood, Concrete, Tiles, Carpet,
    LadderUpDown, LadderUp, LadderDown,
    StairsUpMeta, StairsDownMeta,
    StairsFollowUpMeta,
    StairsUp1N, StairsUp2N, StairsUp3N, StairsUp4N,
    StairsUp1E, StairsUp2E, StairsUp3E, StairsUp4E,
    StairsUp1S, StairsUp2S, StairsUp3S, StairsUp4S,
    StairsUp1W, StairsUp2W, StairsUp3W, StairsUp4W,
    Shaded,
    BlockTotal
} Block;

typedef struct {
    Block object;
    Block floor;
    Block meta_object;
} WorldBlock;

typedef struct {
    WorldBlock *blocks;
    int block_count;
    int x, y, z_level;
} LevelData;


typedef struct grid_node {
    u8 X, Y, Z;
    u8 walkable;
    Block type;
    float g;
    float f;
    float h;
    //float cost;
    u8 opened;
    u8 closed;
    struct grid_node *parent; //used for backtracking
    struct grid_node *Next; // used when in neighbour slist; // not needed in in jpsplus
    //#ifdef JPS_PLUS
    b32 isJumpNode;
    s8 distance[10]; // 4 cardinal, 4 diagonal, 2 up/down / only used when doing jps plus
    //#endif
} grid_node;

typedef struct grid {
    int width;
    int height;
    int depth;
    grid_node* nodes;
} Grid;

typedef struct {
    MemoryArena arena;
    LevelData level;
    Wall *walls;//[16384];
    u32 wall_count;

    Actor *actors;//[16384*4];
    u32 actor_count;

    Glyph *glyphs;//[16384];
    u32 glyph_count;

    ColoredLine *colored_lines;//[16384];
    u32 colored_line_count;

    ///
    s32 x_view_offset;
    s32 y_view_offset;


    /////
    WorldDims dims;
    WorldDims block_size;
    ////
    Grid *grid;
} PermanentState;

void actor_remove(PermanentState *state, u32 index);
void actor_add(PermanentState *state);

#endif
