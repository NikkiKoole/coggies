#ifndef STATES_H
#define STATES_H
#include "memory.h"
//#include "Math.h"
#include "my_math.h"

typedef struct {
    int frameX, frameY, frameW, frameH;
    int sssX, sssY, sssW, sssH;
    int ssW, ssH;
} SimpleFrame;


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

    int x_internal_off; // coming from cropping of sprites in texture
    int y_internal_off; // "

    int x_off; // used for offsetting sprite in world (think stairblocks at various heights)
    int y_off; // "
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
    u8 plays_forward;
    int first_frame;
    int last_frame;
} DynamicBlock; //64



typedef struct FoundPathNode {
    Vector3 node;
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
    Vector3 location;
    s16 dx;
    s16 dy;
    Vector3 velocity;
    Vector3 acceleration;
    float mass;
    float max_speed;
    float max_force;
} ActorSteerData;


typedef struct {
    Vector3 _location;
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
    StairsMeta, /*StairsDownMeta,*/
    StairsFollowUpMeta,
    Stairs1N, Stairs2N, Stairs3N, Stairs4N,
    Stairs1E, Stairs2E, Stairs3E, Stairs4E,
    Stairs1S, Stairs2S, Stairs3S, Stairs4S,
    Stairs1W, Stairs2W, Stairs3W, Stairs4W,

    EscalatorUpMeta,
    EscalatorUp1N, EscalatorUp2N, EscalatorUp3N, EscalatorUp4N,
    EscalatorUp1E, EscalatorUp2E, EscalatorUp3E, EscalatorUp4E,
    EscalatorUp1S, EscalatorUp2S, EscalatorUp3S, EscalatorUp4S,
    EscalatorUp1W, EscalatorUp2W, EscalatorUp3W, EscalatorUp4W,

    EscalatorDownMeta,
    EscalatorDown1N, EscalatorDown2N, EscalatorDown3N, EscalatorDown4N,
    EscalatorDown1E, EscalatorDown2E, EscalatorDown3E, EscalatorDown4E,
    EscalatorDown1S, EscalatorDown2S, EscalatorDown3S, EscalatorDown4S,
    EscalatorDown1W, EscalatorDown2W, EscalatorDown3W, EscalatorDown4W,

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
    b32 is_jumpnode;
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
