
#include <stdio.h>
#include "memory.h"
#include "renderer.h"
#include "random.h"
#include "pathfind.h"
#include "states.h"
#include "level.h"
#include "data_structures.h"
#include "GLKMath.h"
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
//#define PI 3.14159265

internal GLKVector3 seek_return(ActorSteerData *actor, GLKVector3 target) {
    GLKVector3 desired = GLKVector3Subtract(target, actor->location);
    if (desired.x != 0 || desired.y != 0 || desired.z != 0) {
        desired = GLKVector3Normalize(desired);
    }
    desired = GLKVector3MultiplyScalar(desired, actor->max_speed);
    GLKVector3 steer = GLKVector3Subtract(desired, actor->velocity);
    steer = GLKVector3Limit(steer, actor->max_force);
    return steer;
}
internal void actor_apply_force(ActorSteerData *a, GLKVector3 force) {
    force = GLKVector3DivideScalar(force, a->mass);
    a->acceleration = GLKVector3Add(a->acceleration, force);
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
        //if (count % 100 == 0 ) printf("%d\n",count);
        count++;
    }
    return count;
}

void game_update_and_render(Memory* memory,  RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e);

internal int sort_static_blocks_back_front (const void * a, const void * b)
{
    //1536 = some guestimate, assuming the depth is maximum 128.
    // and the height of each block is 128
    const StaticBlock *a2 = (const StaticBlock *) a;
    const StaticBlock *b2 = (const StaticBlock *) b;

    // TODO maybe I need a special batch of tranparant things that are drawn back to front, so the rest can be faster
    // TODO heyhey the back to front order shows the same artifacts i see when trying to get actors drawn on top of all floors
    return (  (a2->y*16384 + a2->z ) - ( b2->y*16384 + b2->z));
    //return (  (a2->y*16384 -  a2->z) - ( b2->y*16384 - b2->z)); // this sorts walls back to front (needed for transparancy)
    //return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z)); //// this sorts walls front to back (much faster rendering)
}
internal int sort_static_blocks_front_back (const void * a, const void * b)
{
    //1536 = some guestimate, assuming the depth is maximum 128.
    // and the height of each block is 128
    const StaticBlock *a2 = (const StaticBlock *) a;
    const StaticBlock *b2 = (const StaticBlock *) b;

    // TODO maybe I need a special batch of tranparant things that are drawn back to front, so the rest can be faster
    // TODO heyhey the back to front order shows the same artifacts i see when trying to get actors drawn on top of all floors
    //return (  (a2->y*16384 + a2->z ) - ( b2->y*16384 + b2->z));
    //return (  (a2->y*16384 -  a2->z) - ( b2->y*16384 - b2->z)); // this sorts walls back to front (needed for transparancy)
    return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z)); //// this sorts walls front to back (much faster rendering)
}

internal void set_actor_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->actor_count / (MAX_IN_BUFFER * 1.0f));
    renderer->used_actor_batches = used_batches;

    if (used_batches == 1) {
        renderer->actors[0].count = permanent->actor_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches - 1; i++) {
            renderer->actors[i].count = MAX_IN_BUFFER;
        }
        renderer->actors[used_batches - 1].count = permanent->actor_count % MAX_IN_BUFFER;
    } else {
        renderer->used_actor_batches = 0;
    }
}

internal void set_dynamic_block_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->dynamic_block_count / 2048.0f);
    renderer->used_dynamic_block_batches = used_batches;

    if (used_batches == 1) {
        renderer->dynamic_blocks[0].count = permanent->dynamic_block_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->dynamic_blocks[i].count = 2048;
        }
        renderer->dynamic_blocks[used_batches-1].count = permanent->dynamic_block_count % 2048;
    } else {
        renderer->used_dynamic_block_batches = 0;
    }
}

internal void set_transparent_block_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->transparent_block_count / 2048.0f);
    renderer->used_transparent_block_batches = used_batches;

    if (used_batches == 1) {
        renderer->transparent_blocks[0].count = permanent->transparent_block_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->transparent_blocks[i].count = 2048;
        }
        renderer->transparent_blocks[used_batches-1].count = permanent->transparent_block_count % 2048;
    } else {
        renderer->used_transparent_block_batches = 0;
    }
}


internal void set_static_block_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->static_block_count / 2048.0f);
    renderer->used_static_block_batches = used_batches;

    if (used_batches == 1) {
        renderer->static_blocks[0].count = permanent->static_block_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->static_blocks[i].count = 2048;
        }
        renderer->static_blocks[used_batches-1].count = permanent->static_block_count % 2048;
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
    grid_node *result;// = GetNodeAt(grid, 0,0,0);
    do {
        result = get_node_at(grid, rand_int(grid->width), rand_int(grid->height), rand_int(grid->depth));
    } while (!result->walkable);
    return result;
}


extern void game_update_and_render(Memory* memory, RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e) {
    UNUSED(keys);
    UNUSED(e);
    UNUSED(last_frame_time_seconds);
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
        //printf("Used at permanent:  %lu\n", (unsigned long) permanent->arena.used);
        permanent->dynamic_blocks = (DynamicBlock*) PUSH_ARRAY(&permanent->arena, (16384), DynamicBlock);
        permanent->static_blocks = (StaticBlock*) PUSH_ARRAY(&permanent->arena, (16384), StaticBlock);
        permanent->transparent_blocks = (StaticBlock*) PUSH_ARRAY(&permanent->arena, (16384), StaticBlock);
        permanent->actors = (Actor*) PUSH_ARRAY(&permanent->arena, (16384*4), Actor);
        permanent->paths = (ActorPath*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorPath);
        permanent->steer_data = (ActorSteerData*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorSteerData);
        for (int i = 0; i < 16384*4; i++) {
            permanent->steer_data[i].mass = 1.0f;
            permanent->steer_data[i].max_force = 10;//rand_float()*100;// + 0.1f;
            permanent->steer_data[i].max_speed = 50 + rand_float()*80;//rand_float()*100;// + 0.1f;

        }
        permanent->anim_data = (ActorAnimData*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorAnimData);
        for (int i = 0; i < 16384*4; i++) {
            Node16 *Sentinel = (Node16 *) PUSH_STRUCT(&node16->arena, Node16);
            permanent->paths[i].Sentinel = Sentinel;
            permanent->paths[i].Sentinel->Next = Sentinel;
            permanent->paths[i].Sentinel->Prev = Sentinel;
            permanent->actors[i].index = i;
            permanent->paths[i].Sentinel->path.node = GLKVector3Make(-999,-999,-999);
        }
        //node16->Sentinel = PUSH_STRUCT(&node16->arena, Node16);
        //node16->Sentinel->Next =  node16->Sentinel;
        //node16->Sentinel->Prev =  node16->Sentinel;

        node16->Free = PUSH_STRUCT(&node16->arena, Node16);
        node16->Free->Next = node16->Free;//node16->Sentinel;
        node16->Free->Prev = node16->Free;//node16->Sentinel;
        //printf("Node16 used: %lu\n", node16->arena.used);
        permanent->glyphs = (Glyph*) PUSH_ARRAY(&permanent->arena, (16384), Glyph);
        permanent->colored_lines = (ColoredLine*) PUSH_ARRAY(&permanent->arena, (16384), ColoredLine);

        //printf("used scrathc space (before init): %lu\n", (unsigned long)scratch->arena.used);
        //printf("Used at permanent:  %lu\n", (unsigned long)permanent->arena.used);
        BlockTextureAtlasPosition texture_atlas_data[BlockTotal];
        //printf("blocktotal %d\n", BlockTotal);

        // TODO: spriteoffsetY


        // NOTE this offset is from the top left, normally I assume a 'block' is represented by
        // a sprite 24px wide and 108px high, the texture position is looking at that from the top left.
        // soo for example the floor is only 14px high, and thus in this case i

        texture_atlas_data[Floor]          =  (BlockTextureAtlasPosition){0*24 , 94,  24, 14 , 0, 0};
        texture_atlas_data[WallBlock]      =  (BlockTextureAtlasPosition){1*24 , 0,   24, 108, 0, 0};
        texture_atlas_data[LadderUpDown]   =  (BlockTextureAtlasPosition){2*24 , 0,   24, 108, 0, 0};
        texture_atlas_data[LadderUp]       =  (BlockTextureAtlasPosition){1*24 , 108, 24, 108, 0, 0};
        texture_atlas_data[WindowBlock]    =  (BlockTextureAtlasPosition){3*24 , 108, 24, 108, 0, 0};
        texture_atlas_data[LadderDown]     =  (BlockTextureAtlasPosition){6*24 , 108, 24, 108, 0, 0};

        //texture_atlas_data[Stairs1N]     =  (BlockTextureAtlasPosition){3*24 , 72,  24, 36, 0, 0};
        //texture_atlas_data[Stairs2N]     =  (BlockTextureAtlasPosition){3*24 , 72,  24, 36, 0, 24};
        //texture_atlas_data[Stairs3N]     =  (BlockTextureAtlasPosition){3*24 , 72,  24, 36, 0, 48};
        //texture_atlas_data[Stairs4N]     =  (BlockTextureAtlasPosition){3*24 , 72,  24, 36, 0, 72};

        texture_atlas_data[Stairs1N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 0};
        texture_atlas_data[Stairs2N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 24};
        texture_atlas_data[Stairs3N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 48};
        texture_atlas_data[Stairs4N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 72};

        texture_atlas_data[Stairs1S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 0};
        texture_atlas_data[Stairs2S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 24};
        texture_atlas_data[Stairs3S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 48};
        texture_atlas_data[Stairs4S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 72};

        texture_atlas_data[Stairs1E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 0};
        texture_atlas_data[Stairs2E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 24};
        texture_atlas_data[Stairs3E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 48};
        texture_atlas_data[Stairs4E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 72};

        texture_atlas_data[Stairs1W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 0};
        texture_atlas_data[Stairs2W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 24};
        texture_atlas_data[Stairs3W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 48};
        texture_atlas_data[Stairs4W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 72};

        //
        texture_atlas_data[EscalatorUp1N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 0};
        texture_atlas_data[EscalatorUp2N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 24};
        texture_atlas_data[EscalatorUp3N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 48};
        texture_atlas_data[EscalatorUp4N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 72};

        texture_atlas_data[EscalatorUp1S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 0};
        texture_atlas_data[EscalatorUp2S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 24};
        texture_atlas_data[EscalatorUp3S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 48};
        texture_atlas_data[EscalatorUp4S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 72};

        texture_atlas_data[EscalatorUp1E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 0};
        texture_atlas_data[EscalatorUp2E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 24};
        texture_atlas_data[EscalatorUp3E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 48};
        texture_atlas_data[EscalatorUp4E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 72};

        texture_atlas_data[EscalatorUp1W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 0};
        texture_atlas_data[EscalatorUp2W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 24};
        texture_atlas_data[EscalatorUp3W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 48};
        texture_atlas_data[EscalatorUp4W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 72};
        //
        texture_atlas_data[EscalatorDown1N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 0};
        texture_atlas_data[EscalatorDown2N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 24};
        texture_atlas_data[EscalatorDown3N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 48};
        texture_atlas_data[EscalatorDown4N]     =  (BlockTextureAtlasPosition){0 , 216,  24, 36, 0, 72};

        texture_atlas_data[EscalatorDown1S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 0};
        texture_atlas_data[EscalatorDown2S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 24};
        texture_atlas_data[EscalatorDown3S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 48};
        texture_atlas_data[EscalatorDown4S]     =  (BlockTextureAtlasPosition){7*24 , 92,  24, 16, 0, 72};

        texture_atlas_data[EscalatorDown1E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 0};
        texture_atlas_data[EscalatorDown2E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 24};
        texture_atlas_data[EscalatorDown3E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 48};
        texture_atlas_data[EscalatorDown4E]     =  (BlockTextureAtlasPosition){0, 312,  24, 36,  0, 72};

        texture_atlas_data[EscalatorDown1W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 0};
        texture_atlas_data[EscalatorDown2W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 24};
        texture_atlas_data[EscalatorDown3W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 48};
        texture_atlas_data[EscalatorDown4W]     =  (BlockTextureAtlasPosition){0, 252,  24, 36, 0, 72};




        texture_atlas_data[Shaded]         =  (BlockTextureAtlasPosition){19*24, 0,   24, 108, 0, 0};

        //int used_wall_block =0;
        int used_static_block_count = 0;
        int used_dynamic_block_count = 0;
        int used_transparent_block_count = 0;
        //int count_shadow = 0;
        int used_floors = 0;
        int used_walls = 0;
        int wall_count =0;



        int simplecount =0;

        //printf("level %d,%d,%d \n",permanent->dims.x,permanent->dims.y,permanent->dims.z_level);
        for (u32 z = 0; z < permanent->dims.z_level ; z++){
            for (u32 y = 0; y< permanent->dims.y; y++){
                for (u32 x = 0; x< permanent->dims.x; x++){
                    WorldBlock *b = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z,permanent->dims.x, permanent->dims.y)];
                    if (b->object == Nothing){

                    } else {
                        simplecount++;
                    }
                    permanent->static_blocks[used_static_block_count].is_floor = 0;
                    switch (b->object){
                    case Floor:
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = (y * permanent->block_size.y) ;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        permanent->static_blocks[used_static_block_count].is_floor = 1;
                        used_static_block_count++;
                        used_floors++;
                        break;
                    case WallBlock:
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;
                        used_walls++;
                        wall_count++;
                        break;
                    case WindowBlock:
                        permanent->transparent_blocks[used_transparent_block_count].frame = texture_atlas_data[b->object];
                        permanent->transparent_blocks[used_transparent_block_count].x = x * permanent->block_size.x;
                        permanent->transparent_blocks[used_transparent_block_count].y = y * permanent->block_size.y;
                        permanent->transparent_blocks[used_transparent_block_count].z = z * permanent->block_size.z_level;
                        used_transparent_block_count++;
                        break;
                    case LadderUpDown:
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;
                        break;
                    case LadderUp:
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;
                        break;
                    case LadderDown:
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;
                        break;
                    case Stairs1N:
                    case Stairs2N:
                    case Stairs3N:
                    case Stairs4N:
                        permanent->static_blocks[used_static_block_count].is_floor = 1;
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;
                        break;
                    case EscalatorUp1N:
                    case EscalatorUp2N:
                    case EscalatorUp3N:
                    case EscalatorUp4N:
                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 12;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].duration_per_frame = 0.5f / 12;
                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 1;

                        permanent->dynamic_blocks[used_dynamic_block_count].start_frame_x = texture_atlas_data[b->object].x_pos;
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
                        used_dynamic_block_count++;
                        break;
                    case EscalatorDown1N:
                    case EscalatorDown2N:
                    case EscalatorDown3N:
                    case EscalatorDown4N:
                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 12;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].duration_per_frame = 0.5f / 12;
                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 0;

                        permanent->dynamic_blocks[used_dynamic_block_count].start_frame_x = texture_atlas_data[b->object].x_pos;
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
                        used_dynamic_block_count++;
                        break;
		      //permanent->static_blocks[used_static_block_count].is_floor = 1;

                    case Stairs1E:
                    case Stairs2E:
                    case Stairs3E:
                    case Stairs4E:
                         permanent->static_blocks[used_static_block_count].is_floor = 1;
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;

                        break;
                    case EscalatorUp1E:
                    case EscalatorUp2E:
                    case EscalatorUp3E:
                    case EscalatorUp4E:
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].duration_per_frame = 0.5f / 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
                        used_dynamic_block_count++;
                        break;
                    case EscalatorDown1E:
                    case EscalatorDown2E:
                    case EscalatorDown3E:
                    case EscalatorDown4E:
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].duration_per_frame = 0.5f / 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
                        used_dynamic_block_count++;
                        break;

		      //permanent->static_blocks[used_static_block_count].is_floor = 1;

                    case Stairs1W:
                    case Stairs2W:
                    case Stairs3W:
                    case Stairs4W:
                        permanent->static_blocks[used_static_block_count].is_floor = 1;
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;

                        break;
                    case EscalatorUp1W:
                    case EscalatorUp2W:
                    case EscalatorUp3W:
                    case EscalatorUp4W:
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].duration_per_frame = 0.5f / 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 1;

                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
                        used_dynamic_block_count++;
                        break;
                    case EscalatorDown1W:
                    case EscalatorDown2W:
                    case EscalatorDown3W:
                    case EscalatorDown4W:
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].duration_per_frame = 0.5f / 8;
                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 0;

                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
                        used_dynamic_block_count++;
                        break;
		      //permanent->static_blocks[used_static_block_count].is_floor = 1;

                    case Stairs1S:
                    case Stairs2S:
                    case Stairs3S:
                    case Stairs4S:
                        permanent->static_blocks[used_static_block_count].is_floor = 1;
                        permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[b->object];
                        permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x;
                        permanent->static_blocks[used_static_block_count].y = y * permanent->block_size.y;
                        permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level;
                        used_static_block_count++;

                        break;
                    case EscalatorUp1S:
                    case EscalatorUp2S:
                    case EscalatorUp3S:
                    case EscalatorUp4S:
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;

                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
                        used_dynamic_block_count++;
                        break;
                    case EscalatorDown1S:
                    case EscalatorDown2S:
                    case EscalatorDown3S:
                    case EscalatorDown4S:
                        permanent->dynamic_blocks[used_dynamic_block_count].total_frames = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].current_frame = 0;

                        permanent->dynamic_blocks[used_dynamic_block_count].is_floor = 1;
                        permanent->dynamic_blocks[used_dynamic_block_count].plays_forward = 0;
                        permanent->dynamic_blocks[used_dynamic_block_count].frame = texture_atlas_data[b->object];
                        permanent->dynamic_blocks[used_dynamic_block_count].x = x * permanent->block_size.x;
                        permanent->dynamic_blocks[used_dynamic_block_count].y = y * permanent->block_size.y;
                        permanent->dynamic_blocks[used_dynamic_block_count].z = z * permanent->block_size.z_level;
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
                        //c = b;
                        //ASSERT("Problem!" && false);
                        break;
                    }

                    /* WorldBlock *one_above = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z+1 ,permanent->dims.x, permanent->dims.y)]; */

                    /* if (one_above->object == Floor && z+1 < permanent->dims.z_level ){ */
                    /*     //count_shadow++; */
                    /*     permanent->static_blocks[used_static_block_count].frame = texture_atlas_data[Shaded]; */
                    /*     permanent->static_blocks[used_static_block_count].x = x * permanent->block_size.x; */
                    /*     permanent->static_blocks[used_static_block_count].y = (y * permanent->block_size.y)-4; //TODO : what is this madness -4 ! */
                    /*     permanent->static_blocks[used_static_block_count].z = z * permanent->block_size.z_level; */
                    /*     used_static_block_count++; */
                    /* } */


                }
            }
        }
        //printf("simple block count : %d, floors:%d, walls:%d\n",simplecount, used_floors, used_walls);
        permanent->static_block_count = used_static_block_count;
        set_static_block_batch_sizes(permanent, renderer);

        permanent->dynamic_block_count = used_dynamic_block_count;
        set_dynamic_block_batch_sizes(permanent, renderer);

        permanent->transparent_block_count = used_transparent_block_count;
        set_transparent_block_batch_sizes(permanent, renderer);


        qsort(permanent->static_blocks, used_static_block_count, sizeof(StaticBlock), sort_static_blocks_front_back);
        qsort(permanent->transparent_blocks, used_transparent_block_count, sizeof(StaticBlock), sort_static_blocks_back_front);
        //set_wall_batch_sizes(permanent, renderer);

        //printf("wall count: %d used wall block:%d \n", permanent->wall_count, used_wall_block);

        renderer->needs_prepare = 1;
        //prepare_renderer(permanent, renderer);

        for (u32 i = 0; i < 1; i++) {
            permanent->steer_data[i].location.x = rand_int(permanent->dims.x) * permanent->block_size.x;
            permanent->steer_data[i].location.y = rand_int(permanent->dims.y) * permanent->block_size.y;
            permanent->steer_data[i].location.z = rand_int(0) * permanent->block_size.z_level;
            permanent->anim_data[i].frame = rand_int(4);
            float speed = 1; //10 + rand_int(10); // px per seconds
            permanent->steer_data[i].dx = rand_bool() ? -1 * speed : 1 * speed;
            permanent->steer_data[i].dy = rand_bool() ? -1 * speed : 1 * speed;
            permanent->anim_data[i].palette_index = rand_float();
        }

        set_actor_batch_sizes(permanent, renderer);

        permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);
        init_grid(permanent->grid, &permanent->arena, &permanent->level);
        preprocess_grid(permanent->grid);
        set_colored_line_batch_sizes(permanent, renderer);
        memory->is_initialized = true;
    }

    // dynamic blocks
    for (u32 i = 0; i < permanent->dynamic_block_count; i++) {
        if ( permanent->dynamic_blocks[i].total_frames > 1) {
            permanent->dynamic_blocks[i].frame_duration_left += last_frame_time_seconds;
            if (permanent->dynamic_blocks[i].frame_duration_left >= permanent->dynamic_blocks[i].duration_per_frame) {

                int frame_index = permanent->dynamic_blocks[i].current_frame;
                if (permanent->dynamic_blocks[i].plays_forward == 1) {
                    frame_index +=1;
                } else {
                    frame_index -= 1;
                }
                frame_index = (frame_index + permanent->dynamic_blocks[i].total_frames) % permanent->dynamic_blocks[i].total_frames;
                permanent->dynamic_blocks[i].current_frame = frame_index;
                permanent->dynamic_blocks[i].frame_duration_left = 0;
                permanent->dynamic_blocks[i].frame.x_pos = permanent->dynamic_blocks[i].start_frame_x + (permanent->dynamic_blocks[i].frame.width * frame_index);
                //                printf("new frame for index: %d x: %d, y: %d\n", i, permanent->dynamic_blocks[i].frame.x_pos, permanent->dynamic_blocks[i].frame.y_pos);
            }
        }
    }



#if 1
    {
        permanent->colored_line_count = 0;
        for (u32 i = 0; i < permanent->actor_count; i++) {

            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                float distance = GLKVector3Distance(permanent->steer_data[i].location, permanent->paths[i].Sentinel->Next->path.node);
                if (distance < 4){
                //if (permanent->paths[i].counter <=0) {
                    Node16 *First = permanent->paths[i].Sentinel->Next;
                    permanent->paths[i].Sentinel->Next = First->Next;
                    First->Next = node16->Free->Next;
                    node16->Free->Next = First;
                    First->Prev = node16->Free;
                }
            }

            // here i should peek at the first node in permanent->paths[i].Sentinel->Next->path.node
            // thats the position i want to steer towards
            // here is where i shoudl put that colored debug line drawing, it'll come in handy still



            BEGIN_PERFORMANCE_COUNTER(actors_steering);
            {
                GLKVector3 seek_force  = seek_return(&permanent->steer_data[i], permanent->paths[i].Sentinel->Next->path.node);
                seek_force = GLKVector3MultiplyScalar(seek_force, 1);
                actor_apply_force(&permanent->steer_data[i], seek_force);

                permanent->steer_data[i].velocity = GLKVector3Add(permanent->steer_data[i].velocity, permanent->steer_data[i].acceleration);
                permanent->steer_data[i].velocity = GLKVector3Limit(permanent->steer_data[i].velocity, permanent->steer_data[i].max_speed);
                permanent->steer_data[i].location = GLKVector3Add(permanent->steer_data[i].location,   GLKVector3MultiplyScalar(permanent->steer_data[i].velocity, last_frame_time_seconds));
                permanent->steer_data[i].acceleration = GLKVector3MultiplyScalar(permanent->steer_data[i].acceleration, 0);

                // left = frame 0, down = frame 1 right = frame 2,up = frame 3
                double angle = (180.0 / PI) * atan2(permanent->steer_data[i].velocity.x, permanent->steer_data[i].velocity.y);
                angle = angle + 180;
                //printf("%f \n",angle);
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




#if 0
            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                Node16 * d= permanent->paths[i].Sentinel->Next;
                u32 c = permanent->colored_line_count;
                while (d->Next != permanent->paths[i].Sentinel){

                    permanent->colored_lines[c].x1 = d->path.node.x;
                    permanent->colored_lines[c].y1 = d->path.node.y;
                    permanent->colored_lines[c].z1 = d->path.node.z;
                    permanent->colored_lines[c].x2 = d->Next->path.node.x;
                    permanent->colored_lines[c].y2 = d->Next->path.node.y;
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

            // if already have a path, i dont need a new one,
            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                continue;
            }



            BEGIN_PERFORMANCE_COUNTER(mass_pathfinding);
            TempMemory temp_mem = begin_temporary_memory(&scratch->arena);
            //TODO something is off with this, the end of a path seems almost never to be walkable
            grid_node * Start = get_node_at(permanent->grid,
                                          permanent->steer_data[i].location.x/permanent->block_size.x,
                                          permanent->steer_data[i].location.y/permanent->block_size.y,
                                          (permanent->steer_data[i].location.z+10) /permanent->block_size.z_level);
            if (Start->walkable) {
            } else {
                Start = get_neighboring_walkable_node(permanent->grid, Start->X, Start->Y, Start->Z);
                if (!Start->walkable) {
                    //printf("why wasnt my current node walkable? %d,%d,%d \n", Start->X, Start->Y, Start->Z);
                    Start = get_random_walkable_node(permanent->grid);
                }
            }
            //grid_node * Start = get_random_walkable_node(permanent->grid);
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

                        // TODO maybe i can fix the stairs oriented east/west and how the path is differnt going up then down just here..
                        N->path.node.x = done->X* permanent->block_size.x;
                        N->path.node.y = done->Y* permanent->block_size.y;
                        N->path.node.z = done->Z* permanent->block_size.z_level;
			// if this is part of a stair move going up east/west
#if 1 // pathing movemenst going up on east/west stairs
                        if (done->Prev != Path->Sentinel) {
                            if (done->Z > done->Prev->Z) {
                                if (done->X < done->Prev->X){
                                    N->path.node.x += permanent->block_size.x;
                                } else if (done->X > done->Prev->X){
                                    N->path.node.x -= permanent->block_size.x;
                                }
                            }
                        }
                        if (done->Next != Path->Sentinel) {
                            if (done->Z < done->Next->Z) {
                                if (done->X < done->Next->X){
                                    N->path.node.x -= permanent->block_size.x;
                                } else if (done->X > done->Next->X){
                                    N->path.node.x += permanent->block_size.x;
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
        //printf("colored lines: %d\n", permanent->colored_line_count);
        set_colored_line_batch_sizes(permanent, renderer);
        //printf("Node16 used: %lu \n", node16->arena.used );
    }
#endif












    BEGIN_PERFORMANCE_COUNTER(actors_data_gathering);

    // TODO for now this suffices, its the easiest and not TOO bad, but i loose 3ms for 64k actors on sorting this way
    // what happens like this is the data is not sorted at all and then i sort it every frame.
    // the solution (I think) would be using some index, so i can leave the renderable data sorted as is, and gather from the correct places.
    // it costs a little in cache misses when gathering, but not as much as the sort is costing more now.
    // maybe as an alternative I can look into using a separate thread to sort this data on, and flip flop data.
    // btw not sorting only costs 10ms per frame (@64k) vs 17/18, but i think the sorting is specifically desired on rpi (TODO TEST THIS AGAiN)


    for (u32 i = 0; i < permanent->actor_count; i++) {
        //printf("index: %d\n",permanent->actors[i].index);

        //permanent->actors[i]._location = permanent->steer_data[permanent->actors[i].index].location;
        //permanent->actors[i]._palette_index = permanent->anim_data[permanent->actors[i].index].palette_index;
        //permanent->actors[i]._frame = permanent->anim_data[permanent->actors[i].index].frame;

        permanent->actors[i]._location = permanent->steer_data[i].location;//
        permanent->actors[i]._palette_index = permanent->anim_data[i].palette_index;//
        permanent->actors[i]._frame = permanent->anim_data[i].frame;//

    }

    END_PERFORMANCE_COUNTER(actors_data_gathering);


    BEGIN_PERFORMANCE_COUNTER(actors_sort);
    //    qsort, timsort, quick_sort
    //64k 7.3    4.8      3.7
    //qsort(permanent->actors,  permanent->actor_count, sizeof(Actor), actorsortfunc);
    //TODO later on when I use binning for steering I might be able to improve sorting
    Actor_quick_sort(permanent->actors, permanent->actor_count);
    //Actor_sqrt_sort(permanent->actors, permanent->actor_count);
    //Actor_tim_sort(permanent->actors,  permanent->actor_count);
    END_PERFORMANCE_COUNTER(actors_sort);

}
