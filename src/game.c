#include "SDL.h"
#include <stdio.h>
#include "memory.h"
#include "renderer.h"

void game_update_and_render(Memory* memory,  RenderState *renderer);

internal int wallsortfunc (const void * a, const void * b)
{
    //1536 = some guestimate, assuming the depth is maximum 128.
    // and the height of each block is 128
    const Wall *a2 = (const Wall *) a;
    const Wall *b2 = (const Wall *) b;
    return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z));
}
typedef struct {
    int x_pos;
    int y_pos;
} BlockTextureAtlasPosition;

internal void set_wall_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->wall_count / 2048.0f);
    renderer->used_wall_batches = used_batches;

    if (used_batches == 1) {
        renderer->walls[0].count = permanent->wall_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->walls[i].count = 2048;
        }
        renderer->walls[used_batches-1].count = permanent->wall_count % 2048;
    } else {
        renderer->used_wall_batches = 0;
    }
}


extern void game_update_and_render(Memory* memory, RenderState *renderer) {
    ASSERT(sizeof(PermanentState) <= memory->permanent_size);
    PermanentState *permanent = (PermanentState *)memory->permanent;
    ASSERT(sizeof(ScratchState) <= memory->scratch_size);
    ScratchState *scratch = (ScratchState *)memory->scratch;
    ASSERT(sizeof(DebugState) <= memory->debug_size);
    DebugState *debug = (DebugState *)memory->debug;

    if (memory->is_initialized == false) {
        BlockTextureAtlasPosition texture_atlas_data[BlockTotal];
        printf("blocktotal %d\n", BlockTotal);

        texture_atlas_data[Floor]       =  (BlockTextureAtlasPosition){0,0};
        texture_atlas_data[WallBlock]   =  (BlockTextureAtlasPosition){1,0};
        texture_atlas_data[Ladder]      =  (BlockTextureAtlasPosition){2,0};
        texture_atlas_data[StairsUp1N]  =  (BlockTextureAtlasPosition){3,0};
        texture_atlas_data[StairsUp2N]  =  (BlockTextureAtlasPosition){4,0};
        texture_atlas_data[StairsUp3N]  =  (BlockTextureAtlasPosition){5,0};
        texture_atlas_data[StairsUp4N]  =  (BlockTextureAtlasPosition){6,0};
        texture_atlas_data[StairsUp1S]  =  (BlockTextureAtlasPosition){7,0};
        texture_atlas_data[StairsUp2S]  =  (BlockTextureAtlasPosition){8,0};
        texture_atlas_data[StairsUp3S]  =  (BlockTextureAtlasPosition){9,0};
        texture_atlas_data[StairsUp4S]  =  (BlockTextureAtlasPosition){10,0};
        texture_atlas_data[StairsUp1E]  =  (BlockTextureAtlasPosition){11,0};
        texture_atlas_data[StairsUp2E]  =  (BlockTextureAtlasPosition){12,0};
        texture_atlas_data[StairsUp3E]  =  (BlockTextureAtlasPosition){13,0};
        texture_atlas_data[StairsUp4E]  =  (BlockTextureAtlasPosition){14,0};
        texture_atlas_data[StairsUp1W]  =  (BlockTextureAtlasPosition){15,0};
        texture_atlas_data[StairsUp2W]  =  (BlockTextureAtlasPosition){16,0};
        texture_atlas_data[StairsUp3W]  =  (BlockTextureAtlasPosition){17,0};
        texture_atlas_data[StairsUp4W]  =  (BlockTextureAtlasPosition){18,0};
        texture_atlas_data[Shaded]      =  (BlockTextureAtlasPosition){19,0};

        ////

        int used_wall_block =0;
        int count_shadow = 0;
        int used_floors = 0;
        int wall_count =0;

        for (u32 z = 0; z < permanent->dims.z_level ; z++){
            for (u32 y = 0; y< permanent->dims.y; y++){
                for (u32 x = 0; x< permanent->dims.x; x++){
                    //renderer->assets.level
                    WorldBlock *b = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z,permanent->dims.x, permanent->dims.y)];

                    if (b->object == Nothing){

                    }
                    if (b->object == StairsUp1N || b->object == StairsUp2N || b->object == StairsUp3N || b->object == StairsUp4N) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }
                    if (b->object == StairsUp1S || b->object == StairsUp2S || b->object == StairsUp3S || b->object == StairsUp4S) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }
                    if (b->object == StairsUp1E || b->object == StairsUp2E || b->object == StairsUp3E || b->object == StairsUp4E) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }
                    if (b->object == StairsUp1W || b->object == StairsUp2W || b->object == StairsUp3W || b->object == StairsUp4W) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }


                    if (b->object == Floor){
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        used_floors++;
                    }



                    if (b->object == WallBlock){
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        wall_count++;
                    }
                    if (b->object == Ladder){
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                    }



                    // Shadow
                    //if (z+1 < permanent->dims.z_level-1 && b->object != Nothing) {
                    //printf("%d, %d, %d\n",x,y,z+1);

                    WorldBlock *one_above = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z+1 ,permanent->dims.x, permanent->dims.y)];


                    //if (one_above->object == Floor || (one_above->object >=StairsUp1N && one_above->object <= StairsUp4W)) {
                    if (one_above->object == Floor && z+1 < permanent->dims.z_level){
                        //if (z >= 0) {
                        count_shadow++;
                        //printf("%d, %d, %d, ..... index: %d \n",x,y,z, FLATTEN_3D_INDEX(x,y,(z_up),permanent->dims.x, permanent->dims.y));
                        //}

                        permanent->walls[used_wall_block].frame = texture_atlas_data[Shaded].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }
                    //}

                }
            }
        }
        permanent->wall_count = used_wall_block;
        qsort(permanent->walls, used_wall_block, sizeof(Wall), wallsortfunc);

        set_wall_batch_sizes(permanent, renderer);
        memory->is_initialized = true;
        printf("wall count: %d used wall block:%d \n", permanent->wall_count, used_wall_block);
        //        prepare_renderer(permanent, renderer);
    }

}
