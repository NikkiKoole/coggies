
#include <stdio.h>
#include "memory.h"
#include "renderer.h"
#include "random.h"
#include "pathfind.h"
#include "states.h"
#include "level.h"
#include "data_structures.h"

#include "blocks.h"
#include "body.h"
#include "my_math.h"
#include <math.h>

#define SORT_NAME Actor
#define SORT_TYPE Actor
//#define SORT_CMP(b, a) ((((a).y * 16384) - (a).z) - (((b).y * 16384) - (b).z))
#define SORT_CMP(b, a) ((((a)._location.y * 16384) - (a)._location.z) - (((b)._location.y * 16384) - (b)._location.z))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Warray-bounds"

#include "sort.h"
#pragma GCC diagnostic pop

typedef enum {
    Front, Left, Right
} FacingSide;

FacingSide facing_side = Front;

global_value SimpleFrame generated_frames[BL_TOTAL];
global_value FrameWithPivotAnchor generated_body_frames[BP_TOTAL];

internal Vector3 seek_return(ActorSteerData *actor, Vector3 target) {
    Vector3 desired = Vector3Subtract(target, actor->location);
    if (desired.x != 0 || desired.y != 0 || desired.z != 0) {
        desired = Vector3Normalize(desired);
    }
    desired = Vector3MultiplyScalar(desired, actor->max_speed);
    Vector3 steer = Vector3Subtract(desired, actor->velocity);
    steer = Vector3Limit(steer, actor->max_force);
    return steer;
}

internal void actor_apply_force(ActorSteerData *a, Vector3 force) {
    force = Vector3DivideScalar(force, a->mass);
    a->acceleration = Vector3Add(a->acceleration, force);
}

internal int path_length_at_index(PermanentState *permanent, int i) {
    Node16 * fp = permanent->paths[i].Sentinel;
    int fp_count = 0;
    while(fp->Next != permanent->paths[i].Sentinel) {
        fp_count++;
        fp = fp->Next;
    }
    return fp_count;
}

internal int get_node16_freelist_length(Node16Arena * arena) {
    int count = 0;
    Node16 * node = arena->Free;
    while (node->Next != arena->Free) {
        node = node->Next;
        count++;
    }
    return count;
}


void game_update_and_render(Memory* memory,  RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e);

internal int sort_static_blocks_back_front (const void * a, const void * b)
{
    const StaticBlock *a2 = (const StaticBlock *) a;
    const StaticBlock *b2 = (const StaticBlock *) b;
    return (  (a2->y*16384 + a2->z ) - ( b2->y*16384 + b2->z));
}
internal int sort_static_blocks_front_back (const void * a, const void * b)
{
    const StaticBlock *a2 = (const StaticBlock *) a;
    const StaticBlock *b2 = (const StaticBlock *) b;
    return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z)); //// this sorts walls front to back (much faster rendering)
}


internal void set_actor_batch_sizes(PermanentState *permanent, RenderState *renderer) {
#define ACTOR_PARTS 2

    u32 used_batches = ceil((permanent->actor_count * ACTOR_PARTS) / ((MAX_IN_BUFFER) * 1.0f));
    renderer->used_actor_batches = used_batches;

    if (used_batches == 1) {
        renderer->actors[0].count = (permanent->actor_count * ACTOR_PARTS);
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches - 1; i++) {
            renderer->actors[i].count = (MAX_IN_BUFFER);
        }
        renderer->actors[used_batches - 1].count = (permanent->actor_count * ACTOR_PARTS)  % (MAX_IN_BUFFER);
    } else {
        renderer->used_actor_batches = 0;
    }
    //    printf("used batches: %d\n", used_batches);
#undef ACTOR_PARTS
}

internal void set_dynamic_block_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->dynamic_block_count / (MAX_IN_BUFFER * 1.0f));
    renderer->used_dynamic_block_batches = used_batches;

    if (used_batches == 1) {
        renderer->dynamic_blocks[0].count = permanent->dynamic_block_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->dynamic_blocks[i].count = MAX_IN_BUFFER;
        }
        renderer->dynamic_blocks[used_batches-1].count = permanent->dynamic_block_count % MAX_IN_BUFFER;
    } else {
        renderer->used_dynamic_block_batches = 0;
    }
}

internal void set_transparent_block_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->transparent_block_count / (MAX_IN_BUFFER * 1.0f));
    renderer->used_transparent_block_batches = used_batches;

    if (used_batches == 1) {
        renderer->transparent_blocks[0].count = permanent->transparent_block_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->transparent_blocks[i].count = MAX_IN_BUFFER;
        }
        renderer->transparent_blocks[used_batches-1].count = permanent->transparent_block_count % MAX_IN_BUFFER;
    } else {
        renderer->used_transparent_block_batches = 0;
    }
}


internal void set_static_block_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->static_block_count / (MAX_IN_BUFFER * 1.0f));
    renderer->used_static_block_batches = used_batches;

    if (used_batches == 1) {
        renderer->static_blocks[0].count = permanent->static_block_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->static_blocks[i].count = MAX_IN_BUFFER;
        }
        renderer->static_blocks[used_batches-1].count = permanent->static_block_count % MAX_IN_BUFFER;
    } else {
        renderer->used_static_block_batches = 0;
    }
}
internal void set_colored_line_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->colored_line_count / (MAX_IN_BUFFER * 1.0f));
    renderer->used_colored_lines_batches = used_batches;

    if (used_batches == 1) {
        renderer->colored_lines[0].count = permanent->colored_line_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches - 1; i++) {
            renderer->colored_lines[i].count = MAX_IN_BUFFER;
        }
        renderer->colored_lines[used_batches - 1].count = permanent->colored_line_count % MAX_IN_BUFFER;
    } else {
        renderer->used_colored_lines_batches = 0;
    }
}


internal grid_node* get_neighboring_walkable_node(Grid *grid, int x, int y, int z){
    grid_node *result = NULL;
    for (int h = -1; h<2; h++) {
        for (int v = -1; v <2; v++) {
            result = get_node_at(grid, x+h, y+v, z);
            if (result->walkable) {
                return result;
            }
        }
    }
    return result;
}

internal grid_node* get_random_walkable_node(Grid *grid) {
    grid_node *result;
    do {
        result = get_node_at(grid, rand_int(grid->width), rand_int(grid->height), rand_int(grid->depth));
    } while (!result->walkable);
    return result;
}

internal BlockTextureAtlasPosition convertSimpleFrameToBlockTexturePos(SimpleFrame frame, int xOff, int yOff) {
    BlockTextureAtlasPosition result;
    result.x_pos          = frame.frameX;
    result.y_pos          = frame.frameY;
    result.width          = frame.frameW;
    result.height         = frame.frameH;
    result.x_off          = xOff;
    result.y_off          = yOff;
    result.y_internal_off = frame.ssH - (frame.sssY + frame.frameH);
    result.x_internal_off = frame.ssW - (frame.sssX + frame.frameW);

    result.sssX = frame.sssX;
    result.sssY = frame.sssY;

    result.ssW = frame.ssW;
    result.ssH = frame.ssH;

    result.pivotX         = frame.pivotX;
    result.pivotY         = frame.pivotY;

    if (result.y_internal_off > 0) printf("result: %d\n", result.y_internal_off);
    return result;
}




internal void add_static_block(int x, int y, int z,PermanentState *permanent, int *used_block_count, BlockTextureAtlasPosition texture_atlas_data) {
    permanent->static_blocks[*used_block_count].x     = x * permanent->block_size.x;
    permanent->static_blocks[*used_block_count].y     = (y * permanent->block_size.y) ;
    permanent->static_blocks[*used_block_count].z     = z * permanent->block_size.z_level;
    permanent->static_blocks[*used_block_count].frame = texture_atlas_data;
}
internal void add_transparent_block(int x, int y, int z,PermanentState *permanent, int *used_block_count, BlockTextureAtlasPosition texture_atlas_data) {
    permanent->transparent_blocks[*used_block_count].x     = x * permanent->block_size.x;
    permanent->transparent_blocks[*used_block_count].y     = (y * permanent->block_size.y) ;
    permanent->transparent_blocks[*used_block_count].z     = z * permanent->block_size.z_level;
    permanent->transparent_blocks[*used_block_count].frame = texture_atlas_data;
}
internal void add_dynamic_block(int x, int y, int z,PermanentState *permanent, int *used_block_count, BlockTextureAtlasPosition texture_atlas_data, int first, int last, float duration, int direction) {
    permanent->dynamic_blocks[*used_block_count].x                  = x * permanent->block_size.x;
    permanent->dynamic_blocks[*used_block_count].y                  = (y * permanent->block_size.y) ;
    permanent->dynamic_blocks[*used_block_count].z                  = z * permanent->block_size.z_level;
    permanent->dynamic_blocks[*used_block_count].frame              = texture_atlas_data;
    permanent->dynamic_blocks[*used_block_count].first_frame        = first;
    permanent->dynamic_blocks[*used_block_count].last_frame         = last;
    permanent->dynamic_blocks[*used_block_count].current_frame      = first;
    permanent->dynamic_blocks[*used_block_count].total_frames       = (last-first)+1;
    permanent->dynamic_blocks[*used_block_count].duration_per_frame = duration;
    permanent->dynamic_blocks[*used_block_count].plays_forward      = direction;
}



internal Block getRotatedBlock(Block input, FacingSide side) {
    // you get a front facing block and a side you want to see.
    if (input != Nothing) {
        if (side == Left) {
            switch(input) {
            case Stairs1N: case Stairs2N: case Stairs3N: case Stairs4N:
            case EscalatorUp1N: case EscalatorUp2N: case EscalatorUp3N: case EscalatorUp4N:
            case EscalatorDown1N: case EscalatorDown2N: case EscalatorDown3N: case EscalatorDown4N:
                return input + 12; // north -> west
            case Stairs1E: case Stairs2E: case Stairs3E: case Stairs4E:
            case EscalatorUp1E: case EscalatorUp2E: case EscalatorUp3E: case EscalatorUp4E:
            case EscalatorDown1E: case EscalatorDown2E: case EscalatorDown3E: case EscalatorDown4E:
                return input - 4; // east -> north
            case Stairs1S: case Stairs2S: case Stairs3S: case Stairs4S:
            case EscalatorUp1S: case EscalatorUp2S: case EscalatorUp3S: case EscalatorUp4S:
            case EscalatorDown1S: case EscalatorDown2S: case EscalatorDown3S: case EscalatorDown4S:
                return input - 4; // south -> east
            case Stairs1W: case Stairs2W: case Stairs3W: case Stairs4W:
            case EscalatorUp1W: case EscalatorUp2W: case EscalatorUp3W: case EscalatorUp4W:
            case EscalatorDown1W: case EscalatorDown2W: case EscalatorDown3W: case EscalatorDown4W:
                return input - 4; // west -> south

            case Nothing: case Shaded: case BlockTotal:
            case WindowBlock: case WallBlock: case LadderUpDown: case LadderUp: case LadderDown:
            case Floor: case Grass: case Wood: case Concrete: case Tiles: case Carpet:
            case StairsMeta: case StairsFollowUpMeta: case EscalatorUpMeta: case EscalatorDownMeta:
                return input;
            }
        } else if (side == Right) {

            switch(input) {
            case Stairs1N: case Stairs2N: case Stairs3N: case Stairs4N:
            case EscalatorUp1N: case EscalatorUp2N: case EscalatorUp3N: case EscalatorUp4N:
            case EscalatorDown1N: case EscalatorDown2N: case EscalatorDown3N: case EscalatorDown4N:
                return input + 4; // north -> east
            case Stairs1E: case Stairs2E: case Stairs3E: case Stairs4E:
            case EscalatorUp1E: case EscalatorUp2E: case EscalatorUp3E: case EscalatorUp4E:
            case EscalatorDown1E: case EscalatorDown2E: case EscalatorDown3E: case EscalatorDown4E:
                return input + 4; // east -> south
            case Stairs1S: case Stairs2S: case Stairs3S: case Stairs4S:
            case EscalatorUp1S: case EscalatorUp2S: case EscalatorUp3S: case EscalatorUp4S:
            case EscalatorDown1S: case EscalatorDown2S: case EscalatorDown3S: case EscalatorDown4S:
                return input + 4; // south -> west
            case Stairs1W: case Stairs2W: case Stairs3W: case Stairs4W:
            case EscalatorUp1W: case EscalatorUp2W: case EscalatorUp3W: case EscalatorUp4W:
            case EscalatorDown1W: case EscalatorDown2W: case EscalatorDown3W: case EscalatorDown4W:
                return input - 12; // west -> north

            case Nothing: case Shaded: case BlockTotal:
            case WindowBlock: case WallBlock: case LadderUpDown: case LadderUp: case LadderDown:
            case Floor: case Grass: case Wood: case Concrete: case Tiles: case Carpet:
            case StairsMeta: case StairsFollowUpMeta: case EscalatorUpMeta: case EscalatorDownMeta:
                return input;
            }
        } else {
            return input;
        }
    }
    return Nothing;
}

internal u32 getRotatedXInt(u32 x, u32 y, FacingSide side, WorldDims dims ) {
    if (side == Front) {
        return x;
    } else if (side == Left) {
        return y;
    } else if (side == Right) {
        return dims.y - y;
    }
    return x;
}
internal u32 getRotatedYInt(u32 x, u32 y, FacingSide side, WorldDims dims ) {
    if (side == Front) {
        return y;
    } else if (side == Left) {
        return dims.x - x;
    } else if (side == Right) {
        return x;
    }
    return y;
}
internal float getRotatedXFloat(float x, float y, FacingSide side, WorldDims dims ) {
    if (side == Front) {
        return x;
    } else if (side == Left) {
        return y;
    } else if (side == Right) {
        // WHAT THE FIUCK is this +1
        // and why isnt is neede3d in the other rotations ;)?
        return (dims.y+1)*24 - y;
    }
    return x;
}
internal float getRotatedYFloat(float x, float y, FacingSide side, WorldDims dims ) {
    if (side == Front) {
        return y;
    } else if (side == Left) {
        return dims.x*24 - x;
    } else if (side == Right) {
        return x;
    }
    return y;
}
internal Vector3 getRotatedVec3(Vector3 pos, FacingSide side, WorldDims dims, WorldDims sizes) {
    if (side == Front) {
    } else if (side == Left){
        return Vector3Make(pos.y, (dims.x * sizes.x) - pos.x, pos.z);
    } else if (side == Right){
        return Vector3Make(( (dims.y * sizes.y) - pos.y) + sizes.y, pos.x, pos.z);
    }
    return Vector3Make(pos.x, pos.y, pos.z);
}


internal void initialize_memory( PermanentState *permanent, Node16Arena *node16, RenderState *renderer, Memory* memory) {
    permanent->dynamic_blocks     = (StaticBlock*) PUSH_ARRAY(&permanent->arena, (16384), StaticBlock);
    permanent->static_blocks      = (StaticBlock*) PUSH_ARRAY(&permanent->arena, (16384), StaticBlock);
    permanent->transparent_blocks = (StaticBlock*) PUSH_ARRAY(&permanent->arena, (16384), StaticBlock);
    permanent->actors             = (Actor*) PUSH_ARRAY(&permanent->arena, (16384*4), Actor);
    permanent->paths              = (ActorPath*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorPath);
    permanent->steer_data         = (ActorSteerData*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorSteerData);
    for (int i = 0; i < 16384*4; i++) {
        permanent->steer_data[i].mass      = 1.0f;
        permanent->steer_data[i].max_force = 10;
        permanent->steer_data[i].max_speed = 50 + rand_float() * 80;
    }
    permanent->anim_data = (ActorAnimData*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorAnimData);

    for (int i = 0; i < 16384*4; i++) {
        permanent->anim_data[i].frame = 4;
    }
    for (int i = 0; i < 16384*4; i++) {
        Node16 *Sentinel = (Node16 *) PUSH_STRUCT(&node16->arena, Node16);
        permanent->paths[i].Sentinel            = Sentinel;
        permanent->paths[i].Sentinel->Next      = Sentinel;
        permanent->paths[i].Sentinel->Prev      = Sentinel;
        permanent->actors[i].index              = i;
        permanent->paths[i].Sentinel->path.node = Vector3Make(-999,-999,-999);
    }

    node16->Free = PUSH_STRUCT(&node16->arena, Node16);
    node16->Free->Next = node16->Free;
    node16->Free->Prev = node16->Free;

    permanent->glyphs = (Glyph*) PUSH_ARRAY(&permanent->arena, (16384), Glyph);
    permanent->colored_lines = (ColoredLine*) PUSH_ARRAY(&permanent->arena, (16384), ColoredLine);

    BlockTextureAtlasPosition texture_atlas_data[BlockTotal];
    fill_generated_values(generated_frames);
    fill_generated_complex_values(generated_body_frames);

    texture_atlas_data[Floor]        = convertSimpleFrameToBlockTexturePos(generated_frames[BL_floor_000], 0, 0);
    texture_atlas_data[WallBlock]    = convertSimpleFrameToBlockTexturePos(generated_frames[BL_wall_000], 0, 0);
    texture_atlas_data[WindowBlock]  = convertSimpleFrameToBlockTexturePos(generated_frames[BL_window_000], 0, 0);
    texture_atlas_data[LadderUp]     = convertSimpleFrameToBlockTexturePos(generated_frames[BL_ladder_up_000], 0, 0);
    texture_atlas_data[LadderUpDown] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_ladder_up_down_000], 0, 0);
    texture_atlas_data[LadderDown]   = convertSimpleFrameToBlockTexturePos(generated_frames[BL_ladder_down_000], 0, 0);

    texture_atlas_data[EscalatorDown1S] = texture_atlas_data[EscalatorUp1S] = texture_atlas_data[Stairs1S] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_south_up_000], 0, 0);
    texture_atlas_data[EscalatorDown2S] = texture_atlas_data[EscalatorUp2S] = texture_atlas_data[Stairs2S] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_south_up_000], 0, 24);
    texture_atlas_data[EscalatorDown3S] = texture_atlas_data[EscalatorUp3S] = texture_atlas_data[Stairs3S] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_south_up_000], 0, 48);
    texture_atlas_data[EscalatorDown4S] = texture_atlas_data[EscalatorUp4S] = texture_atlas_data[Stairs4S] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_south_up_000], 0, 72);

    texture_atlas_data[EscalatorDown1N] = texture_atlas_data[EscalatorUp1N] = texture_atlas_data[Stairs1N] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_north_up_000], 0, 0);
    texture_atlas_data[EscalatorDown2N] = texture_atlas_data[EscalatorUp2N] = texture_atlas_data[Stairs2N] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_north_up_000], 0, 24);
    texture_atlas_data[EscalatorDown3N] = texture_atlas_data[EscalatorUp3N] = texture_atlas_data[Stairs3N] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_north_up_000], 0, 48);
    texture_atlas_data[EscalatorDown4N] = texture_atlas_data[EscalatorUp4N] = texture_atlas_data[Stairs4N] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_north_up_000], 0, 72);

    texture_atlas_data[EscalatorDown1W] = texture_atlas_data[EscalatorUp1W] = texture_atlas_data[Stairs1W] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_west_up_000], 0, 0);
    texture_atlas_data[EscalatorDown2W] = texture_atlas_data[EscalatorUp2W] = texture_atlas_data[Stairs2W] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_west_up_000], 0, 24);
    texture_atlas_data[EscalatorDown3W] = texture_atlas_data[EscalatorUp3W] = texture_atlas_data[Stairs3W] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_west_up_000], 0, 48);
    texture_atlas_data[EscalatorDown4W] = texture_atlas_data[EscalatorUp4W] = texture_atlas_data[Stairs4W] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_west_up_000], 0, 72);

    texture_atlas_data[EscalatorDown1E] = texture_atlas_data[EscalatorUp1E] = texture_atlas_data[Stairs1E] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_east_up_000], 0, 0  );
    texture_atlas_data[EscalatorDown2E] = texture_atlas_data[EscalatorUp2E] = texture_atlas_data[Stairs2E] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_east_up_000], 0, 24 );
    texture_atlas_data[EscalatorDown3E] = texture_atlas_data[EscalatorUp3E] = texture_atlas_data[Stairs3E] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_east_up_000], 0, 48 );
    texture_atlas_data[EscalatorDown4E] = texture_atlas_data[EscalatorUp4E] = texture_atlas_data[Stairs4E] = convertSimpleFrameToBlockTexturePos(generated_frames[BL_escalator_east_up_000], 0, 72 );

    int used_static_block_count = 0;
    int used_dynamic_block_count = 0;
    int used_transparent_block_count = 0;

    for (u32 z = 0; z < permanent->dims.z_level ; z++){
        for (u32 y = 0; y< permanent->dims.y; y++){
            for (u32 x = 0; x< permanent->dims.x; x++){
                WorldBlock *b = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z,permanent->dims.x, permanent->dims.y)];


                u32 x_ = getRotatedXInt(x, y, facing_side, permanent->dims);
                u32 y_ = getRotatedYInt(x, y, facing_side, permanent->dims);
                Block block = getRotatedBlock(b->object, facing_side);


                permanent->static_blocks[used_static_block_count].is_floor = 0;
                switch (block){
                case WindowBlock:
                    add_transparent_block(x_, y_, z, permanent, &used_transparent_block_count, texture_atlas_data[block]);
                    used_transparent_block_count++;
                    break;

                case WallBlock:
                case LadderUpDown:
                case LadderUp:
                case LadderDown:
                    add_static_block(x_, y_, z, permanent, &used_static_block_count, texture_atlas_data[block]);
                    used_static_block_count++;
                    break;

                case Floor:
                case Stairs1N: case Stairs2N: case Stairs3N: case Stairs4N:
                case Stairs1E: case Stairs2E: case Stairs3E: case Stairs4E:
                case Stairs1S: case Stairs2S: case Stairs3S: case Stairs4S:
                case Stairs1W: case Stairs2W: case Stairs3W: case Stairs4W:
                    add_static_block(x_, y_, z, permanent, &used_static_block_count, texture_atlas_data[block]);
                    permanent->static_blocks[used_static_block_count].is_floor = 1;
                    used_static_block_count++;
                    break;

                case EscalatorUp1N: case EscalatorUp2N: case EscalatorUp3N: case EscalatorUp4N:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block], BL_escalator_north_up_000, BL_escalator_north_up_011, 0.5f/12, 1);
                    used_dynamic_block_count++;
                    break;
                case EscalatorDown1N: case EscalatorDown2N: case EscalatorDown3N: case EscalatorDown4N:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block], BL_escalator_north_up_000, BL_escalator_north_up_011, 0.5f/12, 0);
                    used_dynamic_block_count++;
                    break;
                case EscalatorUp1E: case EscalatorUp2E: case EscalatorUp3E: case EscalatorUp4E:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block], BL_escalator_east_up_000, BL_escalator_east_up_007, 0.5f/8, 1);
                    used_dynamic_block_count++;
                    break;
                case EscalatorDown1E: case EscalatorDown2E: case EscalatorDown3E: case EscalatorDown4E:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block],  BL_escalator_east_up_000,  BL_escalator_east_up_007, 0.5f/8, 0);
                    used_dynamic_block_count++;
                    break;
                case EscalatorUp1W: case EscalatorUp2W: case EscalatorUp3W: case EscalatorUp4W:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block], BL_escalator_west_up_000, BL_escalator_west_up_007, 0.5f/8, 1);
                    used_dynamic_block_count++;
                    break;
                case EscalatorDown1W: case EscalatorDown2W: case EscalatorDown3W: case EscalatorDown4W:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block], BL_escalator_west_up_000, BL_escalator_west_up_007, 0.5f/8, 0);
                    used_dynamic_block_count++;
                    break;
                case EscalatorUp1S: case EscalatorUp2S: case EscalatorUp3S: case EscalatorUp4S:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block], BL_escalator_south_up_000, BL_escalator_south_up_007, 0.5f/8, 1);
                    used_dynamic_block_count++;
                    break;
                case EscalatorDown1S: case EscalatorDown2S: case EscalatorDown3S: case EscalatorDown4S:
                    permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                    add_dynamic_block(x_, y_, z, permanent, &used_dynamic_block_count, texture_atlas_data[block], BL_escalator_south_up_000, BL_escalator_south_up_007, 0.5f/8, 0);
                    used_dynamic_block_count++;
                    break;
                case Grass:
                case Wood:
                case Concrete:
                case Nothing:
                case Tiles:
                case Carpet:
                case EscalatorUpMeta:
                case EscalatorDownMeta:
                case StairsMeta:
                case StairsFollowUpMeta:
                case Shaded:
                case BlockTotal:

                default:
                    break;
                }
            }
        }
    }

    permanent->static_block_count      = used_static_block_count;
    permanent->dynamic_block_count     = used_dynamic_block_count;
    permanent->transparent_block_count = used_transparent_block_count;

    set_static_block_batch_sizes(permanent, renderer);
    set_dynamic_block_batch_sizes(permanent, renderer);
    set_transparent_block_batch_sizes(permanent, renderer);

    qsort(permanent->static_blocks, used_static_block_count, sizeof(StaticBlock), sort_static_blocks_front_back);
    qsort(permanent->transparent_blocks, used_transparent_block_count, sizeof(StaticBlock), sort_static_blocks_back_front);

    renderer->needs_prepare = 1;

    set_actor_batch_sizes(permanent, renderer);

    permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);
    init_grid(permanent->grid, &permanent->arena, &permanent->level);
    preprocess_grid(permanent->grid);
    set_colored_line_batch_sizes(permanent, renderer);
    memory->is_initialized = true;

}

extern void game_update_and_render(Memory* memory, RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e) {
    UNUSED(keys);
    UNUSED(e);
    ASSERT(sizeof(PermanentState) <= memory->permanent_size);
    PermanentState *permanent = (PermanentState *)memory->permanent;
    ASSERT(sizeof(ScratchState) <= memory->scratch_size);
    ScratchState *scratch = (ScratchState *)memory->scratch;
    ASSERT(sizeof(DebugState) <= memory->debug_size);
    DebugState *debug = (DebugState *)memory->debug;
    ASSERT(sizeof(Node16Arena) <= memory->node16_size);
    Node16Arena *node16 = (Node16Arena *)memory->node16;

    ASSERT(sizeof(Node16)  == sizeof(Node16*)*2 + 16);
    ASSERT(sizeof(FoundPathNode) == sizeof(BarNode));


    if (memory->is_initialized == false) {
        initialize_memory(permanent, node16, renderer, memory);
    }


    // dynamic blocks
    for (u32 i = 0; i < permanent->dynamic_block_count; i++) {
        if ( permanent->dynamic_blocks[i].total_frames > 1) {
            permanent->dynamic_blocks[i].frame_duration_left += last_frame_time_seconds;
            if (permanent->dynamic_blocks[i].frame_duration_left >= permanent->dynamic_blocks[i].duration_per_frame) {

                int frame_index = permanent->dynamic_blocks[i].current_frame;

                if (permanent->dynamic_blocks[i].plays_forward == 1) {
                    frame_index +=1;
                    if (frame_index > permanent->dynamic_blocks[i].last_frame) frame_index = permanent->dynamic_blocks[i].first_frame;
                } else {
                    frame_index -= 1;
                    if (frame_index < permanent->dynamic_blocks[i].first_frame) frame_index = permanent->dynamic_blocks[i].last_frame;
                }

                permanent->dynamic_blocks[i].current_frame        = frame_index;
                permanent->dynamic_blocks[i].frame_duration_left  = 0;

                SimpleFrame f = generated_frames[frame_index];
                permanent->dynamic_blocks[i].frame.x_pos          = f.frameX;
                permanent->dynamic_blocks[i].frame.y_pos          = f.frameY;
                permanent->dynamic_blocks[i].frame.width          = f.frameW;
                permanent->dynamic_blocks[i].frame.height         = f.frameH;
                permanent->dynamic_blocks[i].frame.y_internal_off = f.ssH - (f.sssY + f.frameH);
                permanent->dynamic_blocks[i].frame.x_internal_off = f.ssW - (f.sssX + f.frameW);
            }
        }
    }



#if 1
    {
        permanent->colored_line_count = 0;

        for (u32 i = 0; i < permanent->actor_count; i++) {

            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                float distance = Vector3Distance(permanent->steer_data[i].location, permanent->paths[i].Sentinel->Next->path.node);
                if (distance < 5.f){
                    Node16 *First = permanent->paths[i].Sentinel->Next;
                    permanent->paths[i].Sentinel->Next = First->Next;
                    First->Next = node16->Free->Next;
                    node16->Free->Next = First;
                    First->Prev = node16->Free;
                }
            }


            BEGIN_PERFORMANCE_COUNTER(actors_steering);
            {
                Vector3 seek_force  = seek_return(&permanent->steer_data[i], permanent->paths[i].Sentinel->Next->path.node);
                seek_force = Vector3MultiplyScalar(seek_force, 1);
                actor_apply_force(&permanent->steer_data[i], seek_force);


                permanent->steer_data[i].velocity = Vector3Add(permanent->steer_data[i].velocity, permanent->steer_data[i].acceleration);
                permanent->steer_data[i].velocity = Vector3Limit(permanent->steer_data[i].velocity, permanent->steer_data[i].max_speed);
                permanent->steer_data[i].location = Vector3Add(permanent->steer_data[i].location,   Vector3MultiplyScalar(permanent->steer_data[i].velocity, last_frame_time_seconds));
                permanent->steer_data[i].acceleration = Vector3MultiplyScalar(permanent->steer_data[i].acceleration, 0);

                // left = frame 0, down = frame 1 right = frame 2,up = frame 3
                double angle = (180.0 / PI) * atan2(permanent->steer_data[i].velocity.x, permanent->steer_data[i].velocity.y);
                angle = angle + 180;

                if (angle > 315 || angle < 45) {
                    permanent->anim_data[i].frame = 10; //11
                } else if (angle >= 45 && angle <= 135) {
                    permanent->anim_data[i].frame = 4;// 5
                } else if (angle >=135 && angle <= 225) {
                    permanent->anim_data[i].frame = 6; //7
                } else if (angle > 225 && angle <= 315) {
                    permanent->anim_data[i].frame = 8; //9
                }



                permanent->anim_data[i].frame_duration_left += last_frame_time_seconds;
                if (permanent->anim_data[i].frame_duration_left > 0.1f) {
                    permanent->anim_data[i].frame += 1;
                }
                if (permanent->anim_data[i].frame_duration_left > 0.2f) {
                    permanent->anim_data[i].frame_duration_left = 0;
                }

                END_PERFORMANCE_COUNTER(actors_steering);
            }


#if 1
            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                Node16 * d= permanent->paths[i].Sentinel->Next;
                u32 c = permanent->colored_line_count;
                while (d->Next != permanent->paths[i].Sentinel){
                    permanent->colored_lines[c].x1 = getRotatedXFloat(d->path.node.x, d->path.node.y, facing_side, permanent->dims);
                    permanent->colored_lines[c].y1 = getRotatedYFloat(d->path.node.x, d->path.node.y, facing_side, permanent->dims);
                    permanent->colored_lines[c].z1 = d->path.node.z;

                    permanent->colored_lines[c].x2 = getRotatedXFloat(d->Next->path.node.x, d->Next->path.node.y, facing_side, permanent->dims);
                    permanent->colored_lines[c].y2 = getRotatedYFloat(d->Next->path.node.x, d->Next->path.node.y, facing_side, permanent->dims);
                    permanent->colored_lines[c].z2 = d->Next->path.node.z;

                    permanent->colored_lines[c].r = 0.0f;
                    permanent->colored_lines[c].g = 0.0f;
                    permanent->colored_lines[c].b = 0.0f;
                    d = d->Next;
                    c++;
                }
                permanent->colored_line_count = c;
            }
#endif
            ASSERT( permanent->colored_line_count < LINE_BATCH_COUNT * MAX_IN_BUFFER)

            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                continue;
            }



            BEGIN_PERFORMANCE_COUNTER(mass_pathfinding);
            TempMemory temp_mem = begin_temporary_memory(&scratch->arena);
            grid_node * Start = get_node_at(permanent->grid,
                                          permanent->steer_data[i].location.x/permanent->block_size.x,
                                          permanent->steer_data[i].location.y/permanent->block_size.y,
                                          (permanent->steer_data[i].location.z + 10)/permanent->block_size.z_level);
            if (Start->walkable) {
            } else {
                Start = get_neighboring_walkable_node(permanent->grid, Start->X, Start->Y, Start->Z);
                if (!Start->walkable) {
                    Start = get_random_walkable_node(permanent->grid);
                }
            }

            grid_node * End =  get_random_walkable_node(permanent->grid);
            ASSERT(Start->walkable);
            ASSERT(End->walkable);

            path_list * PathRaw = find_path(Start, End, permanent->grid, &scratch->arena);
            path_list *Path = NULL;

            if (PathRaw) {
                Path = smooth_path(PathRaw,  &scratch->arena, permanent->grid);

                if (Path) {
                    path_node * done= Path->Sentinel->Prev;
                    while (done != Path->Sentinel) {

                        Node16 *N = NULL;

                        if ( node16->Free->Next !=  node16->Free) {
                            N = node16->Free->Next;
                            node16->Free->Next = N->Next;
                            node16->Free->Next->Prev = node16->Free;
                        } else {
                            N = PUSH_STRUCT(&node16->arena, Node16);
                        }

                        ActorPath * p = &(permanent->paths[i]);

                        // TODO iam not sure about this, is this the right way to center all positions in the path nodes?
                        // should it be done somewhere else instead?
                        // TODO and why o why does it need something extra when facing Front and not under other circumstances.
                        float xMult = facing_side == Front ? 1 :  facing_side == Left ?  -0.5 : 0.5;
                        float yMult = facing_side == Front ? 0.5 : 1;
                        N->path.node.x =(permanent->block_size.x/2) * xMult + done->X * permanent->block_size.x;
                        N->path.node.y =(permanent->block_size.y/2) * yMult + done->Y * permanent->block_size.y;
                        N->path.node.z = done->Z * permanent->block_size.z_level;
			// if this is part of a stair move going up east/west



                        // TODO: this routine patches some issues which, IMO would better be solved at the root, dunno how wnad where though./
#if 1 // pathing movemenst going up on east/west stairs

                        if (facing_side == Front) {

                            if (done->Prev != Path->Sentinel) {
                                // going up
                                if (done->Z > done->Prev->Z) {
                                    if (done->X < done->Prev->X){
                                        N->path.node.x += permanent->block_size.x;
                                    } else if (done->X > done->Prev->X){
                                        N->path.node.x -= permanent->block_size.x;
                                    }
                                }
                            }
                            if (done->Next != Path->Sentinel) {
                                // going down
                                if (done->Z < done->Next->Z) {
                                    if (done->X < done->Next->X){
                                        N->path.node.x -= permanent->block_size.x;
                                    } else if (done->X > done->Next->X){
                                        N->path.node.x += permanent->block_size.x;
                                    }
                                }
                            }
                        }

                         if (facing_side == Left) {
                             if (done->Prev != Path->Sentinel) {
                             // going up
                                if (done->Z > done->Prev->Z) {
                                    if (done->Y < done->Prev->Y){
                                         N->path.node.y += permanent->block_size.y;
                                    } else if (done->Y > done->Prev->Y){
                                        N->path.node.y -= permanent->block_size.y;
                                    }
                                }
                             }
                             if (done->Next != Path->Sentinel) {
                                // going down
                                if (done->Z < done->Next->Z) {
                                    if (done->Y < done->Next->Y){
                                        N->path.node.y -= permanent->block_size.y;
                                    } else if (done->Y > done->Next->Y){
                                        N->path.node.y += permanent->block_size.y;
                                    }
                                }
                            }
                        }




                         if (facing_side == Right) {
                             if (done->Prev != Path->Sentinel) {
                             // going up
                                if (done->Z > done->Prev->Z) {
                                    if (done->Y < done->Prev->Y){
                                        N->path.node.y += permanent->block_size.y;
                                    } else if (done->Y > done->Prev->Y){
                                        N->path.node.y -= permanent->block_size.y;
                                    }
                                }
                             }
                             if (done->Next != Path->Sentinel) {
                                // going down
                                if (done->Z < done->Next->Z) {
                                    if (done->Y < done->Next->Y){
                                        N->path.node.y -= permanent->block_size.y;
                                    } else if (done->Y > done->Next->Y){
                                        N->path.node.y += permanent->block_size.y;
                                    }
                                }
                            }
                        }
#endif
                        DLIST_ADDFIRST(p, N);
                        done = done->Prev;
                    }
                }

            }
            END_PERFORMANCE_COUNTER(mass_pathfinding);
            //path_list * Path = ExpandPath(PathSmooth, &scratch->arena);
            BEGIN_PERFORMANCE_COUNTER(grid_cleaning);

            for (int i = 0; i < permanent->grid->width * permanent->grid->height * permanent->grid->depth;i++) {
                permanent->grid->nodes[i].f = 0;
                permanent->grid->nodes[i].g = 0;
                permanent->grid->nodes[i].opened = 0;
                permanent->grid->nodes[i].closed = 0;
                permanent->grid->nodes[i].Next = NULL;
                permanent->grid->nodes[i].parent = NULL;
            }
            END_PERFORMANCE_COUNTER(grid_cleaning);

            end_temporary_memory(temp_mem);

        }
        set_colored_line_batch_sizes(permanent, renderer);
    }
#endif

    BEGIN_PERFORMANCE_COUNTER(actors_data_gathering);
    for (u32 i = 0; i < permanent->actor_count; i++) {
        permanent->actors[i]._location =  getRotatedVec3( permanent->steer_data[i].location, facing_side, permanent->dims, permanent->block_size);

        permanent->actors[i]._palette_index = permanent->anim_data[i].palette_index;
        permanent->actors[i]._frame = permanent->anim_data[i].frame;

        permanent->actors[i].x_off = 0;
        permanent->actors[i].y_off = 0;

        if ( permanent->anim_data[i].frame == 4) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_west_body_000];
        } else if (permanent->anim_data[i].frame == 5) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_west_body_001];
        } else if (permanent->anim_data[i].frame == 6) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_south_body_000];
        } else if (permanent->anim_data[i].frame == 7) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_south_body_001];
        } else if (permanent->anim_data[i].frame == 8) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_east_body_000];
        } else if (permanent->anim_data[i].frame == 9) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_east_body_001];
        } else if (permanent->anim_data[i].frame == 10) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_north_body_000];
        } else if (permanent->anim_data[i].frame == 11) {
            permanent->actors[i].complex = &generated_body_frames[BP_walking_north_body_001];
        }

        /* if ( permanent->anim_data[i].frame % 2 == 0) { */
        /*     permanent->actors[i].complex = &generated_body_frames[BP_total_east_body_000]; */
        /* } else { */
        /*     permanent->actors[i].complex = &generated_body_frames[BP_total_east_body_001]; */
        /* } */
    }


    for (u32 i = permanent->actor_count; i < permanent->actor_count*2; i++) {
        u32 j = i - permanent->actor_count;
        permanent->actors[i]._location = getRotatedVec3( permanent->steer_data[j].location, facing_side, permanent->dims, permanent->block_size);
        permanent->actors[i]._palette_index = (1.0f / 16.0f) * (j % 16);//permanent->anim_data[j].palette_index;
        permanent->actors[i]._frame = permanent->anim_data[j].frame;

        int deltaX = permanent->actors[j].complex->anchorX - permanent->actors[j].complex->pivotX;
        int deltaY = permanent->actors[j].complex->anchorY - permanent->actors[j].complex->pivotY;

        permanent->actors[i].x_off = deltaX;
        permanent->actors[i].y_off = -1 * deltaY;

        if ( permanent->anim_data[j].frame == 4) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_west_head_000];
        } else if (permanent->anim_data[j].frame == 5) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_west_head_000];
        } else if (permanent->anim_data[j].frame == 6) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_south_head_000];
        } else if (permanent->anim_data[j].frame == 7) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_south_head_000];
        } else if (permanent->anim_data[j].frame == 8) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_east_head_000];
        } else if (permanent->anim_data[j].frame == 9) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_east_head_000];
        } else if (permanent->anim_data[j].frame == 10) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_north_head_000];
        } else if (permanent->anim_data[j].frame == 11) {
            permanent->actors[i].complex = &generated_body_frames[BP_facing_north_head_000];
        }


    }


    END_PERFORMANCE_COUNTER(actors_data_gathering);


    BEGIN_PERFORMANCE_COUNTER(actors_sort);
    Actor_quick_sort(permanent->actors, permanent->actor_count*2);
    END_PERFORMANCE_COUNTER(actors_sort);

}
