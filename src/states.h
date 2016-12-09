#ifndef STATES_H
#define STATES_H
#include "memory.h"
#include "GLKMath.h"

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
    int width;
    int height;
    int x_off;
    int y_off;
} BlockTextureAtlasPosition;

typedef struct {
    u16 x;
    u16 y;
    u16 z;
    BlockTextureAtlasPosition frame;
    u8 is_floor;
} StaticBlock; //64

typedef struct {
    u16 x;
    u16 y;
    u16 z;
    BlockTextureAtlasPosition frame;
    u8 is_floor;
    u8 total_frames;
    u8 current_frame;
    u16 start_frame_x;
    r32 duration_per_frame;
    r32 frame_duration_left;
} DynamicBlock; //64



typedef struct FoundPathNode {
    GLKVector3 node;
    float unused;
} FoundPathNode;

typedef struct {
    float data[4];
} BarNode;

typedef struct Node16
{
    union
    {
        FoundPathNode path;
        BarNode bar;
        u8 reserved[16];
    };
    struct Node16 *Next, *Prev;
} Node16;

typedef struct {
    MemoryArena arena;
    Node16 *Free;
    //Node16 *Sentinel;
} Node16Arena;


typedef struct {
    Node16 * Sentinel;
    int counter;
} ActorPath;

typedef struct {
    u16 frame;
    r32 palette_index;
    r32 frame_duration_left;
} ActorAnimData;

typedef struct {
    GLKVector3 location;
    s16 dx;
    s16 dy;
    GLKVector3 velocity;
    GLKVector3 acceleration;
    float mass;
    float max_speed;
    float max_force;
} ActorSteerData;


typedef struct {
    GLKVector3 _location;
    //ActorPath path;
    //ActorAnimData anim;
    //ActorSteerData steer;
    u32 _frame;
    r32 _palette_index;
    u32 index;
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
    WindowBlock,
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


    StaticBlock *static_blocks;//[16384];
    u32 static_block_count;

    DynamicBlock *dynamic_blocks;//[16384];
    u32 dynamic_block_count;

    StaticBlock *transparent_blocks;//[16384];
    u32 transparent_block_count;

    Actor *actors;//[16384*4];
    u32 actor_count;

    ActorPath *paths; //
    // doesnt need a count because it will be the same as actor
    ActorSteerData *steer_data;
    ActorAnimData *anim_data;



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
    //



} PermanentState;

void actor_remove(PermanentState *state, u32 index);
void actor_add(PermanentState *state);

#endif
